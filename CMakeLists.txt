cmake_minimum_required(VERSION 3.23)
project(httpserver)

set(BUILD_SHARED_LIBS OFF)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")

add_executable(httpserver ${SOURCES})

target_link_options(httpserver PRIVATE
    "-static-libgcc"
    "-Wl,-Bstatic,--whole-archive"
    "-lwinpthread"
    "-Wl,--no-whole-archive"
    "-O2"
)

find_package(PkgConfig REQUIRED)
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
set(ENV{PKG_CONFIG} "pkg-config --static")

find_package(json-c REQUIRED)
target_link_libraries(httpserver PRIVATE json-c::json-c)