# CMake Options Guide

## Available Options

### Debug Options

#### `SONIC_ENABLE_DEBUG`
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable debug features and symbols
- **Effects**:
  - Adds `DEBUG` preprocessor definition
  - Enables additional debug symbols (g3 on GCC/Clang, /Zi on MSVC)
  - Useful for development and debugging

#### `SONIC_ENABLE_ASAN`
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable AddressSanitizer for memory error detection
- **Effects**:
  - Adds `-fsanitize=address` flag (GCC/Clang)
  - Adds `/fsanitize=address` flag (MSVC)
  - Enables runtime memory error detection
  - Useful for finding memory bugs and leaks

#### `SONIC_STATIC_ANALYSIS`
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable static analysis tools
- **Effects**:
  - Adds `-fanalyzer` flag (GCC/Clang)
  - Adds `/analyze` flag (MSVC)
  - Enables compile-time static analysis
  - Useful for code quality and security

## Usage Examples

### Development Build with All Features
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=ON \
      -DSONIC_ENABLE_ASAN=ON \
      -DSONIC_STATIC_ANALYSIS=ON \
      ..
cmake --build . --config Debug
```

### Production Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSONIC_ENABLE_DEBUG=OFF \
      -DSONIC_ENABLE_ASAN=OFF \
      -DSONIC_STATIC_ANALYSIS=OFF \
      ..
cmake --build . --config Release
```

### Debug Build with AddressSanitizer Only
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=OFF \
      -DSONIC_ENABLE_ASAN=ON \
      -DSONIC_STATIC_ANALYSIS=OFF \
      ..
cmake --build . --config Debug
```

### Static Analysis Only
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=OFF \
      -DSONIC_ENABLE_ASAN=OFF \
      -DSONIC_STATIC_ANALYSIS=ON \
      ..
cmake --build . --config Debug
```

## Output Information

When configuring, CMake will display the current settings:

```
SONiC Mini Switch - Celero Benchmarks Configuration:
  Version: 1.0.0
  Build Type: Debug
  Compiler: MSVC

Build Options:
  C++ Standard: 20
  Threads: TRUE
  Celero: 0
  Debug Features: ON
  AddressSanitizer: ON
  Static Analysis: ON
```

## Performance Impact

### `SONIC_ENABLE_DEBUG`
- **Performance**: Minimal impact
- **Binary Size**: Increased due to debug symbols
- **Use Case**: Development and debugging

### `SONIC_ENABLE_ASAN`
- **Performance**: Significant runtime overhead (2x-10x slower)
- **Binary Size**: Increased due to sanitizer runtime
- **Use Case**: Memory error detection during testing

### `SONIC_STATIC_ANALYSIS`
- **Performance**: No runtime impact (compile-time only)
- **Binary Size**: No impact
- **Use Case**: Code quality and security analysis

## Best Practices

1. **Development**: Use `SONIC_ENABLE_DEBUG=ON` for regular development
2. **Testing**: Use `SONIC_ENABLE_ASAN=ON` for testing and debugging
3. **Code Review**: Use `SONIC_STATIC_ANALYSIS=ON` for code quality checks
4. **Production**: Set all options to OFF for optimal performance

## Troubleshooting

### AddressSanitizer Issues
- Ensure you have a recent compiler version
- On Windows, use MSVC 2019 or later
- Some platforms may require additional runtime libraries

### Static Analysis Issues
- GCC 10+ or Clang 10+ recommended for best results
- MSVC 2019+ required on Windows
- Some analysis features may be compiler-specific

### Debug Symbol Issues
- Ensure sufficient disk space for debug symbols
- Use appropriate tools for your platform (gdb, lldb, Visual Studio debugger)

## Integration with IDEs

### Visual Studio
```cmake
# Configure for Visual Studio
cmake -G "Visual Studio 17 2022" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSONIC_ENABLE_DEBUG=ON \
      -DSONIC_ENABLE_ASAN=ON \
      ..
```

### VS Code
```json
{
    "cmake.configureArgs": [
        "-DSONIC_ENABLE_DEBUG=ON",
        "-DSONIC_ENABLE_ASAN=ON",
        "-DSONIC_STATIC_ANALYSIS=ON"
    ]
}
```

### CLion
Add to CMake options in Settings/Preferences:
```
-DSONIC_ENABLE_DEBUG=ON -DSONIC_ENABLE_ASAN=ON -DSONIC_STATIC_ANALYSIS=ON
```
