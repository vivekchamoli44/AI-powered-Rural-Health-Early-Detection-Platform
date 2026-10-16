#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <winhttp.h>
#include "crow_all.h"
#include "models.h"
#include "db_history.h"
#include "nlohmann/json.h"
using namespace std;
#pragma comment(lib, "winhttp.lib")\
namespace health {
  inline string load_env_var(const std::string& key_name) {
    const char* sys_val = std::getenv(key_name.c_str());
    if (sys_val && std::string(sys_val).length() > 0) {
        return std::string(sys_val);
    }
    vector<std::string> env_candidates = {
        ".env",
        "../.env",
        "backend/.env",
        "../../.env"
    };
  for (const auto& path : env_candidates) {
        ifstream f(path);
        if (!f.is_open()) continue;

        std::string line;
        while (std::getline(f, line)) {
            auto start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos || line[start] == '#') continue;
            auto eq_pos = line.find('=', start);
            if (eq_pos == std::string::npos) continue;
            std::string k = line.substr(start, eq_pos - start);
            auto k_end = k.find_last_not_of(" \t");
            if (k_end != std::string::npos) k = k.substr(0, k_end + 1);
            if (k == key_name) {
                std::string v = line.substr(eq_pos + 1);
                auto v_start = v.find_first_not_of(" \t");
                auto v_end = v.find_last_not_of(" \t\r\n");
                if (v_start != std::string::npos && v_end != std::string::npos) {
                    v = v.substr(v_start, v_end - v_start + 1);
                    // Strip optional quotes
                    if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
                        v = v.substr(1, v.size() - 2);
                    }
                    return v;
                }
            }
        }
    }
    return "";
}
// Gemini Clinical AI Engine
class GeminiAIEngine {
private:
    string api_key_;
    static wstring to_wide(const std::string& str) {
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
        return wstr;
    }
    // Native Windows HTTPS POST for Google Gemini REST API
    string http_post(const wstring& host, const wstring& path, const string& json_body) {
        string response_data;
        HINTERNET hSession = WinHttpOpen(L"AI-Health-Center-Vivek/2.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return "";
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return "";
        }
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                                NULL, WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }
        WinHttpSetTimeouts(hRequest, 5000, 5000, 10000, 10000);
        wstring headers = L"Content-Type: application/json\r\n";
        BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(),
                                           (LPVOID)json_body.c_str(), (DWORD)json_body.length(),
                                           (DWORD)json_body.length(), 0);
        if (bResults) {
            bResults = WinHttpReceiveResponse(hRequest, NULL);
        }

        if (bResults) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (dwSize == 0) break;
                vector<char> buffer(dwSize + 1, 0);
                if (WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded)) {
                    response_data.append(buffer.data(), dwDownloaded);
                }
            } while (dwSize > 0);
        }
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response_data;
    }
    struct GeminiOverloadedError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    struct GeminiQuotaExhaustedError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    AIResult request_gemini_assessment(const Assessment& req, const std::string& model) {
        string prompt =
            "You are an expert AI clinical health diagnostician specialized in early chronic disease detection and community triage for ASHA health workers in India.\n"
            "Evaluate this patient's assessment data carefully:\n"
            "- Age: " + std::to_string(req.age) + "\n"
            "- Gender: " + req.gender + "\n"
            "- Patient Reported Symptoms & Duration: " + req.symptoms_text + "\n"
            "- Medical History, Family Risk & Lifestyle: " + req.medical_history + "\n\n"
            "Return a STRICT JSON response ONLY (no markdown formatting, no ```json codeblocks, plain text JSON only) with these exact keys:\n"
            "{\n"
            "  \"risk_score\": <integer 0-100>,\n"
            "  \"risk_level\": <\"Low\" | \"Moderate\" | \"High\" | \"Critical\">,\n"
            "  \"possible_chronic_diseases\": [<list of probable conditions>],\n"
            "  \"recommended_doctor_specialty\": <string e.g. \"Cardiologist\", \"Endocrinologist\", \"General Physician\">,\n"
            "  \"other_symptoms_to_watch\": [<list of other warning symptoms to monitor>],\n"
            "  \"precautions\": [<list of actionable lifestyle and dietary precautions>],\n"
            "  \"immediate_actions\": <concise clinical next action>\n"
            "}";
        nlohmann::json request_payload = {
            {"contents", {{{"parts", {{{"text", prompt}}}}}}},
            {"generationConfig", {{"temperature", 0.2}, {"response_mime_type", "application/json"}}}
        };
        wstring host = L"generativelanguage.googleapis.com";
        wstring path = L"/v1beta/models/" + to_wide(model) + L":generateContent?key=" + to_wide(api_key_);
        string raw_resp = http_post(host, path, request_payload.dump());
        if (raw_resp.empty()) {
            throw GeminiOverloadedError("Gemini API (" + model + ") did not respond. Check network connectivity.");
        }
        auto parsed = nlohmann::json::parse(raw_resp);
        if (parsed.contains("error")) {
            string status = parsed["error"].value("status", "");
            int code = parsed["error"].value("code", 0);
            string msg = "Gemini API (" + model + ") error: " + parsed["error"].dump();
            if (status == "RESOURCE_EXHAUSTED" || code == 429) {
                throw GeminiQuotaExhaustedError(msg);
            }
            if (status == "UNAVAILABLE" || code == 503) {
                throw GeminiOverloadedError(msg);
            }
            throw std::runtime_error(msg);
        }
        if (!parsed.contains("candidates") || parsed["candidates"].empty()) {
            throw std::runtime_error("Gemini API (" + model + ") returned no candidates.");
        }
        auto& cand = parsed["candidates"][0];
        if (!cand.contains("content") || !cand["content"].contains("parts") || cand["content"]["parts"].empty()) {
            throw std::runtime_error("Gemini API (" + model + ") response missing content.");
        }
        string text_content = cand["content"]["parts"][0]["text"].get<std::string>();
        if (text_content.find("```json") != std::string::npos) {
            auto p1 = text_content.find("```json");
            text_content = text_content.substr(p1 + 7);
        } else if (text_content.find("```") != std::string::npos) {
            auto p1 = text_content.find("```");
            text_content = text_content.substr(p1 + 3);
        }
        if (text_content.rfind("```") != std::string::npos) {
            text_content = text_content.substr(0, text_content.rfind("```"));
        }
        auto ai_json = nlohmann::json::parse(text_content);
        AIResult res;
        res.risk_score = ai_json.value("risk_score", 45);
        res.risk_level = ai_json.value("risk_level", "Moderate");
        if (ai_json.contains("possible_chronic_diseases") && ai_json["possible_chronic_diseases"].is_array()) {
            for (auto& item : ai_json["possible_chronic_diseases"]) {
                res.possible_chronic_diseases.push_back(item.get<std::string>());
            }
        }
        res.recommended_doctor = ai_json.value("recommended_doctor_specialty", "General Physician");
        if (ai_json.contains("other_symptoms_to_watch") && ai_json["other_symptoms_to_watch"].is_array()) {
            for (auto& item : ai_json["other_symptoms_to_watch"]) {
                res.other_symptoms_to_watch.push_back(item.get<std::string>());
            }
        }
        if (ai_json.contains("precautions") && ai_json["precautions"].is_array()) {
            for (auto& item : ai_json["precautions"]) {
                res.precautions.push_back(item.get<std::string>());
            }
        }
        res.immediate_actions = ai_json.value("immediate_actions", "Consult physician for care assessment.");
        cout << "[Vivek - AI Module] Successfully parsed live Gemini AI diagnosis (model: " << model << ")! Risk: " << res.risk_score << std::endl;
        return res;
    }

