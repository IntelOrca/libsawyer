cmake_minimum_required(VERSION 3.15)
project(fsaw CXX)

find_package(sawyer REQUIRED PATHS ../../out/share)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
else ()

endif ()

add_executable(fsaw ${SAWYER_SOURCES})
if (${VCPKG_TARGET_TRIPLET} MATCHES "-static$")
    set_property(TARGET fsaw PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

target_link_libraries(fsaw sawyer::sawyer)
