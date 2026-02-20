# tests/CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(oeselect_tests
    cpp/test_main.cpp
    cpp/test_selection.cpp
    cpp/test_spatial_index.cpp
)

target_link_libraries(oeselect_tests
    PRIVATE
        oeselect
        GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(oeselect_tests)
