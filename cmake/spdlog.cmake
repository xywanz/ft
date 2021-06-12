include(FetchContent)
FetchContent_Declare(spdlog GIT_REPOSITORY https://hub.fastgit.org/gabime/spdlog.git
                            GIT_TAG v1.8.5
                            SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/spdlog")
set(SPDLOG_FMT_EXTERNAL ON CACHE INTERNAL "Use external fmt")
FetchContent_MakeAvailable(spdlog)