public:
    GeminiAIEngine(const std::string& explicit_key = "") {
        if (!explicit_key.empty()) {
            api_key_ = explicit_key;
        } else {
            api_key_ = load_env_var("GEMINI_API_KEY");
        }
    }
    bool has_api_key() const { return !api_key_.empty() && api_key_.length() > 10; }
    AIResult assess_patient(const Assessment& req) {
        if (!has_api_key()) {
            throw std::runtime_error("Gemini API key is not configured. Set GEMINI_API_KEY in .env or config.json.");
        }
        static const std::vector<std::string> models = {
            "gemini-flash-lite-latest",
            "gemini-2.5-flash-lite",
            "gemini-2.5-flash"
        };
        const int max_attempts_per_model = 2;
        string last_error = "Unknown error.";
        for (const auto& model : models) {
            for (int attempt = 1; attempt <= max_attempts_per_model; ++attempt) {
                try {
                    cout << "[Vivek - AI Module] Sending assessment request to Google Gemini API (model: "
                              << model << ", attempt " << attempt << "/" << max_attempts_per_model << ")..." << std::endl;
                    return request_gemini_assessment(req, model);
                } catch (const GeminiQuotaExhaustedError& e) {
                    last_error = e.what();
                    cout<< last_error << std::endl;
                    break;
                } catch (const GeminiOverloadedError& e) {
                    last_error = e.what();
                    cout<< last_error << std::endl;
                    if (attempt < max_attempts_per_model) {
                        Sleep(800 * attempt);
                    }
                } catch (const std::exception& e) {
                    last_error = e.what();
                    cout << "[Vivek - AI Module] " << last_error << std::endl;
                    break;
                }
            }
            cout<< model << "' unavailable, trying next free-tier Gemini model..." << std::endl;
        }
        throw std::runtime_error("All free-tier Gemini models are currently unavailable. Last error: " + last_error);
    }
};
