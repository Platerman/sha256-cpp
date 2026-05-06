cmake_minimum_required(VERSION 3.15)
project(sha256 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sha256
    src/main.cpp
    src/sha256.cpp
    src/verify.cpp
    src/generate.cpp
    src/utils.cpp
    resource/version.rc
)

target_include_directories(sha256 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(sha256 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /EHsc
    >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:
        -O3
        -DNDEBUG
        -flto=auto
        -march=native
    >
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:
        -O3
        -DNDEBUG
        -flto=thin
        -march=native
    >
)

target_compile_options(sha256 PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /O2
        /GL
        /Oi
        /Ot
        /Gy
        /Gw
    >
)

target_compile_definitions(sha256 PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    _CRT_SECURE_NO_WARNINGS
)

target_link_options(sha256 PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /LTCG
        /OPT:REF
        /OPT:ICF
        /SUBSYSTEM:CONSOLE
    >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:
        -flto=auto
        -s
    >
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:
        -flto=thin
        -s
    >
)
