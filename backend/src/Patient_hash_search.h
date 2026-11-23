#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "crow_all.h"
#include "models.h"
#include "db_history.h"
#include "auth_notification.h"
#include "dashboard_triage.h"
#include "nlohmann/json.h"
namespace health {
inline std::string normalize_hash_key(const std::string& input) {
    std::string clean = "";
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return clean;
}
class PatientHashTable {
private:
    struct HashNode {
        std::string key;
        std::vector<Assessment> records;
        HashNode* next = nullptr;

        HashNode(const std::string& k, const Assessment& a) : key(k) {
            records.push_back(a);
        }
    };
    static const int TABLE_SIZE = 101;
    std::vector<HashNode*> buckets_;
    int total_entries_ = 0;
    int hash_function(const std::string& key) const {
        if (key.empty()) return 0;
        const int p = 31;
        long long hash_val = 0;
        long long p_pow = 1;

        for (char c : key) {
            hash_val = (hash_val + (static_cast<unsigned char>(c) + 1) * p_pow) % TABLE_SIZE;
            p_pow = (p_pow * p) % TABLE_SIZE;
        }
        return static_cast<int>((hash_val + TABLE_SIZE) % TABLE_SIZE);
    }

    void free_buckets() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            HashNode* curr = buckets_[i];
            while (curr != nullptr) {
                HashNode* prev = curr;
                curr = curr->next;
                delete prev;
            }
            buckets_[i] = nullptr;
        }
        total_entries_ = 0;
    }

public:
    PatientHashTable() : buckets_(TABLE_SIZE, nullptr), total_entries_(0) {}

    ~PatientHashTable() {
        free_buckets();
    }


    PatientHashTable(const PatientHashTable&) = delete;
    PatientHashTable& operator=(const PatientHashTable&) = delete;

    void clear() {
        free_buckets();
    }

    int size() const {
        return total_entries_;
    }

    int table_size() const {
        return TABLE_SIZE;
    }

  
    void insert(const std::string& raw_key, const Assessment& record) {
        std::string key = normalize_hash_key(raw_key);
        if (key.empty()) return;

        int idx = hash_function(key);
        HashNode* curr = buckets_[idx];
        while (curr != nullptr) {
            if (curr->key == key) {
                for (const auto& existing : curr->records) {
                    if (existing.id == record.id) return;
                }
                curr->records.push_back(record);
                total_entries_++;
                return;
            }
            curr = curr->next;
        }

        HashNode* new_node = new HashNode(key, record);
        new_node->next = buckets_[idx];
        buckets_[idx] = new_node;
        total_entries_++;
    }

    std::vector<Assessment> search(const std::string& raw_key) const {
        std::string key = normalize_hash_key(raw_key);
        if (key.empty()) return {};

        int idx = hash_function(key);
        HashNode* curr = buckets_[idx];

        while (curr != nullptr) {
            if (curr->key == key) {
                return curr->records;
            }
            curr = curr->next;
        }
        return {};
    }

    void index_assessment(const Assessment& a) {
        // 1. Index by full patient name
        insert(a.patient_name, a);
        std::stringstream ss(a.patient_name);
        std::string token;
        while (ss >> token) {
            insert(token, a);
        }

        insert(a.phone, a);
        insert(std::to_string(a.patient_id), a);

        insert(std::to_string(a.id), a);
    }
};
inline void register_patient_hash_search_routes(crow::SimpleApp& app, DatabaseManager& db, AuthSessionManager& auth) {

    CROW_ROUTE(app, "/api/worker/assessments/search").methods(crow::HTTPMethod::Get)
    ([&db, &auth](const crow::request& req) {
        crow::response res;
        res.set_header("Content-Type", "application/json");

        User worker;
        if (!require_asha_worker(req, auth, worker, res)) return res;

        std::string query = req.url_params.get("q") ? req.url_params.get("q") : "";
        if (query.empty()) {
            res.code = 400;
            res.body = nlohmann::json({
                {"success", false},
                {"error", "Query parameter 'q' is required for search."}
            }).dump();
            return res;
        }
        auto all_assessments = db.get_all_assessments_sorted();

        PatientHashTable hash_table;
        int area_patients_count = 0;
        for (const auto& a : all_assessments) {
            if (in_worker_area(worker, a.village_area)) {
                hash_table.index_assessment(a);
                area_patients_count++;
            }
        }
        std::vector<Assessment> matched = hash_table.search(query);

        nlohmann::json results = nlohmann::json::array();
        for (const auto& a : matched) {
            results.push_back(a.to_json());
        }

        res.code = 200;
        res.body = nlohmann::json({
            {"success", true},
            {"module", "ASHA Worker Module: Patient Search via Hashing"},
            {"algorithm", "Hash Table with Separate Chaining (Polynomial Rolling Hash)"},
            {"query", query},
            {"area_patients_indexed", area_patients_count},
            {"total_matches", results.size()},
            {"results", results}
        }).dump();
        return res;
    });
}

}
