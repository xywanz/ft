include(FetchContent)
FetchContent_Declare(hiredis GIT_REPOSITORY https://github.com/redis/hiredis.git
                             GIT_TAG v1.0.0
                             SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/hiredis")
set(DISABLE_TESTS ON CACHE INTERNAL "Disable hiredis tests")
FetchContent_MakeAvailable(hiredis)
unset(DISABLE_TESTS CACHE)
