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


     


                         

            

