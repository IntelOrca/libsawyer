cmake_minimum_required(VERSION 3.14)
project(spritec CXX)

# libsawyer
find_package(sawyer REQUIRED PATHS ../../out/share)

# libraries
find_package(libpng CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
else ()

endif ()

add_executable(spritec ${SAWYER_SOURCES})
target_link_libraries(spritec PRIVATE png)
target_link_libraries(spritec PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(spritec PRIVATE sawyer::sawyer)
