/*
Copyright (C) 2022  pom@vro.life
Copyright (C) 2026  Modified for FingerCode matching engine

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef __cvext_hpp__
#define __cvext_hpp__

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace cvext {

template<typename T>
void gamma(const cv::Mat& input, cv::Mat& output, double gamma_val)
{
    for (size_t row = 0; row < input.rows; ++row) {
        for (size_t col = 0; col < input.cols; ++col) {
            auto val = static_cast<double>(input.at<T>(row, col)) / 255;
            val = ::pow(static_cast<double>(val), gamma_val);
            output.at<T>(row, col) = static_cast<T>(val * 255);
        }
    }
}

// ============================================================
// FingerCode: Gabor Filterbank fingerprint feature extraction
// ============================================================

struct FingerCode {
    static constexpr int NUM_BLOCKS = 8;
    static constexpr int NUM_ORIENTATIONS = 8;
    static constexpr int FEATURE_SIZE = NUM_BLOCKS * NUM_BLOCKS * NUM_ORIENTATIONS;

    std::vector<float> features;

    FingerCode() : features(FEATURE_SIZE, 0.0f) {}

    // Extract FingerCode from preprocessed grayscale image
    static FingerCode extract(const cv::Mat& img);

    // Cosine similarity [-1, 1]
    static float similarity(const FingerCode& a, const FingerCode& b);

    bool valid() const;
};

// Multi-template match result
struct MatchResult {
    bool matched;
    float best_score;
    int best_template_index;
};

// Match sample against multiple stored FingerCode templates
MatchResult match_fingercode(
    const std::vector<FingerCode>& templates,
    const FingerCode& sample,
    float threshold
);

// SIFT-based alignment between two images (for dual verification)
bool try_align(const cv::Mat& template_img, const cv::Mat& sample_img, cv::Mat& matrix);

// SSIM computation with mask
cv::Scalar compute_ssim(const cv::Mat& img1, const cv::Mat& img2, cv::InputArray mask);

// Legacy merge - kept for build compatibility, simplified
bool merge(const cv::Mat& img1, const cv::Mat& mask1, const cv::Mat& img2,
           cv::Mat& output, cv::Mat& output_mask);

// Legacy match - kept for build compatibility
bool match(const cv::Mat& fingerprint, const cv::Mat& fp_mask,
           const cv::Mat& partial, int min_match, double min_score, bool filter);

}

#endif
