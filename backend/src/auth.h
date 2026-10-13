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
