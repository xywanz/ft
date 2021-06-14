include(FetchContent)
FetchContent_Declare(libuv GIT_REPOSITORY https://github.com/libuv/libuv.git
                           GIT_TAG v1.41.0
                           SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/libuv")
FetchContent_MakeAvailable(libuv)
