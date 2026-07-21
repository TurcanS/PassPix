cmake_minimum_required(VERSION 3.10)
project(passpix)

set(CMAKE_CXX_STANDARD 17)

# Try to find OpenSSL normally first
find_package(OpenSSL QUIET)

if(NOT OPENSSL_FOUND)
    message(STATUS "OpenSSL was not found by the default search. Trying common locations and environment variables...")

    # Candidate OpenSSL root folders to try (includes common Windows, macOS/Homebrew and Unix locations)
    set(_OPENSSL_SEARCH_PATHS
        "$ENV{OPENSSL_ROOT_DIR}"
        "$ENV{OPENSSL_DIR}"
        "C:/Program Files/OpenSSL-Win64"
        "C:/Program Files (x86)/OpenSSL-Win32"
        "C:/OpenSSL-Win64"
        "C:/OpenSSL-Win32"
        "/usr/local/opt/openssl"
        "/usr/local/opt/openssl@1.1"
        "/usr/local/ssl"
        "/usr/ssl"
        "/usr"
        "/usr/local"
    )

    foreach(_p IN LISTS _OPENSSL_SEARCH_PATHS)
        if(_p AND EXISTS "${_p}")
            message(STATUS "Trying OPENSSL_ROOT_DIR=${_p}")
            # Set as a CACHE PATH so FindOpenSSL will use it
            set(OPENSSL_ROOT_DIR "${_p}" CACHE PATH "OpenSSL root dir (auto-probed)" FORCE)
            find_package(OpenSSL QUIET)
            if(OPENSSL_FOUND)
                message(STATUS "Found OpenSSL in: ${_p}")
                break()
            endif()
        endif()
    endforeach()
endif()

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Could NOT find OpenSSL. Tried default search and common locations.\n"
        "Install OpenSSL development files or set OPENSSL_ROOT_DIR to your OpenSSL install folder and re-run CMake.\n"
        "Examples:\n"
        "  - On Windows with prebuilt OpenSSL: set OPENSSL_ROOT_DIR=\"C:/Program Files/OpenSSL-Win64\"\n"
        "  - With Homebrew on macOS: -DOPENSSL_ROOT_DIR=\"$(brew --prefix openssl)\"\n"
        "  - On Debian/Ubuntu install libssl-dev and re-run CMake (sudo apt install libssl-dev)\n")
endif()

message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL crypto lib: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OpenSSL ssl lib: ${OPENSSL_SSL_LIBRARY}")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM libsodium)
endif()
if(NOT SODIUM_FOUND)
    find_library(SODIUM_LIBRARY sodium)
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/crypto_utils.cpp
    src/image_gen.cpp
    src/stego.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)

# Define the executable
add_executable(passpix ${SOURCES})

# Link OpenSSL libraries (FindOpenSSL provides imported targets on modern CMake)
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(passpix OpenSSL::SSL OpenSSL::Crypto)
else()
    # Fallback to using the variables if imported targets aren't available
    target_include_directories(passpix PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(passpix PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
endif()

# Testing
enable_testing()

set(TEST_SOURCES
    test/catch_amalgamated.cpp
    test/test_crypto.cpp
    test/test_stego.cpp
    src/crypto_utils.cpp
    src/image_gen.cpp
    src/stego.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)

add_executable(test_runner ${TEST_SOURCES})
target_include_directories(test_runner PRIVATE src Include)

if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(test_runner OpenSSL::SSL OpenSSL::Crypto ${SODIUM_LIBRARIES} -lsodium)
else()
    target_include_directories(test_runner PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(test_runner PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${SODIUM_LIBRARIES} -lsodium)
endif()

add_test(NAME passpix_tests COMMAND test_runner WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
