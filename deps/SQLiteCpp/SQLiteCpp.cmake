orcaslicer_add_cmake_project(SQLiteCpp
  URL                 https://github.com/SRombauts/SQLiteCpp/archive/refs/tags/3.3.3.zip
  CMAKE_ARGS
    -DSQLITECPP_USE_SYSTEM_SQLITE:BOOL=OFF
    -DSQLITECPP_BUILD_TESTS:BOOL=OFF
    -DSQLITECPP_RUN_DOXYGEN:BOOL=OFF
)
