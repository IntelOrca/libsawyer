cmake_minimum_required(VERSION 3.15)
project(gxc CXX)

# libsawyer
find_package(sawyer REQUIRED PATHS ../../out/share)

# libraries
if (MSVC)
    find_package(libpng CONFIG REQUIRED)
endif ()
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
    if (EXISTS "/etc/alpine-release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
    endif()
endif ()

add_executable(gxc ${SAWYER_SOURCES})
if (${VCPKG_TARGET_TRIPLET} MATCHES "-static$")
    set_property(TARGET gxc PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(gxc PRIVATE png_static)
endif ()

target_link_libraries(gxc PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(gxc PRIVATE sawyer::sawyer)
