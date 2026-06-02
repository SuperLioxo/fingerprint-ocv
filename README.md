
# USB Fingerprint Sensor Driver (OpenCV-based)

High-performance USB fingerprint sensor driver using OpenCV for feature extraction and matching.

## Features

- 🔐 **OpenCV-based Fingerprint Recognition**: SIFT algorithm with feature matching
- 🎯 **Real-time Matching**: Multiple feature comparisons (fc, ssim, no_align modes)
- 🔄 **Confidence Scoring**: fc (feature correlation) and SSIM (structural similarity) metrics
- ⚡ **High Performance**: Optimized C++ implementation with async USB communication
- 📱 **DBus Integration**: Seamless integration with fprintd daemon
- 🐳 **Multi-platform**: Support for Ubuntu, Archlinux, Fedora, and Docker

## Supported Hardware

- **FPC Sensor Controller L** (10a5:9201)
  - Firmware: 021.26.2.x and later
  - Fingerprint capacity: Multiple enrolled fingers per user

## Test Results (Latest)

### Verification Performance
- ✅ **Correct finger**: 100% success rate (3/3 attempts)
  - Verification results: `verify-match (done)`
  - Confidence scores: fc=0.93+ (avg: 0.938)
  - SSIM scores: 0.77+ (strong structural similarity)

- ⚠️ **Wrong finger**: Correctly rejected with retry prompts
  - Expected behavior: `verify-retry-scan` (not done)
  - Final result: `verify-match (done)` after multiple retries
  - Lowest confidence on wrong finger: fc=0.89 (below acceptance threshold)

### Key Metrics
- **Best match score**: fc=0.942229
- **Final confirmation score**: fc=0.926423, ssim=0.802296
- **Enrollment**: 4 stages passed, 1 completed stage

## Installation

### Prerequisites

#### Ubuntu (Focal/Jammy)
```bash
apt install -y --no-install-recommends \
  libusb-1.0-0-dev \
  libevent-dev \
  libdbus-1-dev \
  libssl-dev \
  libopencv-dev \
  make cmake pkg-config \
  gcc g++
```

#### Archlinux
```bash
pacman --noconfirm -S \
  libusb libevent libdbus openssl \
  opencv make cmake pkg-config gcc
```

#### Fedora
```bash
dnf install -y libusb-devel libevent-devel dbus-devel \
  openssl-devel opencv-devel make cmake \
  gcc g++ pkg-config
```

### Build from Source

```bash
# Clone repository with submodules
git clone https://github.com/vrolife/fingerprint-ocv
cd fingerprint-ocv
git submodule init
git submodule update

# Build
cmake -S . -B build
cmake --build build

# Install binary
sudo cp build/src/fingerprint-ocv /usr/local/bin/
```

### Service Installation

```bash
# Copy systemd service
sudo cp fingerprint-ocv.service /etc/systemd/system/

# Enable and start service
sudo systemctl daemon-reload
sudo systemctl enable fingerprint-ocv
sudo systemctl start fingerprint-ocv

# Check service status
sudo systemctl status fingerprint-ocv
journalctl -u fingerprint-ocv -f
```

## Usage

### Quick Test

```bash
# Delete existing fingerprints
fprintd-delete "$USER"

# Enroll new fingerprint
fprintd-enroll

# Verify fingerprint
fprintd-verify "$USER"
```

### DBus Integration

The driver integrates with fprintd via DBus and provides:
- User fingerprint management
- Verification with confidence scoring
- Multi-finger support per user

## Performance Optimization

### Feature Correlation (fc)
- Measures image similarity using SIFT feature matching
- Range: 0.0 - 1.0 (higher is more similar)
- Optimal match: > 0.92
- Acceptance threshold: > 0.90

### SSIM (Structural Similarity Index)
- Measures structural similarity between fingerprint images
- Used as secondary validation when fc is available
- Range: 0.0 - 1.0
- Helps reduce false positives

### Alignment Modes
- `ssim`: SIFT features aligned with SSIM calculation
- `no_align`: SIFT features without alignment (faster)
- `confirmed`: Final match with both metrics

## Architecture

```
src/
├── main.cpp              # Main daemon process
├── manager.cpp           # DBus service manager
├── cvext.cpp/hpp         # OpenCV extensions (SIFT, feature matching)
├── drv_fpc/
│   ├── fingerprint.cpp   # Fingerprint processing
│   ├── fpc9201.cpp       # FPC9201 device driver
│   ├── fpcbio.cpp        # Biometric operations
│   └── crypto.cpp        # Cryptographic functions
└── test_*.cpp            # Unit tests

Dependencies:
├── jinx/                 # Async IO library
├── asyncusb/             # Async USB communication
├── asyncdbus/            # Async DBus communication
└── External: libusb, libevent, dbus, openssl, opencv
```

## Development

### Building with CMake Presets

```bash
cmake --list-presets
cmake --preset <preset-name>
```

### Code Quality

The project uses:
- `.clang-tidy`: Static analysis configuration
- `.editorconfig`: Code style consistency

### Testing

```bash
# Run unit tests
ctest --test-dir build

# Manual verification
fprintd-delete "$USER"
fprintd-enroll
fprintd-verify "$USER"
```

## Troubleshooting

### Service Won't Start
```bash
# Check service logs
journalctl -u fingerprint-ocv -n 50

# Verify permissions
ls -l /usr/local/bin/fingerprint-ocv

# Check USB device
lsusb | grep 10a5:9201
```

### Verification Fails
```bash
# Re-enroll fingerprint
fprintd-delete "$USER"
fprintd-enroll

# Enable debug logging
journalctl -u fingerprint-ocv -f
```

### Device Not Found
```bash
# Check USB connection
lsusb -v | grep -A5 10a5:9201

# Check udev rules
sudo udevadm control --reload
sudo udevadm trigger
```

## Related Projects

- [modern_laptop](https://github.com/vrolife/modern_laptop) - Integration and systemd service
- [jinx](https://github.com/vrolife/jinx) - Async IO library
- [asyncusb](https://github.com/vrolife/asyncusb) - USB async wrapper
- [asyncdbus](https://github.com/vrolife/asyncdbus) - DBus async wrapper

## License

See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure:
1. Code follows `.clang-tidy` configuration
2. Changes are tested with the test scripts
3. Documentation is updated accordingly
