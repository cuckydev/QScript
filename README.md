# QScript

Toolchain for compiling and decompiling Neversoft QScript.

Currently only supports Tony Hawk's Underground. Tony Hawk's Underground 2 support may be added in the future.

## Compiling

Standard CMake build process:

```bash
cmake -B build
cmake --build build
```

This will compile the QDecompile library and app.

If you have Flex in your PATH, the QCompile library and app will also be compiled.
