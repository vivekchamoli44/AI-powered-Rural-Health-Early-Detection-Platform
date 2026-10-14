#include<string>
#include<vector>
#include<iostream>
#include<sstream>
#include<mutex>
#include<utility>
#include<chrono>
#include<iomanip>
#include<mysql.h>
#include "crow_all.h"
#include "models.h"
#include "nlohmann/json.h"
namespace health{
  class AssessmentMaxheap{
        private:
               std::vector<Assessment>data_;

            static int parent(int i){ return (i-1)/2; }
            static int left(int i){ return 2 * i + 1; }
            static int right(int i){return 2 * i + 2; }

           void sift_up(int i){
               while(i > 0 && data_[parent(i)].risk_score < data_[i].risk_score){
                    std::swap(data_[parent(i)], data_[i]);
                      i = parent(i);
               }
           }

             void sift_down(int i){
                 int n= (int)data_.size();
                  while(true){
                       int l = left(i), r = right(i), largest = i;
                       if (l < n && data_[l].risk_score > data_[largest].risk_score) largest = l;
                       if (r < n && data_[r].risk_score > data_[largest].risk_score) largest = r;
                       if (largest == i) break;
                       std::swap(data_[i], data_[largest]);
                       i = largest;
                  }
             }
       public:
             bool empty() const { return data_.empty(); }

             void insert(const Assessment& a){
                   data_.push_back(a);
                   sift_up((int)data_.size() - 1);
             }

          //Removes and returns the assessment with the highest risk_score.
            Assessment extract_max(){
                Assessment top = data_.front();
                data_.front() = dat_.back();
                data_.pop_back();
                if (!data_.empty()) sift_down(0);
                return top;
            }

          //Drains the whole heap into a vector ordered highest-risk-first.
            std::vector<Assessment> drain_sorted(){
                 std::vector<Assessment> out;
                 out.reverse(data_.size());
                 while (!empty()) out.push_back(extract_max());
                 return out;
            }
         };

       class DatabaseManager {
           private:
                std::string host_;
                int port_;
                std::string user_;
                std::string password_;
                std::string dbname_;
                MYSQL* conn_ = nullptr;
                std::mutex db_mutex_;
                bool is_connected_ = false;

             //In-memory fallback stronge
              std::vector<User> memory_users_;
              std::vector<Assessment> memory_assessments_;
              int next_user_id_ = 10;    //IDs 1-2 are reserved for the seeded demo login accounts
              int next_assessment_id_ = 1;  //No assessments are seeded; real submissions start at 1

              void seed_default_records(){
                       User asha;
                       asha.id = 1;
                       asha.username = "asha.worken@gmail.com";
                       asha.email = "asha.worker@gmail.com";
                       asha.password_hash = "asha123";
                       asha.role = "asha_worker";
                       asha.full_name = "Sunita Devi (ASHA Worker)";
                       asha.phone = "+91 98765 43210";
                       asha.village_area = "Rampur Ward 4";
                       asha.age = 38;
                       asha.gender = "Female";
                       memory_users_.push_back(asha);

                       User p1;
                       p1.id = 2;
                       p1.username = "patient@gmail.com";
                       p1.email = "patient@gmail.com";
                       p1.password_hash = "patient123";
                       p1.role = "patient";
                       p1.full_name = "Ramesh Kumar";
                       p1.phone = "+91 91234 56789";
                       p1.village_area = "Rampur Ward 4";
                       p1.age = 54;
                       p1.gender = "Male";
                       memory_users_.push_back(p1);
              }

          public:
              DatabaseManager(const std::string& host, int port, const std::string& user, const std::string& password, const std::string& dbname)
               : host_(host), port_(port), user_(user), password_(password), dbname_(dbname) {
                   seed_default_records();
               }
         
               ~DatabaseManager() {
                      if (conn_)  {
                           mysql_close(conn_);
                           conn_ = nullptr;
                      }
               }
              
                std::string escape(const std::string& input) {
                         if(!is_connected_ || !conn_) {
                              std::string res;
                              for (char c : input) {
                                    if (c == '\'') res += "''";
                                     else if (c == '\\') res += "\\\\";
                                     else res += c;
                              }
                              return res;
                         }
                         std::vector<char> to(input.size() * 2+1, 0);
                         mysql_real_escape_string(conn_, to.data(), input.c_str(), input.length());
                         return std::string(to.data());
                }

