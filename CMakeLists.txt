cmake_minimum_required(VERSION 3.23)
project(httpserver LANGUAGES C)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
unset(SOURCES)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE OS_WINDOWS)
endif()

find_package(libnsocket REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libnsocket::nsocket)

find_package(libmonotime REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libmonotime::monotime)