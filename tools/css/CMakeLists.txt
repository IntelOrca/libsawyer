cmake_minimum_required(VERSION 3.15)
project(css C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE CSS_SOURCES "src/*.c")

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHsc /utf-8 /WX /wd5105")
else ()

endif ()

add_executable(css ${CSS_SOURCES})
if (${VCPKG_TARGET_TRIPLET} MATCHES "-static$")
    set_property(TARGET css PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
