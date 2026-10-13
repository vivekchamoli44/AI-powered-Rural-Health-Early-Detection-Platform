#include <string>
#include<unoredered_map>
#include<mutex>
#include<random>
#include<iostream>
#include "crow_all.h"
#include "models.h"
#include "db_history.h"
#include "nlohmann/json.h"
namespace health {
class AuthSessionManager {
private :
std ::unordered_map<std::string,User>
sessions_;
std::string generate_token() {
  static const char alphanum[] = "012345678910ABCDEFGHIJKLMNOPQRSTUYWXYZabcdefghijklmnopqrsuvwxyz";
std ::random_device rd ;
std ::mt19937 gen(rd());
std ::uniform_int_intdistribution<>
dis(0, sizeof(alphanum)-2);
std :: string s = "sess_"
for (int i = 0; i<32;i++) s +=
  alphanum[dis(gen)];
return s;
}
public :
std ::lock_guard<std::mutex>
lock(auth_mutex_);
std::string token = generate_token();
sessions_[token] = user ;
return token ;
}
bool get_user(const std ::string& token , user& out_user){
  if (token.empty()) return false ;
std ::lock_guard<std::mutex>
lock(auth_mutex_);
auto it = sessions_.find(token);
if (it = sessions_.end()){
  out_user = it->econd;
  return true ;
  
}
return false ;
}
void destroy_session(const std ::string&token) {
std :: lock_guard<std::mutex>
lock(auth_mutex_);
sessions_.erase(token);
}
};
// Route registration for Yashwardhan 
inline void register_auth_notification _routes(crow::SimpleApp& app, DatabaseManager &
CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::Post)
([&db , &auth](const crow::request& req) {
 crow::response res;
        res.set_header("Content-Type", "application/json");

        try {
            auto body = nlohmann::json::parse(req.body);
            User u;
            // Support 'email' / 'gmail' field (or fallback to username if provided)
            std::string email = body.value("email", "");
            if (email.empty()) email = body.value("gmail", "");
            if (email.empty()) email = body.value("username", "");

            u.email = email;
            u.username = email; // For DB uniqueness
            u.password_hash = body.value("password", "");
            u.role = body.value("role", "patient");
            u.full_name = body.value("full_name", "");
            u.phone = body.value("phone", "");
            u.village_area = body.value("village_area", "");
            u.age = body.value("age", 0);
            u.gender = body.value("gender", "Not Specified");

            if (u.email.empty() || u.password_hash.empty() || u.full_name.empty()) {
                res.code = 400;
                res.body = nlohmann::json({{"error", "Gmail address, password, and full name are required."}}).dump();
                return res;
            }

            // Gmail validation check: Must contain @ and end with gmail.com or have valid domain
            if (u.email.find('@') == std::string::npos) {
                res.code = 400;
                res.body = nlohmann::json({{"error", "Please provide a valid Gmail address (e.g., yourname@gmail.com)."}}).dump();
                return res;
            }

            int new_id = 0;
            if (!db.create_user(u, new_id)) {
                res.code = 409;
                res.body = nlohmann::json({{"error", "An account with this Gmail already exists."}}).dump();
                return res;
            }

            u.id = new_id;
            std::string token = auth.create_session(u);

            // OOP demo: build the role-specific Person subclass (Patient/AshaWorker)
            // and log its polymorphic role_description() via the Person base pointer.
            auto person = make_person(u);
            std::cout << "[Auth Module] New account: " << person->full_name()
                      << " -> " << person->role_description() << std::endl;

            res.code = 201;
            res.body = nlohmann::json({
                {"success", true},
                {"module", "User Registration (Sign Up)"},
                {"developer", "Yashwardhan Rohilla"},
                {"token", token},
                {"user", u.to_json()}
            }).dump();
            return res;
        } catch (const std::exception& e) {
            res.code = 400;
            res.body = nlohmann::json({{"error", std::string("Registration error: ") + e.what()}}).dump();
            return res;
        }
    });

