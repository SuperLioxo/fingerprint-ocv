/*
Copyright (C) 2022  pom@vro.life
Copyright (C) 2026  Modified for FingerCode matching engine

FingerCode extraction using Gabor Filterbank.
Based on: Jain et al. "Filterbank-Based Fingerprint Matching" (2000)

Adapted for low-resolution (112x88) FPC fingerprint sensors.
*/
#include <iostream>
#include <cmath>
#include <numeric>
#include <algorithm>

#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>

#include "cvext.hpp"

namespace cvext {

// ============================================================
// Gabor filter generation
// ============================================================

static cv::Mat create_gabor_kernel(int ksize, double sigma, double theta, double lambda, double gamma)
{
    return cv::getGaborKernel(cv::Size(ksize, ksize), sigma, theta, lambda, gamma, 0, CV_32F);
}

// ============================================================
// FingerCode extraction
// ============================================================

FingerCode FingerCode::extract(const cv::Mat& img)
{
    FingerCode fc;

    if (img.empty() || img.rows < 16 || img.cols < 16) {
        return fc;
    }

    // Ensure grayscale
    cv::Mat gray;
    if (img.channels() > 1) {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = img.clone();
    }

    // Normalize and denoise
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);

    // Resize to standard size for block division
    // Target: large enough for NUM_BLOCKS blocks with meaningful content
    const int target_size = NUM_BLOCKS * 16; // 128 pixels
    cv::Mat resized;
    cv::resize(blurred, resized, cv::Size(target_size, target_size));

    // Block size in pixels
    const int block_w = target_size / NUM_BLOCKS; // 16 pixels per block
    const int block_h = target_size / NUM_BLOCKS;

    // Generate Gabor filters at NUM_ORIENTATIONS orientations
    const double sigma = 3.0;
    const double lambda = 10.0;
    const double gabor_gamma = 0.5;
    const int ksize = 9;

    std::array<cv::Mat, NUM_ORIENTATIONS> filters{};
    for (int i = 0; i < NUM_ORIENTATIONS; ++i) {
        double theta = CV_PI * i / NUM_ORIENTATIONS;
        filters[i] = create_gabor_kernel(ksize, sigma, theta, lambda, gabor_gamma);
    }

    // Extract features: for each block, apply each Gabor filter, compute std dev
    int feature_idx = 0;
    for (int by = 0; by < NUM_BLOCKS; ++by) {
        for (int bx = 0; bx < NUM_BLOCKS; ++bx) {
            // Extract block region
            cv::Rect block_roi(bx * block_w, by * block_h, block_w, block_h);
            cv::Mat block = resized(block_roi);

            for (int ori = 0; ori < NUM_ORIENTATIONS; ++ori) {
                // Apply Gabor filter
                cv::Mat filtered;
                cv::filter2D(block, filtered, CV_32F, filters[ori]);

                // Compute standard deviation of filtered response
                cv::Scalar mean, stddev;
                cv::meanStdDev(filtered, mean, stddev);

                fc.features[feature_idx++] = static_cast<float>(stddev[0]);
            }
        }
    }

    // Normalize feature vector to unit length
    float norm = 0.0f;
    for (float f : fc.features) {
        norm += f * f;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (float& f : fc.features) {
            f /= norm;
        }
    }

    return fc;
}

float FingerCode::similarity(const FingerCode& a, const FingerCode& b)
{
    if (a.features.size() != b.features.size()) return 0.0f;

    // Dot product (features are already normalized to unit length)
    float dot = 0.0f;
    for (size_t i = 0; i < a.features.size(); ++i) {
        dot += a.features[i] * b.features[i];
    }

    return dot; // Already in [-1, 1] due to normalization
}

bool FingerCode::valid() const
{
    float sum = 0.0f;
    for (float f : features) {
        sum += std::abs(f);
    }
    return sum > 0.01f;
}

// ============================================================
// Multi-template matching
// ============================================================

