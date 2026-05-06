# sha256.cpp

A fast, cross-platform SHA‑256 checksum utility written in modern C++20.  
Compute file hashes, generate directory checksums, and verify existing checksum files — with ANSI‑colored output and robust symlink handling.

## Features

- **Single file hash** – compute SHA‑256 of any file
- **Directory checksums** – recursively scan a folder and generate a `checksum.txt`
- **Verification** – verify files against a `checksum.txt` (compatible with standard `sha256sum` format)
- **Symlink‑aware** – resolves symlinks safely and skips targets outside the base directory
- **Cross‑platform** – Windows, Linux, macOS
- **Colored output** – OK/Missing/BAD status shown in green/red on terminals that support ANSI codes

## Usage

```bash
# Hash a single file
sha256 file.txt

# Generate checksums for a directory
sha256 my_folder

# Verify checksums from a checksum file
sha256 -c checksum.txt
```

When generating checksums, the tool creates `checksum.txt` inside the target directory. Each line follows the format:

```
<hash> *<relative_path>
```

## Building from source

### Requirements
- CMake ≥ 3.15
- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 14+)

### Build & install
```bash
git clone https://github.com/yourusername/sha256.cpp.git
cd sha256.cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The executable `sha256` (or `sha256.exe` on Windows) will be in the `build/Release` folder. Optionally move it to a directory in your PATH.

## Verification format

The generated checksum file is compatible with the standard `sha256sum` format:

```
a1b2c3d4... *relative/path/to/file
```

You can verify files using `sha256 -c checksum.txt`. The tool checks that each referenced file exists, is inside the base directory, and matches the expected hash.

## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/Platerman/sha256-cpp/blob/main/LICENSE) file for details.
