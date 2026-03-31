include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(test_metrics   test_metrics.cpp)
add_executable(test_integration test_integration.cpp)

target_link_libraries(test_metrics   PRIVATE DeciBeliever_lib Catch2::Catch2WithMain)
target_link_libraries(test_integration PRIVATE DeciBeliever_lib Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_metrics)
catch_discover_tests(test_integration)
