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
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(nlohmann_json googletest httplib)

add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
    src/SlidingWindow.cpp
    src/FixedWindow.cpp
    src/HttpServer.cpp
    src/PersistenceManager.cpp
)
target_include_directories(rate_limiter_lib PUBLIC include)
target_link_libraries(rate_limiter_lib PUBLIC nlohmann_json::nlohmann_json httplib::httplib)
target_compile_options(rate_limiter_lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

add_executable(rate_limiter src/main.cpp)
target_link_libraries(rate_limiter PRIVATE rate_limiter_lib)

add_custom_command(
    TARGET rate_limiter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/web"
            "$<TARGET_FILE_DIR:rate_limiter>/web"
    COMMENT "Copying web dashboard to output directory"
)

add_custom_command(
    TARGET rate_limiter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/launch.bat"
            "$<TARGET_FILE_DIR:rate_limiter>/launch.bat"
    COMMENT "Copying launch script to output directory"
)

add_executable(rate_limiter_sim simulations/multi_thread_sim.cpp)
target_link_libraries(rate_limiter_sim PRIVATE rate_limiter_lib)

enable_testing()
add_subdirectory(tests)
