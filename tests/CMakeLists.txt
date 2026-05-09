add_executable(rate_limiter_tests
    test_metrics.cpp
    test_token_bucket.cpp
    test_manager.cpp
    test_sliding_window.cpp
    test_fixed_window.cpp
    test_persistence.cpp
)
target_link_libraries(rate_limiter_tests
    PRIVATE rate_limiter_lib GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(rate_limiter_tests)