// gmail authentication completed
 CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::Post)
    ([&db, &auth](const crow::request& req) {
        crow::response res;
        res.set_header("Content-Type", "application/json");

        try {
            auto body = nlohmann::json::parse(req.body);
            std::string email = body.value("email", "");
            if (email.empty()) email = body.value("gmail", "");
            if (email.empty()) email = body.value("username", "");
            std::string password = body.value("password", "");

            if (email.empty() || password.empty()) {
                res.code = 400;
                res.body = nlohmann::json({{"error", "Gmail and password are required."}}).dump();
                return res;
            }

            User u;
            if (!db.authenticate_user(email, password, u)) {
                res.code = 401;
                res.body = nlohmann::json({{"error", "Invalid Gmail address or password."}}).dump();
                return res;
            }

            std::string token = auth.create_session(u);

         
            auto person = make_person(u);
            std::cout << "[Auth Module] Login: " << person->full_name()
                      << " -> " << person->role_description() << std::endl;

            res.code = 200;
            res.body = nlohmann::json({
                {"success", true},
                {"module", "User Authentication (Login)"},
                {"developer", "Yashwardhan Rohilla"},
                {"token", token},
                {"user", u.to_json()}
            }).dump();
            return res;
        } catch (const std::exception& e) {
            res.code = 400;
            res.body = nlohmann::json({{"error", std::string("Login error: ") + e.what()}}).dump();
            return res;
        }
    });

    // GET /api/auth/me
    CROW_ROUTE(app, "/api/auth/me").methods(crow::HTTPMethod::Get)
    ([&auth](const crow::request& req) {
        crow::response res;
        res.set_header("Content-Type", "application/json");
        std::string token = req.get_header_value("Authorization");
        if (token.rfind("Bearer ", 0) == 0) token = token.substr(7);

        User u;
        if (!auth.get_user(token, u)) {
            res.code = 401;
            res.body = nlohmann::json({{"error", "Not authenticated"}}).dump();
            return res;
        }

        res.code = 200;
        res.body = nlohmann::json({{"authenticated", true}, {"user", u.to_json()}}).dump();
        return res;
    });
 // POST /api/auth/logout
    CROW_ROUTE(app, "/api/auth/logout").methods(crow::HTTPMethod::Post)
    ([&auth](const crow::request& req) {
        crow::response res;
        res.set_header("Content-Type", "application/json");
        std::string token = req.get_header_value("Authorization");
        if (token.rfind("Bearer ", 0) == 0) token = token.substr(7);
        auth.destroy_session(token);
        res.code = 200;
        res.body = nlohmann::json({{"success", true}}).dump();
        return res;
    });
   CROW_ROUTE(app, "/api/worker/assessments/<int>/alert").methods(crow::HTTPMethod::Post)
    ([&db](const crow::request& req, int assessment_id) {
        crow::response res;
        res.set_header("Content-Type", "application/json");

        std::string custom_message = "";
        try {
            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                custom_message = body.value("message", "");
            }
        } catch (...) {}

        if (custom_message.empty()) {
            custom_message = "HIGH RISK ALERT: ASHA Healthcare Worker has flagged critical risk for this patient.";
        }

        if (db.update_alert(assessment_id, custom_message)) {
            std::cout << "[Notification Module] Alert dispatched for Assessment #" << assessment_id << std::endl;
            res.code = 200;
            res.body = nlohmann::json({
                {"success", true},
                {"module", "ASHA Worker: Send Notification"},
                {"developer", "Yashwardhan Rohilla"},
                {"message", "Emergency alert notification successfully dispatched to patient and health center."},
                {"assessment_id", assessment_id}
            }).dump();
        } else {
            res.code = 404;
            res.body = nlohmann::json({{"error", "Assessment record not found."}}).dump();
        }
        return res;
    });
}

}
// asha worker send notification module completed


   
