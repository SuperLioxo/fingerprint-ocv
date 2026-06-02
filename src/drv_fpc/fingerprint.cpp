/*
Copyright (C) 2022  pom@vro.life
Copyright (C) 2026  Modified for multi-template FingerCode matching
*/
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <opencv2/imgproc.hpp>

#include "jinx/logging.hpp"

#include "fingerprint.hpp"
#include "crypto.hpp"

namespace fpc {
using namespace std::string_literals;

bool Fingerprint::merge(const cv::Mat& img)
{
    // First image: store directly
    if (_templates.empty()) {
        _templates.push_back(img.clone());
        cv::Mat mask(img.size(), CV_32F);
        mask.setTo(1.0f);
        _masks.push_back(mask);
        _codes.push_back(cvext::FingerCode::extract(img));
        return true;
    }

    // Subsequent images: store as additional template if under limit
    if ((int)_templates.size() >= MAX_TEMPLATES) {
        return false; // Already have enough templates
    }

    // Compute FingerCode for the new image
    cvext::FingerCode new_code = cvext::FingerCode::extract(img);
    if (!new_code.valid()) {
        return false;
    }

    // Check similarity with existing templates (quality gate)
    float min_sim = 1.0f;
    for (const auto& existing_code : _codes) {
        float sim = cvext::FingerCode::similarity(existing_code, new_code);
        if (sim < min_sim) min_sim = sim;
    }

    // If too different from existing templates, reject (probably bad scan)
    // Using a low threshold to be permissive during enrollment
    if (min_sim < -0.2f) {
        std::cout << "  enroll_sim=" << min_sim << " (too low, rejected)" << std::endl;
        return false;
    }

    _templates.push_back(img.clone());
    cv::Mat mask(img.size(), CV_32F);
    mask.setTo(1.0f);
    _masks.push_back(mask);
    _codes.push_back(new_code);

    // Update enrollment baseline
    _enroll_min_similarity = min_sim;

    // Update legacy fields for serialization compatibility
    _fingerprint = _templates[0].clone();
    _mask = _masks[0].clone();

    std::cout << "  enroll_sim=" << min_sim << " templates=" << _templates.size() << std::endl;

    return true;
}

bool Fingerprint::match(const cv::Mat& img, float min_score, bool /*filter*/) const
{
    if (_codes.empty()) {
        return false;
    }

    cvext::FingerCode sample = cvext::FingerCode::extract(img);
    if (!sample.valid()) {
        std::cout << "  fc_sample_invalid" << std::endl;
        return false;
    }

    // Phase 1: FingerCode scoring - find best matching templates
    struct FCResult { size_t idx; float score; };
    std::vector<FCResult> fc_results;
    float best_score = -999.0f;
    for (size_t i = 0; i < _codes.size(); ++i) {
        float score = cvext::FingerCode::similarity(_codes[i], sample);
        std::cout << "  fc[" << i << "]=" << score << std::endl;
        if (score > best_score) {
            best_score = score;
        }
        fc_results.push_back({i, score});
    }

    std::cout << "  fc_best=" << best_score << std::endl;

    // Quick reject: if best FingerCode score is too low, skip expensive SIFT
    if (best_score < min_score) {
        std::cout << "  fc_reject" << std::endl;
        return false;
    }

    // Phase 2: SIFT confirmation against top templates
    // Sort by FingerCode score, try top templates with SIFT
    std::sort(fc_results.begin(), fc_results.end(),
        [](const FCResult& a, const FCResult& b) { return a.score > b.score; });

    for (size_t rank = 0; rank < fc_results.size() && rank < 3; ++rank) {
        size_t tpl_idx = fc_results[rank].idx;
        float fc_score = fc_results[rank].score;

        cv::Mat matrix;
        // Try SIFT alignment between stored template and new scan
        if (cvext::try_align(_templates[tpl_idx], img, matrix)) {
            // SIFT aligned - compute SSIM score
            cv::Mat shadow(img.size(), CV_32F);
            shadow.setTo(1.0f);

            cv::Mat dst_img, dst_mask;
            cv::warpPerspective(img, dst_img, matrix, _templates[tpl_idx].size());
            cv::warpPerspective(shadow, dst_mask, matrix, _templates[tpl_idx].size());

            dst_mask.setTo(0.0F, _masks[tpl_idx] == 0);

            cv::Mat fpr;
            _templates[tpl_idx].copyTo(fpr, dst_mask > 0);

            cv::Scalar ssim = cvext::compute_ssim(fpr, dst_img, dst_mask > 0);

            std::cout << "  sift[" << tpl_idx << "] fc=" << fc_score << " ssim=" << ssim[0] << std::endl;

            // Combined check: FingerCode score + SSIM
            // SSIM threshold 0.8 separates correct (~0.87) from wrong (~0.73)
            if (fc_score >= min_score && ssim[0] >= 0.8f) {
                std::cout << "  CONFIRMED fc=" << fc_score << " ssim=" << ssim[0] << std::endl;
                return true;
            }
        } else {
            std::cout << "  sift[" << tpl_idx << "] fc=" << fc_score << " no_align" << std::endl;
        }
    }

    std::cout << "  rejected_all" << std::endl;
    return false;
}

size_t Fingerprint::total() const
{
    size_t total = 0;
    for (const auto& mask : _masks) {
        auto sum = cv::sum(mask);
        total += static_cast<size_t>(sum[0]);
    }
    return total;
}

void Fingerprint::recompute_codes()
{
    _codes.clear();
    for (const auto& img : _templates) {
        _codes.push_back(cvext::FingerCode::extract(img));
    }
}

void Fingerprint::write(cv::FileStorage& fstorage, int idx) const
{
    std::string name{};

    name = "user"s + std::to_string(idx);
    fstorage << name << _user;

    name = "name"s + std::to_string(idx);
    fstorage << name << _name;

    name = "count"s + std::to_string(idx);
    fstorage << name << (int)_templates.size();

    name = "enroll_min_sim"s + std::to_string(idx);
    fstorage << name << _enroll_min_similarity;

    // Legacy fields for backward compat
    name = "print"s + std::to_string(idx);
    if (!_fingerprint.empty()) {
        fstorage << name << _fingerprint;
    } else if (!_templates.empty()) {
        fstorage << name << _templates[0];
    }

    name = "mask"s + std::to_string(idx);
    if (!_mask.empty()) {
        fstorage << name << _mask;
    } else if (!_masks.empty()) {
        fstorage << name << _masks[0];
    }

    // Store additional templates
    for (int t = 0; t < (int)_templates.size(); ++t) {
        name = "tpl"s + std::to_string(idx) + "_"s + std::to_string(t);
        fstorage << name << _templates[t];
    }
}

void Fingerprint::read(cv::FileStorage& fstorage, int idx)
{
    std::string name{};

    name = "user"s + std::to_string(idx);
    _user = fstorage[name].string();

    name = "name"s + std::to_string(idx);
    _name = fstorage[name].string();

    // Try to read template count (new format)
    name = "count"s + std::to_string(idx);
    int tpl_count = 0;
    if (fstorage[name].isInt()) {
        tpl_count = (int)fstorage[name];
    }

    // Try to read enrollment baseline
    name = "enroll_min_sim"s + std::to_string(idx);
    if (fstorage[name].isReal()) {
        _enroll_min_similarity = (float)fstorage[name];
    }

    // Read legacy single template
    name = "print"s + std::to_string(idx);
    if (!fstorage[name].empty()) {
        _fingerprint = fstorage[name].mat();
    }

    name = "mask"s + std::to_string(idx);
    if (!fstorage[name].empty()) {
        _mask = fstorage[name].mat();
    }

    // Read additional templates (new format)
    _templates.clear();
    _masks.clear();
    _codes.clear();

    if (tpl_count > 0) {
        for (int t = 0; t < tpl_count; ++t) {
            name = "tpl"s + std::to_string(idx) + "_"s + std::to_string(t);
            if (!fstorage[name].empty()) {
                cv::Mat tpl = fstorage[name].mat();
                _templates.push_back(tpl);
                cv::Mat mask(tpl.size(), CV_32F);
                mask.setTo(1.0f);
                _masks.push_back(mask);
            }
        }
    }

    // Fallback: use legacy single template
    if (_templates.empty() && !_fingerprint.empty()) {
        _templates.push_back(_fingerprint.clone());
        if (_mask.empty()) {
            cv::Mat mask(_fingerprint.size(), CV_32F);
            mask.setTo(1.0f);
            _masks.push_back(mask);
        } else {
            _masks.push_back(_mask.clone());
        }
    }

    // Recompute FingerCodes from images
    recompute_codes();
}

void FingerprintStorage::load()
{
    _fingerprints.clear();

    std::filesystem::path filename{_filename};

    if (not std::filesystem::exists(filename)) {
        return;
    }

    size_t filesize = std::filesystem::file_size(filename);

    if (filesize <= 16) {
        return;
    }

    std::vector<unsigned char> encrypted{};
    encrypted.resize(filesize);

    std::vector<unsigned char> data{};
    data.resize(filesize - 16);

    FILE* file = fopen(_filename.c_str(), "rb");
    if (file == nullptr) {
        return;
    }
    fread(encrypted.data(), encrypted.size(), 1, file);
    fclose(file);

    jinx::SliceConst key{_key.data(), _key.size()};
    jinx::SliceConst nonce{_key.data(), 12};

    auto ret = crypto::decrypt(
        EVP_chacha20_poly1305(),
        key,
        nonce,
        key,
        {encrypted.data(), encrypted.size() - 16},
        {data.data(), data.size()},
        {encrypted.data() + encrypted.size() - 16, 16});

    if (not ret) {
        return;
    }

    std::string data_string{reinterpret_cast<char*>(data.data()), data.size()};
    cv::FileStorage fstorage{data_string, cv::FileStorage::READ | cv::FileStorage::MEMORY | cv::FileStorage::FORMAT_JSON};

    auto count = (int)fstorage["count"];

    for (int idx = 0 ; idx < count; ++idx) {
        Fingerprint print{};
        print.read(fstorage, idx);
        insert_or_update(std::move(print));
    }
}

void FingerprintStorage::save()
{
    cv::FileStorage fstorage{"memory", cv::FileStorage::WRITE | cv::FileStorage::MEMORY | cv::FileStorage::FORMAT_JSON};
    int count = 0;
    std::string name{};
    for (auto& user : _fingerprints) {
        for (auto& print : user.second) {
            print.second.write(fstorage, count);
            ++ count;
        }
    }
    fstorage.write("count", count);

    auto data = fstorage.releaseAndGetString();

    std::vector<unsigned char> encrypted{};
    encrypted.resize(data.size() + 16);

    jinx::SliceConst key{_key.data(), _key.size()};
    jinx::SliceConst nonce{_key.data(), 12};

    crypto::encrypt(
        EVP_chacha20_poly1305(),
        key,
        nonce,
        key,
        {data.data(),
        data.size()},
        {encrypted.data(), data.size()},
        {encrypted.data() + data.size(), 16});

    FILE* file = fopen(_filename.c_str(), "wb");
    if (file == nullptr) {
        jinx_log_error() << "write " << _filename << " failed: " << strerror(errno);
    }
    fwrite(encrypted.data(), encrypted.size(), 1, file);
    fclose(file);
}

void FingerprintStorage::init(const std::string &filename, const std::vector<unsigned char>& key)
{
    assert(key.size() == 32);
    _filename = filename;
    _key = key;
    load();
}

void FingerprintStorage::insert_or_update(Fingerprint&& fingerprint)
{
    std::string name = fingerprint._name;
    auto user = _fingerprints.find(fingerprint._user);
    if (user == _fingerprints.end()) {
        auto pair = _fingerprints.emplace(fingerprint._user, std::unordered_map<std::string, Fingerprint>{});
        pair.first->second.emplace(name, std::move(fingerprint));
        return;
    }
    auto print = user->second.find(name);
    if (print == user->second.end()) {
        user->second.emplace(name, std::move(fingerprint));
    } else {
        print->second = std::move(fingerprint);
    }
}

}
