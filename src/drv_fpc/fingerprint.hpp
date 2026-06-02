/*
Copyright (C) 2022  pom@vro.life
Copyright (C) 2026  Modified for multi-template FingerCode storage

Multi-template fingerprint storage with FingerCode matching.
*/
#ifndef __fingerprint_hpp__
#define __fingerprint_hpp__

#include <string>
#include <vector>
#include <unordered_map>

#include "cvext.hpp"

namespace fpc {

// Maximum number of individual templates to store per finger
static constexpr int MAX_TEMPLATES = 5;

struct Fingerprint
{
    std::string _user{};
    std::string _name{};

    // Multi-template storage
    std::vector<cv::Mat> _templates{};              // Individual scan images
    std::vector<cv::Mat> _masks{};                   // Masks for each template
    std::vector<cvext::FingerCode> _codes{};          // FingerCode for each template

    // Legacy fields kept for serialization compatibility
    cv::Mat _fingerprint{};
    cv::Mat _mask{};

    // Enrollment calibration
    float _enroll_min_similarity{0.0f}; // Min pairwise similarity among enrolled templates

    Fingerprint() = default;
    Fingerprint(Fingerprint&&) = default;
    Fingerprint(const Fingerprint&) = default;
    ~Fingerprint() = default;

    Fingerprint& operator =(Fingerprint&&) = default;
    Fingerprint& operator =(const Fingerprint&) = default;

    // Add a new scan image as a template
    // Returns true if this was the first image (no merge needed)
    bool merge(const cv::Mat& img);

    // Match a new scan against all stored FingerCode templates
    bool match(const cv::Mat& img, float min_score, bool filter) const;

    // Total coverage (sum of mask pixels across all templates)
    size_t total() const;

    // Number of stored templates
    size_t count() const { return _templates.size(); }

    // Recompute all FingerCodes from stored images
    void recompute_codes();

    void write(cv::FileStorage& fstorage, int idx) const;
    void read(cv::FileStorage& fstorage, int idx);

    static bool is_any(const std::string& name) {
        return name.empty() or name == "any";
    }
};

class FingerprintStorage
{
    std::string _filename{};
    std::vector<unsigned char> _key{};
    std::unordered_map<std::string, std::unordered_map<std::string, Fingerprint>> _fingerprints{};

public:
    template<typename F>
    void foreach(const std::string& username, F&& fun) {
        auto user = _fingerprints.find(username);
        if (user == _fingerprints.end()) {
            return;
        }

        for (auto& pair : user->second) {
            auto ret = fun(pair.second);
            if (ret) {
                return;
            }
        }
    }

    size_t get_enrolled_count(const std::string& username) {
        auto user = _fingerprints.find(username);
        if (user == _fingerprints.end()) {
            return 0;
        }
        return user->second.size();
    }

    void delete_all(const std::string& username)
    {
        auto user = _fingerprints.find(username);
        if (user == _fingerprints.end()) {
            return;
        }
        user->second.clear();
    }

    bool delete_fingerprint(const std::string& username, const std::string& name)
    {
        auto user = _fingerprints.find(username);
        if (user == _fingerprints.end()) {
            return false;
        }

        auto print = user->second.find(name);
        if (print != user->second.end()) {
            user->second.erase(print);
            return true;
        }

        return false;
    }

    bool check(const std::string& username, const std::string& name)
    {
        auto user = _fingerprints.find(username);
        if (user == _fingerprints.end()) {
            return false;
        }

        auto print = user->second.find(name);
        return print != user->second.end();
    }

    void insert_or_update(Fingerprint&& fingerprint);

    void load();
    void save();

    void reset() {
        _filename.clear();
        _key.clear();
        _fingerprints.clear();
    }

    void init(const std::string& filename, const std::vector<unsigned char>& key);
};

}

#endif