MatchResult match_fingercode(
    const std::vector<FingerCode>& templates,
    const FingerCode& sample,
    float threshold)
{
    MatchResult result{false, 0.0f, -1};

    if (!sample.valid() || templates.empty()) {
        return result;
    }

    float best_score = -999.0f;
    int best_idx = -1;

    for (int i = 0; i < (int)templates.size(); ++i) {
        if (!templates[i].valid()) continue;

        float score = FingerCode::similarity(templates[i], sample);

        std::cout << "  fc_sim[" << i << "]=" << score << std::endl;

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    result.best_score = best_score;
    result.best_template_index = best_idx;
    result.matched = (best_score >= threshold);

    std::cout << "  fc_best=" << best_score << " threshold=" << threshold
              << " result=" << (result.matched ? "MATCH" : "reject") << std::endl;

    return result;
}

// ============================================================
// SIFT-based alignment for dual verification
// ============================================================

bool try_align(const cv::Mat& template_img, const cv::Mat& sample_img, cv::Mat& matrix)
{
    if (template_img.empty() || sample_img.empty()) return false;

    auto sift = cv::SIFT::create(0, 3, 0.04, 0.1);

    std::vector<cv::KeyPoint> kp1{}, kp2{};
    cv::Mat desc1{}, desc2{};
    sift->detectAndCompute(template_img, {}, kp1, desc1);
    sift->detectAndCompute(sample_img, {}, kp2, desc2);

    if (kp1.empty() || kp2.empty() || desc1.empty() || desc2.empty()) return false;

    auto bf = cv::BFMatcher::create(cv::NORM_L2);
    std::vector<std::vector<cv::DMatch>> matches{};
    bf->knnMatch(desc1, desc2, matches, 2);

    std::vector<std::pair<size_t, size_t>> good{};
    for (auto& vpair : matches) {
        if (vpair.size() < 2) continue;
        if (vpair[0].distance < 0.65f * vpair[1].distance) {
            good.emplace_back(vpair[0].queryIdx, vpair[0].trainIdx);
        }
    }

    if (good.size() < 4) return false;

    std::vector<cv::Point2f> pts1{}, pts2{};
    for (auto& [qi, ti] : good) {
        pts1.push_back(kp1[qi].pt);
        pts2.push_back(kp2[ti].pt);
    }

    matrix = cv::findHomography(pts2, pts1, cv::RANSAC, 4);
    return !matrix.empty();
}

// ============================================================
// SSIM with mask
// ============================================================

cv::Scalar compute_ssim(const cv::Mat& img1, const cv::Mat& img2, cv::InputArray mask)
{
    const double C1 = 6.5025;
    const double C2 = 58.5225;

    cv::Mat i1, i2;
    img1.convertTo(i1, CV_32F);
    img2.convertTo(i2, CV_32F);

    cv::Mat i1_2 = i1.mul(i1);
    cv::Mat i2_2 = i2.mul(i2);
    cv::Mat i12  = i1.mul(i2);

    cv::Mat mu1{}, mu2{};
    cv::GaussianBlur(i1, mu1, {11, 11}, 1.5);
    cv::GaussianBlur(i2, mu2, {11, 11}, 1.5);

    cv::Mat mu1_2 = mu1.mul(mu1);
    cv::Mat mu2_2 = mu2.mul(mu2);
    cv::Mat mu12  = mu1.mul(mu2);

    cv::Mat s1_2{}, s2_2{}, s12{};
    cv::GaussianBlur(i1_2, s1_2, {11, 11}, 1.5);
    cv::GaussianBlur(i2_2, s2_2, {11, 11}, 1.5);
    cv::GaussianBlur(i12,  s12,  {11, 11}, 1.5);

    s1_2 -= mu1_2;
    s2_2 -= mu2_2;
    s12  -= mu12;

    auto t1 = 2.0 * i12 + C1;
    auto t2 = 2.0 * s12 + C2;
    auto t3 = t1.mul(t2);

    t1 = i1_2 + i2_2 + C1;
    auto t4 = s1_2 + s2_2 + C2;
    t1 = t1.mul(t4);

    cv::Mat ssim_map{};
    cv::divide(t3, t1, ssim_map);

    return cv::mean(ssim_map, mask);
}

// ============================================================
// Legacy functions (kept for build compatibility)
// ============================================================

bool merge(const cv::Mat& /*img1*/, const cv::Mat& /*mask1*/,
           const cv::Mat& /*img2*/, cv::Mat& /*output*/, cv::Mat& /*output_mask*/)
{
    return true;
}

bool match(const cv::Mat& /*fingerprint*/, const cv::Mat& /*fp_mask*/,
           const cv::Mat& /*partial*/, int /*min_match*/, double /*min_score*/, bool /*filter*/)
{
    return false;
}

} // namespace cvext
