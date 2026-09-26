# Skill: Build and Test

## Command Procedure
```bash
# 1. Debug build and test
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure

# 2. Clang-format check
cmake --build --preset debug --target format-check

# 3. Sanitizer validation (ASan + UBSan)
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure

# 4. ThreadSanitizer validation (TSan)
cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan --output-on-failure
```
