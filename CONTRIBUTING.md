# Contributing Guidelines

Thank you for your interest in contributing to the fingerprint-ocv project!

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/<your-username>/fingerprint-ocv.git
   cd fingerprint-ocv
   git submodule init
   git submodule update
   ```
3. **Create a feature branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Setup

### Building the Project
```bash
cmake -S . -B build
cmake --build build --parallel $(nproc)
```

### Code Quality Standards

This project uses automated code quality tools:

#### Clang-Tidy
The project includes a `.clang-tidy` configuration file. All C++ code must pass clang-tidy checks:

```bash
# Run clang-tidy on specific file
clang-tidy src/myfile.cpp -- -I. -Ibuild
```

#### Code Style
- Follow the existing code style in the project
- Use the `.editorconfig` file for consistent formatting
- C++ standard: C++17 or later

### Testing

Before submitting a pull request, ensure:

1. **Compilation succeeds** without warnings:
   ```bash
   cmake --build build -- -Wall -Wextra
   ```

2. **Unit tests pass**:
   ```bash
   ctest --test-dir build --verbose
   ```

3. **Manual verification** with real hardware:
   ```bash
   sudo systemctl restart fingerprint-ocv
   fprintd-delete "$USER"
   fprintd-enroll
   fprintd-verify "$USER"  # Should succeed
   ```

## Submitting Changes

### Commit Messages

Follow these conventions for commit messages:

```
type(scope): subject line

Body paragraph explaining the change.
More details about the implementation and reasoning.

Fixes #issue-number (if applicable)
```

**Types:**
- `feat`: A new feature
- `fix`: A bug fix
- `refactor`: Code refactoring without feature changes
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `docs`: Documentation changes
- `chore`: Build configuration, dependencies, etc.

**Examples:**
```
feat(fingerprint): add SSIM-based verification

Implement SSIM scoring for secondary validation of fingerprint matches.
This reduces false positives while maintaining high accuracy.

Fixes #42
```

```
fix(cvext): improve feature extraction robustness

Handle edge cases in SIFT feature detection when fingerprint image quality is poor.
```

### Pull Request Process

1. **Update CHANGELOG.md** with your changes:
   ```markdown
   ### Added
   - Your new feature description

   ### Fixed
   - Bug fix description
   ```

2. **Test thoroughly** on supported platforms:
   - Ubuntu 20.04 LTS (Focal)
   - Ubuntu 22.04 LTS (Jammy)
   - Archlinux
   - Fedora 38+

3. **Create a Pull Request**:
   - Clear description of changes
   - Reference related issues
   - Screenshots or test results if applicable
   - Link to any related documentation

4. **Address review feedback**:
   - Respond to reviewer comments
   - Push additional commits to address issues
   - Don't force-push after review has started (unless requested)

## Areas for Contribution

### High Priority
- [ ] Improve fingerprint matching accuracy
- [ ] Add liveness detection to prevent spoofing
- [ ] Optimize performance for slower devices
- [ ] Add comprehensive error messages
- [ ] Improve documentation with more examples

### Medium Priority
- [ ] Add support for additional fingerprint sensors
- [ ] Implement confidence score calibration
- [ ] Create Python bindings for easier integration
- [ ] Add monitoring/metrics support
- [ ] Improve logging and debugging capabilities

### Documentation
- Tutorial articles
- API documentation
- Hardware setup guides
- Troubleshooting guides
- Integration examples

## Reporting Issues

When reporting bugs, include:

1. **System information**:
   ```bash
   uname -a
   lsusb | grep 10a5:9201
   ldd /usr/local/bin/fingerprint-ocv | grep -E "opencv|libusb"
   ```

2. **Steps to reproduce**:
   - Clear instructions to reproduce the issue
   - Expected vs actual behavior

3. **Logs and output**:
   ```bash
   journalctl -u fingerprint-ocv -n 50
   fprintd-verify "$USER" 2>&1
   ```

4. **Verification results** (if applicable):
   - Whether correct fingers are recognized
   - Whether wrong fingers are rejected
   - Confidence scores if available

## Code Review Process

- All changes require at least one approval before merging
- Maintainers will review for:
  - Code quality and consistency
  - Performance implications
  - Security considerations
  - Documentation completeness
  - Compatibility with supported platforms

## Recognition

Contributors will be recognized in:
- GitHub contributors page
- Project CHANGELOG
- Project README (for significant contributions)

## Questions?

- Open a GitHub discussion
- Check existing issues and pull requests
- Review the documentation in `/docs` folder

Thank you for contributing to fingerprint-ocv!
