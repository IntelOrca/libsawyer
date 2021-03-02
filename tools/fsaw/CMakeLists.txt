cmake_minimum_required(VERSION 3.14)
project(fsaw CXX)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8 /WX")
else ()

endif ()

include_directories("../../out/include")
link_directories("../../out/lib")

add_executable(fsaw ${SAWYER_SOURCES})
