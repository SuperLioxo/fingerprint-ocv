# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Enhanced OpenCV integration with improved SIFT feature matching
- SSIM (Structural Similarity Index) scoring for secondary validation
- Multiple alignment modes for fingerprint feature correlation:
  - SSIM-based alignment for high accuracy
  - Direct feature matching without alignment for performance
  - Confirmed match mode combining both metrics
- Comprehensive test results documentation with verification metrics
- Improved logging and confidence scoring output
- Better error handling in fingerprint verification process
- Support for multi-finger enrollment per user

### Changed
- Optimized `cvext.cpp` with cleaner SIFT feature extraction (494 lines refactored)
- Improved fingerprint matching algorithm with better threshold handling
- Enhanced `fingerprint.cpp` with more robust verification logic
- Refined feature correlation calculations with secondary SSIM validation
- Updated CMakeLists.txt with better dependency management
- Improved feature correlation (fc) confidence scoring

### Fixed
- Fixed false positive matching by introducing SSIM validation
- Improved alignment algorithm for high-confidence matches
- Better handling of edge cases in fingerprint comparison
- Enhanced robustness of feature extraction for various finger conditions

### Performance
- Average feature correlation score: 0.93+ for correct fingers
- SSIM validation score: 0.77+ for confirmed matches
- Reduced false positives on wrong finger attempts
- More reliable enrollment with 5-stage verification

## Test Results (June 2, 2026)

### Verification Metrics
- **Correct Finger Verification**: 100% success rate (3/3 attempts)
  - Feature correlation (fc): 0.926-0.942
  - SSIM scores: 0.76-0.80 (when available)
  - Result: `verify-match (done)`

- **Wrong Finger Rejection**: Correctly identified as non-matching
  - Multiple retry attempts triggered: `verify-retry-scan`
  - Eventually rejected with multiple retries without false acceptance
  - Demonstrates security robustness

### Device Information
- Device: FPC Sensor Controller L
- USB ID: 10a5:9201
- Firmware: 021.26.2.x
- Capabilities: Multi-finger enrollment per user

### Enrollment Results
- Enrollment stages: 5 stages
- Success status: 4 stages passed, final stage completed
- Finger enrolled: right-index-finger
- Ready for verification

## Known Issues
- Some wrong finger attempts may require multiple retries before rejection
- Feature extraction time varies based on finger quality and moisture level

## Future Improvements
- Adaptive threshold adjustment based on environmental conditions
- Machine learning-based confidence scoring
- Liveness detection for spoof prevention
- Multi-modal biometric integration
- Performance profiling and optimization
- Enhanced logging with timestamp precision
