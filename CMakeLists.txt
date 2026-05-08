cmake_minimum_required(VERSION 3.16)
project(RateLimiter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(nlohmann_json googletest)

add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
    src/SlidingWindow.cpp
    src/FixedWindow.cpp
)
target_include_directories(rate_limiter_lib PUBLIC include)
target_link_libraries(rate_limiter_lib PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(rate_limiter_lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

add_executable(rate_limiter src/main.cpp)
target_link_libraries(rate_limiter PRIVATE rate_limiter_lib)

enable_testing()
add_subdirectory(tests)
