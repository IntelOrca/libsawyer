cmake_minimum_required(VERSION 3.14)
project(sawyer CXX)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")
file(GLOB_RECURSE SAWYER_HEADERS "src/*.h" "src/*.hpp")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
else ()

endif ()

add_library(sawyer STATIC ${SAWYER_SOURCES})
# set_target_properties(sawyer PROPERTIES PUBLIC_HEADER "src/SawyerStream.h;src\\thirdparty/span.hpp")

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if (CMAKE_BUILD_TYPE_UPPER MATCHES "DEBUG")
    set(TARGETS_PREFIX "debug/lib")
endif ()

install(DIRECTORY "src/" DESTINATION "include/sawyer" FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(TARGETS sawyer EXPORT sawyerConfig LIBRARY ARCHIVE DESTINATION ${TARGETS_PREFIX})
install(EXPORT sawyerConfig DESTINATION share)
