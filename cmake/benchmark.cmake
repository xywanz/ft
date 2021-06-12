include(FetchContent)
FetchContent_Declare(benchmark GIT_REPOSITORY https://github.com/google/benchmark.git
                               GIT_TAG v1.5.5
                               SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/benchmark")
set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Don't build benchmark test")
FetchContent_MakeAvailable(benchmark)
