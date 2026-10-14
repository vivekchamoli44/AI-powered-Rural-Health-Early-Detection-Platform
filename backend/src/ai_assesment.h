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
#pragma comment(lib, "winhttp.lib")\
namespace health {
  inline std::string load_env_var(const std::string& key_name) {
    const char* sys_val = std::getenv(key_name.c_str());
    if (sys_val && std::string(sys_val).length() > 0) {
        return std::string(sys_val);
    }
    std::vector<std::string> env_candidates = {
        ".env",
        "../.env",
        "backend/.env",
        "../../.env"
    };
  for (const auto& path : env_candidates) {
        std::ifstream f(path);
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


