#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "crow_all.h"
#include "models.h"
#include "db_history.h"
#include "nlohmann/json.h"
namespace health {
inline void register_dashboard_triage_routes(crow::SimpleApp& app, DatabaseManager& db) {
    CROW_ROUTE(app, "/api/worker/assessments").methods(crow::HTTPMethod::Get)
    ([&db](const crow::request& ) {
        crow::response res;
        res.set_header("Content-Type", "application/json");
        auto assessments = db.get_all_assessments_sorted();
        nlohmann::json arr = nlohmann::json::array();
        int critical_count = 0;
        int high_count = 0;
        int alerts_sent_count = 0;
        for (const auto& a : assessments) {
            arr.push_back(a.to_json());
            if (a.risk_level == "Critical") critical_count++;
            if (a.risk_score > 80) high_count++;
            if (a.alert_sent) alerts_sent_count++;
        }
        res.code = 200;
        res.body = nlohmann::json({
            {"success", true},
            {"module", "ASHA Worker Module: High Risk Individual & Dashboard"},
            {"developer", "Nandini"},
            {"total_screenings", assessments.size()},
            {"critical_risk_count", critical_count},
            {"high_risk_count", high_count},
            {"alerts_sent_count", alerts_sent_count},
            {"assessments", arr}
        }).dump();
        return res;
    });
