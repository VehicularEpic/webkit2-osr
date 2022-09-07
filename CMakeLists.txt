cmake_minimum_required(VERSION 3.0)
project(webkit2-osr)

if (UNIX)
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glad/cmake)
glad_add_library(glad STATIC API gl:core=3.3)

add_executable(webkit2-osr main.c)

target_link_libraries(webkit2-osr
    glad
    ${GLFW3_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${WEBKIT2GTK_LIBRARIES}
)

target_include_directories(webkit2-osr PRIVATE
    ${GLAD_DIR}/include
    ${GLFW3_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${WEBKIT2GTK_INCLUDE_DIRS}
)
