include(FetchContent)
FetchContent_Declare(yaml-cpp GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
                              SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/yaml-cpp")
FetchContent_MakeAvailable(yaml-cpp)
