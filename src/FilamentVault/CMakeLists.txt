cmake_minimum_required(VERSION 3.13)
project(FilamentVault)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLiteCpp REQUIRED)

set(FilamentVault_SOURCES
    Core/VaultModel.cpp
    Core/SpoolMatcher.cpp
    DB/SchemaManager.cpp
    DB/SQLiteStore.cpp
)

set(FilamentVault_HEADERS
    Core/Spool.hpp
    Core/VaultModel.hpp
    Core/ConsumptionCalculator.hpp
    Core/SpoolMatcher.hpp
    DB/SchemaManager.hpp
    DB/SQLiteStore.hpp
)

add_library(FilamentVault STATIC
    ${FilamentVault_SOURCES}
    ${FilamentVault_HEADERS}
)

target_include_directories(FilamentVault
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(FilamentVault
    PUBLIC
        SQLiteCpp
)
