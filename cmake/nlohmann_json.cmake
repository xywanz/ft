include(FetchContent)
FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git
                                   SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/nlohmann_json")
set(JSON_BuildTests OFF CACHE INTERNAL "Don't build the json tests")
FetchContent_MakeAvailable(nlohmann_json)
