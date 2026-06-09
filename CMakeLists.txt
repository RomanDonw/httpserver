cmake_minimum_required(VERSION 3.23)
project(httpserver LANGUAGES C)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
unset(SOURCES)

#target_link_options(${PROJECT_NAME} PRIVATE
#    "-static-libgcc"
#    "-Wl,-Bstatic,--whole-archive"
#    "-lwinpthread"
#    "-Wl,--no-whole-archive"
#    "-O2"
#)

find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
set(ENV{PKG_CONFIG} "pkg-config --static")

find_package(libsocket REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libsocket::socket)