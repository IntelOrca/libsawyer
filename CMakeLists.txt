cmake_minimum_required(VERSION 3.14)

# Options
option(ENABLE_DISCORD_RPC   "Embed discord-rpc into the library." ON)
option(ENABLE_SCRIPTING     "Embed duktape and dukglue into the library." ON)
option(ENABLE_TESTS         "Build the unit tests for the library." ON)

# Library
project(sawyer C CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")
file(GLOB_RECURSE SAWYER_HEADERS "src/*.h" "src/*.hpp")
add_library(sawyer STATIC ${SAWYER_SOURCES})

# Discord RPC
if (ENABLE_DISCORD_RPC)
    include(ExternalProject)
    ExternalProject_Add(
        discord-rpc
        URL https://github.com/discord/discord-rpc/archive/963aa9f3e5ce81a4682c6ca3d136cddda614db33.zip
        URL_HASH SHA1=10081a9a4e68459884185a25ff0d22ab6cd5a184
        TIMEOUT 10
        CMAKE_ARGS "-DBUILD_EXAMPLES=off" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)
    ExternalProject_Get_Property(discord-rpc SOURCE_DIR)
    ExternalProject_Get_Property(discord-rpc BINARY_DIR)
    if (MSVC)
        set(DISCORD_RPC_LIB "${BINARY_DIR}/src/discord-rpc.lib")
    else ()
        set(DISCORD_RPC_LIB "${BINARY_DIR}/src/libdiscord-rpc.a")
    endif ()

    add_dependencies(sawyer discord-rpc)

    target_include_directories(sawyer PRIVATE "${SOURCE_DIR}/include")
    target_compile_definitions(sawyer PRIVATE CS_ENABLE_DISCORD_RPC)
endif ()

# Scripting
if (ENABLE_SCRIPTING)
    include(ExternalProject)
    ExternalProject_Add(
        duktape
        URL https://github.com/svaarala/duktape/releases/download/v2.6.0/duktape-2.6.0.tar.xz
        URL_HASH SHA1=5936bf5e9943128b53a9e534e0a6842532b52bb5
        TIMEOUT 10
        BUILD_BYPRODUCTS "duktape-prefix/src/duktape/duktape-src/duktape.c"
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND python2 tools/configure.py --output-directory duktape-src -UDUK_USE_DATE_NOW_WINDOWS -UDUK_USE_CPP_EXCEPTIONS
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON)

    ExternalProject_Get_Property(duktape SOURCE_DIR)
    set(DUKTAPE_SOURCES "${SOURCE_DIR}/duktape-src/duktape.c")

    add_compile_definitions(sawyer PRIVATE CS_ENABLE_SCRIPTING)
    target_sources(sawyer PRIVATE ${DUKTAPE_SOURCES})
    add_dependencies(sawyer duktape)
    install(FILES "${SOURCE_DIR}/duktape-src/duk_config.h" DESTINATION "include")
    install(FILES "${SOURCE_DIR}/duktape-src/duktape.h" DESTINATION "include")
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
else ()

endif ()

if (ENABLE_DISCORD_RPC)
    if (MSVC)
        # Merge discord-rpc.lib into sawyer.lib
        set(LIBSAWYER_PATH "${CMAKE_BINARY_DIR}/sawyer.lib")
        set(LIBSAWYER_MAIN_PATH "${CMAKE_BINARY_DIR}/temp/sawyer.lib")
        add_custom_command(TARGET sawyer POST_BUILD
            COMMAND md temp && mv ${LIBSAWYER_PATH} ${LIBSAWYER_MAIN_PATH}
            COMMAND lib "/out:${LIBSAWYER_PATH}" ${LIBSAWYER_MAIN_PATH} ${DISCORD_RPC_LIB}
            COMMENT "Merging multiple static libraries")
    else ()
        # Merge libdiscord-rpc.a into libsawyer.a
        set(LIBSAWYER_PATH "${CMAKE_BINARY_DIR}/libsawyer.a")
        set(LIBSAWYER_MAIN_PATH "${CMAKE_BINARY_DIR}/temp/libsawyer.a")
        add_custom_command(TARGET sawyer POST_BUILD
            COMMAND mkdir -p temp && mv ${LIBSAWYER_PATH} ${LIBSAWYER_MAIN_PATH}
            COMMAND ${CMAKE_AR} rcT ${LIBSAWYER_PATH} ${LIBSAWYER_MAIN_PATH} ${DISCORD_RPC_LIB}
            COMMENT "Merging multiple static libraries")
    endif ()
endif ()

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

    file(GLOB_RECURSE SAWYER_TEST_SOURCES "test/*.cpp")
    add_executable(tests EXCLUDE_FROM_ALL ${SAWYER_TEST_SOURCES})
    target_include_directories(tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_include_directories(tests PRIVATE "${CMAKE_INSTALL_PREFIX}/include")
    add_dependencies(tests install)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    target_link_libraries(tests ${GTEST_LIBRARIES} sawyer Threads::Threads)
    gtest_discover_tests(tests)
endif ()