                 bool init_database() {
                       std::lock_guard<std::mutex> lock(db_mutex_);
                       conn_ = mysql_init(nullptr);
                       if (!conn_) return false;

                      if (!mysql_real_connect(conn_, host_.c_str(), user_.c_str(), password_.c_str(), nullptr, port_, nullptr, 0)) {
                          std::cout << "[DB Module] MySQL deferred (using built-in storage fallback)." << std::endl;
                          is_connected_ = false;
                          return false;
                      }
                   
                      std::string create_db = "CREATE DATABASE IF NOT EXISTS" + dbname_ + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;";
                      mysql_query(conn_, create_db.c_str());
                      mysql_select_db(conn_, dbname_.c_str());

                      std::string sql_users =
                           "CREATE TABLE IF NOT EXISTS users ("
                           "id INT AUTO_INCREMENT PRIMARY KEY,"
                           "username VARCHAR(100) NOT NULL UNIQUE,"
                           "password_hash VARCHAR(255) NOT NULL,"
                           "role ENUM('patient', 'asha_worker') NOT NULL,"
                           "full_name VARCHAR(150) NOT NULL,"
                           "phone VARCHAR(20) NOT NULL,"
                           "village_area VARCHAR(150) NOT NULL,"
                           "age INT DEFAULT 0,"
                           "gender VARCHAR(20) DEFAULT 'NOT Specified',"
                           "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                           ");";
                          mysql_query(conn_, sql_users.c_str(0);

                      std::string sql_assessments =
                          "CREATE TABLE IF NOT EXISTS assessments ("
                          "id INT AUTO_INCREMENT PRIMARY KEY,"
                          "patient_id INT NOT NULL,"
                          "patient_name VARCHAR(150) NOT NULL,"
                          "age INT NOT NULL,"
                          "gender VARCHAR(20) NOT NULL,"
                          "phone VARCHAR(20) NOT NULL,"
                          "village_area VARCHAR(150) NOT NULL,"
                          "bp_systolic INT DEAFULT 120,"
                          "bp_diastolic INT DEFAULT 80,"
                          "blood_sugar INT DEFAULT 95,"
                          "heart_rate INT DEFAULT 72,"
                          "oxygen_level INT DEFAULT 98,"
                          "temperature DECIMAL(4,1) DEFAULT 98.6,"
                          "symptoms_text TEXT,"
                          "medical_history TEXT,"
                          "risk_score INT NOT NULL,"
                          "risk_level VARCHAR(30) NOT NULL,"
                          "possible_chronic_diseases TEXT,"
                          "recommended_doctor VARCHAR(100),"
                          "other_symptoms_to_watch TEXT,"
                          "precautions TEXT,"
                          "immediate_actions TEXT,"
                          "alert_sent TINYINT(1) DEFAULT 0,"
                          "alert_sent_at DATETIME NULL,"
                          "alert_message TEXT NULL,"
                          "asha_contacted TINYINT(1) DEFAULT 0,"
                          "asha_notes TEXT NULL,"
                          "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                          ");";
                       mysql_query(conn_, sql_assessments.c_str());
                        
                       is_connected_ =true;
                       std::cout << "[DB Module] Connected to MySQL database: " << dbname_ << std::endl;
                       return true;
                 }
                 
                 bool is_connected() const { return is_connected_; }

                 bool create_user(const User& user, int& new_id) {
                   std::lock_guard<std::mutex> lock(db_mutex_);
                   if (is_connected_) {
                           std::stringstream ss;
                           ss << "INSERT INTO users (username, password_hash, role, full_name, phone, village_area, age, gender) VALUES ("
                              << "'" << escape(user.username) << "', " << escape(user.password_hash) << "','" << escape(user.role) << "',"
                              << "'" << escape(user.full_name) << "','" << escape(user.phone) << "','" << escape(user.village_area) <<"',"
                              << user.age << ",'" << escape(user.gender) << "');";
                          if (mysql_query(conn_, ss.str().c_str()) ==0) {
                              new_id = (int)mysql_insert_id(conn__;
                              return true;
                          }
                          return false;
                   } else {
                       for (const auto& u : memory_users_) if (u.username == user.username) return false;
                       User nu = user;
                       nu.id = next_user_id_++;
                       nu.created_at = "Just now";
                       memory_users_.push_back(nu);
                       new_id = nu.id;
                       return true;
                   }
                 }

               bool authenticate_user(const std::string& identifier, const std::string& password, User& out_user) {
                    std::lock_guard<std::mutex> lock(db_mutex_);
                    if(is_connected_) {
                        std::string q = "SELECT id, username, password_hash, role, full_name, phone, village_area, age, gender, created_at FROM users WHERE username = '" + escape(identifier) + "' LIMIT 1;";
                        if (mysql_query(conn_, q.c_str()) == 0) {
                             MYSQL_RES* res = mysql_store_result(conn_);
                             if (res) {
                                  MYSQL_ROW* row = mysql_fetch_row(res);
                                  if (row && std::string(row[2] ? row[2] : "") == password) {
                                      out_user.id = std::atoi(row[0]);
                                      out_user.username = row[1] ? row[1] : "";
                                      out_user.email = out_user.username;
                                      out_user.password_hash = row[2] ? row[2] : "";
                                      out_user.role = row[3] ? row[3] : "";
                                      out_user.full_name = row[4] ? row[4] : "";
                                      out_user.phone = row[5] ? row[5] : "";
                                      out_user.village_area = row[6] ? row[6] : "";
                                      out_user.age = row[7] ? std::atoi(row[7]) : 0;
                                      out_user.gender = row[8] ? row[8] : "";
                                      out_user.created_at = row[9] ? row[9] : "";
                                      mysql_free_result(res);
                                      return true;
                                  }
                                   mysql_free_result(res);
                             }
                        }
                        return false;
                    }else {
                        for (const auto& u : memory_users_) {
                          if ((u.username == identifier || u.email == identifier) && u.password_hash == password) {
                            out_user = u;
                           return true;
                          }
                        }
                        return false;
                    }
               }

            bool save_assessment(Assessment& a) {
                   std::lock_guard<std::mutex> lock(db_mutex_);
                   nlohmann::json d_json = a.possible_chronic_diseases;
                   nlohmann::json s_json = a.other_symptoms_to_watch;
                   nlohmann::json p_json = a.precautions;
                          
                     
                                          

            
                     


                         

            

