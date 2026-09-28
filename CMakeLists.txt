cmake_minimum_required(VERSION 3.10)
project(passpix CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libsodium provides Argon2id, XChaCha20-Poly1305 and SHA-256
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()

if(SODIUM_FOUND)
    set(SODIUM_INCLUDE ${SODIUM_INCLUDE_DIRS})
    set(SODIUM_LIBS ${SODIUM_LINK_LIBRARIES})
    if(NOT SODIUM_LIBS)
        set(SODIUM_LIBS ${SODIUM_LIBRARIES})
    endif()
else()
    find_path(SODIUM_INCLUDE sodium.h)
    find_library(SODIUM_LIBS sodium)
    if(NOT SODIUM_INCLUDE OR NOT SODIUM_LIBS)
        message(FATAL_ERROR "Could NOT find libsodium.\n"
            "Install the libsodium development files and re-run CMake.\n"
            "Examples:\n"
            "  - Debian/Ubuntu: sudo apt install libsodium-dev\n"
            "  - MSYS2: pacman -S mingw-w64-ucrt-x86_64-libsodium\n"
            "  - Homebrew: brew install libsodium\n")
    endif()
endif()

message(STATUS "libsodium include: ${SODIUM_INCLUDE}")
message(STATUS "libsodium libraries: ${SODIUM_LIBS}")

set(CORE_SOURCES
    src/crypto_utils.cpp
    src/image_gen.cpp
    src/stego.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)

# Define the executable
add_executable(passpix
    src/main.cpp
    src/terminal.cpp
    src/clipboard.cpp
    ${CORE_SOURCES}
)
target_include_directories(passpix PRIVATE src Include ${SODIUM_INCLUDE})
target_link_libraries(passpix PRIVATE ${SODIUM_LIBS})

# Testing
enable_testing()

add_executable(test_runner
    test/catch_amalgamated.cpp
    test/test_crypto.cpp
    test/test_stego.cpp
    ${CORE_SOURCES}
)
target_include_directories(test_runner PRIVATE src Include ${SODIUM_INCLUDE})
target_link_libraries(test_runner PRIVATE ${SODIUM_LIBS})

add_test(NAME passpix_tests COMMAND test_runner WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

find_program(BASH_EXECUTABLE bash)
if(BASH_EXECUTABLE)
    add_test(NAME passpix_smoke
             COMMAND ${BASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/smoke_test.sh $<TARGET_FILE:passpix>)
endif()
