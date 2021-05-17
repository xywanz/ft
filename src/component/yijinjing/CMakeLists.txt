
add_library(yijinjing STATIC
    journal/Journal.cpp
    journal/JournalHandler.cpp
    journal/JournalReader.cpp
    journal/JournalWriter.cpp
    journal/LocalPageProvider.cpp
    journal/Page.cpp
    journal/PageUtil.cpp
    journal/Timer.cpp
    paged/PageEngine.cpp
    paged/PageServiceTask.cpp)
