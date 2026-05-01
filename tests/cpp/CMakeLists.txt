include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(oeselect_tests
    test_main.cpp
    test_selection.cpp
    test_spatial_index.cpp
    test_glob_match.cpp
)

target_include_directories(oeselect_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(oeselect_tests
    PRIVATE
        oeselect
        GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(oeselect_tests)
