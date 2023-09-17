# QScript

Toolchain for compiling and decompiling Neversoft QScript.

Compiles:
- Tony Hawk's Underground
- Tony Hawk's Underground 2

Decompiles:
- Tony Hawk's Underground
- Tony Hawk's Underground 2

## Compiling

Standard CMake build process:

```bash
cmake -B build
cmake --build build
```

This will compile the QDecompile library and app.

If you have Flex in your PATH, the QCompile library and app will also be compiled.
