cmake_minimum_required(VERSION 3.23)

project(compositor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(compositor ${SOURCES})

target_include_directories(compositor
    PUBLIC SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MIRAL_INCLUDE_DIRS}
)

target_link_libraries(compositor
    ${MIRAL_LDFLAGS}
    ${XKBCOMMON_LIBRARIES}
)

enable_testing()

add_subdirectory(tests)
