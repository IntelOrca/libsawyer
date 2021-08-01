cmake_minimum_required(VERSION 3.14)

# Do not allow building in root
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Do not build in the root. Create a bin directory and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif ()

function (merge_static_libraries target source)
    set(TARGET_PATH $<TARGET_FILE:${target}>)
    if (MSVC)
        set(TEMP_PATH "tmplib")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND mv ${TARGET_PATH} ${TEMP_PATH}
            COMMAND lib "/out:${TARGET_PATH}" ${TEMP_PATH} ${source}
            COMMAND rm ${TEMP_PATH}
            COMMENT "Relinking ${target} with ${source}")
    else ()
        set(TEMP_PATH "tmpobj")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND mkdir -p ${TEMP_PATH}
            COMMAND ${CMAKE_AR} x ${TARGET_PATH} --output ${TEMP_PATH}
            COMMAND ${CMAKE_AR} x ${source} --output ${TEMP_PATH}
            COMMAND ${CMAKE_AR} qc ${TARGET_PATH} "${TEMP_PATH}/*.o"
            COMMAND rm -rf ${TEMP_PATH}
            COMMENT "Relinking ${target} with ${source}")
    endif ()
endfunction ()

# Options
option(ENABLE_BREAKPAD      "Embed breakpad into the library." ON)
option(ENABLE_DISCORD_RPC   "Embed discord-rpc into the library." ON)
option(ENABLE_SCRIPTING     "Embed duktape and dukglue into the library." ON)
option(ENABLE_TESTS         "Build the unit tests for the library." ON)

# Library
project(sawyer CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SAWYER_SOURCES "src/*.cpp")
file(GLOB_RECURSE SAWYER_HEADERS "src/*.h" "src/*.hpp")
add_library(sawyer STATIC ${SAWYER_SOURCES})

# Breakpad
if (ENABLE_BREAKPAD AND MSVC)
    target_compile_definitions(sawyer PRIVATE CS_ENABLE_BREAKPAD)

    find_package(unofficial-breakpad CONFIG REQUIRED)

    # Add breakpad include directories
    get_target_property(BREAKPAD_INCLUDE_DIRS unofficial::breakpad::libbreakpad INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(sawyer PRIVATE ${BREAKPAD_INCLUDE_DIRS})

    # Get other .lib files breakpad depends on
    get_target_property(BREAKPAD_LIBS_A unofficial::breakpad::libbreakpad INTERFACE_LINK_LIBRARIES)
    get_target_property(BREAKPAD_LIBS_B unofficial::breakpad::libbreakpad_client INTERFACE_LINK_LIBRARIES)

    # Merge all breakpad .lib into sawyer.lib
    merge_static_libraries(sawyer ${BREAKPAD_LIBS_A})

    # Complains about wininet.lib
    # merge_static_libraries(sawyer ${BREAKPAD_LIBS_B})

    merge_static_libraries(sawyer $<TARGET_FILE:unofficial::breakpad::libbreakpad>)
    merge_static_libraries(sawyer $<TARGET_FILE:unofficial::breakpad::libbreakpad_client>)
else ()
    set(ENABLE_BREAKPAD OFF)
endif ()

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
    merge_static_libraries(sawyer ${DISCORD_RPC_LIB})
endif ()

# Scripting
if (ENABLE_SCRIPTING)
    set(DUKTAPE_DEFINES "-DDUK_USE_CPP_EXCEPTIONS" "-DDUK_USE_INTERRUPT_COUNTER" "-DDUK_USE_EXEC_TIMEOUT_CHECK=duk_exec_timeout_check" "--fixup-line=extern duk_bool_t duk_exec_timeout_check(void*)\\\\\\\\\\\;")
    if (MSVC)
        set(DUKTAPE_DEFINES ${DUKTAPE_DEFINES} "-DDUK_USE_DATE_NOW_WINDOWS")
    endif ()

    include(ExternalProject)
    ExternalProject_Add(
        duktape
        URL https://github.com/svaarala/duktape/releases/download/v2.6.0/duktape-2.6.0.tar.xz
        URL_HASH SHA1=5936bf5e9943128b53a9e534e0a6842532b52bb5
        TIMEOUT 10
        BUILD_BYPRODUCTS "duktape-prefix/src/duktape/duktape-src/duktape.c"
        BUILD_IN_SOURCE ON
        CONFIGURE_COMMAND python2 tools/configure.py --output-directory duktape-src ${DUKTAPE_DEFINES}
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON)

    ExternalProject_Get_Property(duktape SOURCE_DIR)
    set(DUKTAPE_SOURCES "${SOURCE_DIR}/duktape-src/duktape.c")
    set_source_files_properties(${DUKTAPE_SOURCES} PROPERTIES LANGUAGE CXX)

    add_compile_definitions(sawyer PRIVATE CS_ENABLE_SCRIPTING)
    target_sources(sawyer PRIVATE ${DUKTAPE_SOURCES})
    add_dependencies(sawyer duktape)
    install(FILES "${SOURCE_DIR}/duktape-src/duk_config.h" DESTINATION "include")
    install(FILES "${SOURCE_DIR}/duktape-src/duktape.h" DESTINATION "include")
endif ()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /std:c++17 /utf-8 /WX")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else ()

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

# Expose include directory for library consumers
target_include_directories(sawyer INTERFACE $<INSTALL_INTERFACE:include>)

install(DIRECTORY "src/" DESTINATION "include/sawyer" FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(TARGETS sawyer EXPORT sawyerConfig LIBRARY ARCHIVE DESTINATION ${TARGETS_PREFIX})
install(EXPORT sawyerConfig NAMESPACE sawyer:: DESTINATION share)

## Testing
if (ENABLE_TESTS)
    find_package(GTest REQUIRED)
    set(GTEST_LIBRARIES ${GTEST_BOTH_LIBRARIES})

    set(SAWYER_TEST_CRASH_HANDLER_SOURCES "test/CrashHandlerTest.cpp")

    file(GLOB_RECURSE SAWYER_TEST_SOURCES "test/*.cpp")
    list(FILTER SAWYER_TEST_SOURCES EXCLUDE REGEX ${SAWYER_TEST_CRASH_HANDLER_SOURCES})
    add_executable(tests EXCLUDE_FROM_ALL ${SAWYER_TEST_SOURCES})
    target_include_directories(tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
    target_include_directories(tests SYSTEM PRIVATE "${CMAKE_INSTALL_PREFIX}/include")
    add_dependencies(tests install)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    target_link_libraries(tests ${GTEST_LIBRARIES} sawyer Threads::Threads)
    gtest_discover_tests(tests)

    if (ENABLE_BREAKPAD)
        add_executable(test_crash EXCLUDE_FROM_ALL ${SAWYER_TEST_CRASH_HANDLER_SOURCES})
        target_include_directories(test_crash SYSTEM PRIVATE "${CMAKE_INSTALL_PREFIX}/include")
        target_link_libraries(test_crash sawyer)
        add_dependencies(test_crash install)
        add_test(NAME test_crash COMMAND $<TARGET_FILE:test_crash>)
    endif ()
endif ()
