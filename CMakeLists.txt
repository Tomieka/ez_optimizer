cmake_minimum_required(VERSION 3.10...3.27)

project(PacmanCacheCleaner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

add_executable(PacmanCacheCleaner
    main.cpp
)

target_link_libraries(PacmanCacheCleaner PRIVATE Qt5::Core Qt5::Widgets Qt5::Network)

install(TARGETS PacmanCacheCleaner
    RUNTIME DESTINATION bin
) 