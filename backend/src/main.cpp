#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "crow_all.h"
#include "db_history.h"
#include "ai_assessment.h"
#include "auth_notification.h"
#include "dashboard_triage.h"
namespace fs = std::filesystem;
// Static file reader for frontend assets
std::string read_frontend_file(const std::string& relative_path) {
    std::vector<std::string> search_paths = {
        "../frontend/" + relative_path,
        "./frontend/" + relative_path,
        "../../frontend/" + relative_path,
        "frontend/" + relative_path
    };
    for (const auto& p : search_paths) {
        if (fs::exists(p)) {
            std::ifstream file(p, std::ios::binary);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        }
    }
    return "";
}
