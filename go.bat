@echo off
cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_TOOLCHAIN_FILE=M:/git/vcpkg/scripts/buildsystems/vcpkg.cmake" || goto fail
cmake --build bin || goto fail
cmake --install bin || goto fail
goto :eof

:fail
