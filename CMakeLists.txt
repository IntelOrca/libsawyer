cmake_minimum_required(VERSION 3.14)

# Options
option(ENABLE_TESTS "Build the unit tests for the library." ON)

# Library
project(sawyer CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

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

# Set default install prefix to out
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/out" CACHE PATH "..." FORCE)
endif()

install(DIRECTORY "src/" DESTINATION "include/sawyer" FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(TARGETS sawyer EXPORT sawyerConfig LIBRARY ARCHIVE DESTINATION ${TARGETS_PREFIX})
install(EXPORT sawyerConfig DESTINATION share)

## Testing
if (ENABLE_TESTS)
    find_package(GTest REQUIRED)
    set(GTEST_LIBRARIES ${GTEST_BOTH_LIBRARIES})

    include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
    include_directories("${CMAKE_INSTALL_PREFIX}/include")
    link_directories("${CMAKE_INSTALL_PREFIX}/lib")

    file(GLOB_RECURSE SAWYER_TEST_SOURCES "test/*.cpp")
    
    add_executable(tests EXCLUDE_FROM_ALL ${SAWYER_TEST_SOURCES})
    add_dependencies(tests install)
    target_link_libraries(tests ${GTEST_LIBRARIES} sawyer pthread)
    add_test(NAME libsawyer COMMAND tests)
endif ()
