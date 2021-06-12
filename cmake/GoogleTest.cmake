include(FetchContent)
FetchContent_Declare(GoogleTest GIT_REPOSITORY https://github.com/google/googletest.git
                                GIT_TAG release-1.11.0
                                SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/googletest")
FetchContent_MakeAvailable(GoogleTest)
find_package(GTest)
