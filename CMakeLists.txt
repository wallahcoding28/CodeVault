cmake_minimum_required(VERSION 3.16)
project(CodeVault VERSION 0.1.0 LANGUAGES C CXX)

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE language servers (clangd / cpptools)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Global include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Third-party SQLite library
add_library(sqlite3 STATIC
    third_party/sqlite/sqlite3.c
)
target_include_directories(sqlite3 SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)
if (NOT MSVC)
    target_compile_options(sqlite3 PRIVATE -w)
endif()

# Third-party Argon2 library
add_library(argon2 STATIC
    third_party/argon2/src/argon2.c
    third_party/argon2/src/core.c
    third_party/argon2/src/encoding.c
    third_party/argon2/src/ref.c
    third_party/argon2/src/thread.c
    third_party/argon2/src/blake2/blake2b.c
)
target_include_directories(argon2 SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/argon2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/argon2/src
)
if (NOT MSVC)
    target_compile_options(argon2 PRIVATE -w)
endif()

# Core CodeVault Library
add_library(codevault_core STATIC
    src/persistence/file_question_repository.cpp
    src/persistence/sqlite_question_repository.cpp
    src/persistence/sqlite_user_repository.cpp
    src/persistence/migration_service.cpp
    src/services/question_service.cpp
    src/services/search_service.cpp
    src/services/revision_service.cpp
    src/services/practice_service.cpp
    src/services/recent_history_service.cpp
    src/services/problem_playlist_service.cpp
    src/services/statistics_service.cpp
    src/dsa/trie.cpp
    src/utils/crypto.cpp
    src/utils/json.cpp
    src/persistence/sqlite_session_repository.cpp
    src/services/auth_service.cpp
    src/services/import_service.cpp
    src/services/export_service.cpp
    src/app/cli_shell.cpp
    src/app/http_server.cpp
)

target_include_directories(codevault_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/argon2/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(codevault_core PUBLIC sqlite3 argon2)

if (WIN32)
    target_link_libraries(codevault_core PUBLIC ws2_32 advapi32)
endif()

# Main Executable Shell
add_executable(codevault
    src/main.cpp
)

target_link_libraries(codevault PRIVATE codevault_core)

# Smoke Test Executable
add_executable(smoke_test
    tests/smoke/smoke_test.cpp
)

target_link_libraries(smoke_test PRIVATE codevault_core)

# Unit Tests Executable
add_executable(unit_tests
    tests/unit/unit_tests.cpp
)

target_link_libraries(unit_tests PRIVATE codevault_core)

# Enable CTest integration
enable_testing()
add_test(NAME SmokeTest COMMAND smoke_test "${CMAKE_CURRENT_SOURCE_DIR}/data/sample/questions.csv")
add_test(NAME UnitTests COMMAND unit_tests)
set_tests_properties(SmokeTest PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(UnitTests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Copy compile_commands.json to root directory for language servers
add_custom_target(copy_compile_commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    COMMENT "Syncing compile_commands.json to root for IDE IntelliSense"
)
