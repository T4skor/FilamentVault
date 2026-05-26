# FindSQLiteCpp.cmake
# Finds the SQLiteCpp library (a C++ wrapper around SQLite3)
#
# This module defines:
#  SQLiteCpp_INCLUDE_DIRS - include directories
#  SQLiteCpp_LIBRARIES    - libraries to link
#  SQLiteCpp_FOUND        - true if found

include(FindPackageHandleStandardArgs)

find_path(SQLiteCpp_INCLUDE_DIR
    NAMES SQLiteCpp/SQLiteCpp.h SQLiteCpp/Database.h
    PATHS /usr/local/include /usr/include /home/pablo/.local/include
)

find_library(SQLiteCpp_LIBRARY
    NAMES SQLiteCpp libSQLiteCpp
    PATHS /usr/local/lib /usr/lib /usr/lib64 /home/pablo/.local/lib
)

find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    set(SQLiteCpp_SQLite3_INCLUDE_DIR ${SQLite3_INCLUDE_DIRS})
    list(APPEND SQLiteCpp_LIBRARIES ${SQLite3_LIBRARIES})
endif()

find_package_handle_standard_args(SQLiteCpp DEFAULT_MSG
    SQLiteCpp_INCLUDE_DIR
    SQLiteCpp_LIBRARY
)

if(SQLiteCpp_FOUND)
    set(SQLiteCpp_INCLUDE_DIRS ${SQLiteCpp_INCLUDE_DIR})
    set(SQLiteCpp_LIBRARIES ${SQLiteCpp_LIBRARY} ${SQLite3_LIBRARIES})
    mark_as_advanced(SQLiteCpp_INCLUDE_DIR SQLiteCpp_LIBRARY)
endif()
