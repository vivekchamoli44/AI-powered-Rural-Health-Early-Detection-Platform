#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <chrono>
#include "crow_all.h"
#include "models.h"
#include "db_history.h"
#include "auth_notification.h"
#include "nlohmann/json.h"

namespace health {


struct SlotInterval {
    int start;          
    int end;            
    int booking_id;     
    std::string patient_name;
    std::string test_name;

    SlotInterval(int s = 0, int e = 0, int b_id = 0, const std::string& p_name = "", const std::string& t_name = "")
        : start(s), end(e), booking_id(b_id), patient_name(p_name), test_name(t_name) {}

    // Overlap condition: max(start1, start2) < min(end1, end2)
    bool overlaps_with(const SlotInterval& other) const {
        return std::max(start, other.start) < std::min(end, other.end);
    }

    std::string format_time_label() const {
        auto format_m = [](int mins) {
            int h = mins / 60;
            int m = mins % 60;
            std::string period = (h >= 12) ? "PM" : "AM";
            int dh = (h > 12) ? (h - 12) : ((h == 0) ? 12 : h);
            char b[16];
            snprintf(b, sizeof(b), "%02d:%02d %s", dh, m, period.c_str());
            return std::string(b);
        };
        return format_m(start) + " - " + format_m(end);
    }
};

struct IntervalTreeNode {
    SlotInterval interval;
    int max_end;               
    int height;                
    IntervalTreeNode* left;
    IntervalTreeNode* right;

    IntervalTreeNode(const SlotInterval& iv)
        : interval(iv), max_end(iv.end), height(1), left(nullptr), right(nullptr) {}
};

class LabSlotIntervalTree {
private:
    IntervalTreeNode* root_ = nullptr;
    int count_ = 0;

    int get_h(IntervalTreeNode* n) const { return n ? n->height : 0; }
    int get_max(IntervalTreeNode* n) const { return n ? n->max_end : 0; }

    void update(IntervalTreeNode* n) {
        if (!n) return;
        n->height = 1 + std::max(get_h(n->left), get_h(n->right));
        n->max_end = std::max({n->interval.end, get_max(n->left), get_max(n->right)});
    }

    int get_balance(IntervalTreeNode* n) const {
        return n ? (get_h(n->left) - get_h(n->right)) : 0;
    }

    IntervalTreeNode* rot_right(IntervalTreeNode* y) {
        IntervalTreeNode* x = y->left;
        IntervalTreeNode* T2 = x->right;
        x->right = y;
        y->left = T2;
        update(y);
        update(x);
        return x;
    }

    IntervalTreeNode* rot_left(IntervalTreeNode* x) {
        IntervalTreeNode* y = x->right;
        IntervalTreeNode* T2 = y->left;
        y->left = x;
        x->right = T2;
        update(x);
        update(y);
        return y;
    }

    IntervalTreeNode* insert_rec(IntervalTreeNode* node, const SlotInterval& iv) {
        if (!node) {
            count_++;
            return new IntervalTreeNode(iv);
        }

        if (iv.start < node->interval.start) {
            node->left = insert_rec(node->left, iv);
        } else {
            node->right = insert_rec(node->right, iv);
        }

        update(node);

        int balance = get_balance(node);
        // LL
        if (balance > 1 && iv.start < node->left->interval.start)
            return rot_right(node);
        // RR
        if (balance < -1 && iv.start >= node->right->interval.start)
            return rot_left(node);
        // LR
        if (balance > 1 && iv.start >= node->left->interval.start) {
            node->left = rot_left(node->left);
            return rot_right(node);
        }
        // RL
        if (balance < -1 && iv.start < node->right->interval.start) {
            node->right = rot_right(node->right);
            return rot_left(node);
        }

        return node;
    }

    IntervalTreeNode* search_overlap(IntervalTreeNode* node, const SlotInterval& target) const {
        if (!node) return nullptr;

        if (node->interval.overlaps_with(target)) {
            return node;
        }
        if (node->left && node->left->max_end > target.start) {
            IntervalTreeNode* res = search_overlap(node->left, target);
            if (res) return res;
        }
        return search_overlap(node->right, target);
    }
    void free_all(IntervalTreeNode* node) {
        if (!node) return;
        free_all(node->left);
        free_all(node->right);
        delete node;
    }

public:
    LabSlotIntervalTree() : root_(nullptr), count_(0) {}
    ~LabSlotIntervalTree() { free_all(root_); }

    LabSlotIntervalTree(const LabSlotIntervalTree&) = delete;
    LabSlotIntervalTree& operator=(const LabSlotIntervalTree&) = delete;

    void clear() {
        free_all(root_);
        root_ = nullptr;
        count_ = 0;
    }
    int size() const { return count_; }
    bool has_conflict(int start_m, int end_m, SlotInterval& out_conflicting) const {
        SlotInterval req(start_m, end_m);
        IntervalTreeNode* hit = search_overlap(root_, req);
        if (hit) {
            out_conflicting = hit->interval;
            return true;
        }
        return false;
    }

    // O(log N) insert
    bool book_slot(int start_m, int end_m, int booking_id, const std::string& p_name, const std::string& t_name, std::string& err_msg) {
        SlotInterval conflict;
        if (has_conflict(start_m, end_m, conflict)) {
            err_msg = "Slot conflict! Already booked from " + conflict.format_time_label() +
                      " (" + conflict.test_name + ") for " + conflict.patient_name;
            return false;
        }
        root_ = insert_rec(root_, SlotInterval(start_m, end_m, booking_id, p_name, t_name));
        return true;
    }
};
