#!/bin/bash
set -e

usage() {
  echo "usage: build TARGET [ -c CONFIGURATION ] [ --static ] [ -p PLATFORM ] [ --cmake CMAKE_ARGS ]"
  echo "targets:"
  echo "    libsawyer (default)"
  echo "    fsaw"
  echo "    gxc"
  echo "    tools"
  exit 2
}

OS=unix
BINEXT=
ARTEFACT_OS=linux
ARTEFACT_EXT=.tar.gz
if [[ "$OSTYPE" == "cygwin" || "$OSTYPE" == "msys" ]]; then
    OS=windows
    BINEXT=.exe
    ARTEFACT_OS=win
    ARTEFACT_EXT=.zip
elif [[ -f "/etc/alpine-release" ]]; then
    ARTEFACT_OS=musl
fi

COMPONENT="libsawyer"
CONFIGURATION=Release
PLATFORM=x64
CMAKE_ARGS=""
while (( "$#" )); do
    case "$1" in
        --static)
            STATIC=true
            shift
            ;;
        -c)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                CONFIGURATION=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing" >&2
                exit 1
            fi
            ;;
        --cmake)
            if [ -n "$2" ] ; then
                CMAKE_ARGS=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing" >&2
                exit 1
            fi
            ;;
        -p)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                PLATFORM=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing" >&2
                exit 1
            fi
            ;;
        -h|--help)
            usage
            ;;
        -*) # unsupported flags
            echo "Error: Unsupported flag $1" >&2
            exit 1
            ;;
        *)
            COMPONENT="$1"
            shift
            ;;
    esac
done

VCPKG_TRIPLET_ARG=""
VCPKG_TOOLCHAIN_ARG=""
if [ "$STATIC" == "true" ]; then
    VCPKG_TRIPLET_ARG="-DVCPKG_TARGET_TRIPLET=$PLATFORM-windows-static"
    STATIC_DESC="-static"
fi
if [ "$OS" == "windows" ]; then
    if [[ "$VCPKG_INSTALLATION_ROOT" = "" ]]; then
        echo "\$VCPKG_INSTALLATION_ROOT not specified";
        exit 1
    fi
    VCPKG_TOOLCHAIN_ARG="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake"
fi

CMAKE_CLEAN="rm -rf bin out"
CMAKE_CONFIGURE_CMD="cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=$CONFIGURATION $VCPKG_TRIPLET_ARG $VCPKG_TOOLCHAIN_ARG $CMAKE_ARGS"
CMAKE_BUILD_CMD="cmake --build bin"
CMAKE_INSTALL_CMD="cmake --install bin"

archive() {
    mkdir -p artefacts
    pushd "$2" > /dev/null
        rm -f "../artefacts/$1"
        if [[ "$OSTYPE" == "cygwin" || "$OSTYPE" == "msys" ]]; then
            7z a -y -bd -r "../artefacts/$1" ./*
        else
            tar -czf "../artefacts/$1" *
        fi
    popd > /dev/null
}

TARGET_DESC="$CONFIGURATION"
if [[ "$STATIC" = "true" ]]; then
    TARGET_DESC="$CONFIGURATION (static)"
fi

if [[ "$COMPONENT" = "libsawyer" ]]; then
    echo -e "\033[0;36mBuilding libsawyer for $TARGET_DESC\033[0m"
    $CMAKE_CLEAN
    $CMAKE_CONFIGURE_CMD
    $CMAKE_BUILD_CMD
    $CMAKE_INSTALL_CMD
    archive "libsawyer-$ARTEFACT_OS-$PLATFORM$STATIC_DESC$ARTEFACT_EXT" out
fi

if [[ "$COMPONENT" = "fsaw" ]]; then
    echo -e "\033[0;36mBuilding fsaw for $TARGET_DESC\033[0m"
    pushd tools/fsaw > /dev/null
        $CMAKE_CLEAN
        $CMAKE_CONFIGURE_CMD
        $CMAKE_BUILD_CMD
    popd > /dev/null
fi

if [[ "$COMPONENT" = "gxc" ]]; then
    echo -e "\033[0;36mBuilding gxc for $TARGET_DESC\033[0m"
    pushd tools/gxc > /dev/null
        $CMAKE_CLEAN
        $CMAKE_CONFIGURE_CMD
        $CMAKE_BUILD_CMD
    popd > /dev/null
fi

if [[ "$COMPONENT" = "tools" ]]; then
    mkdir -p temp
    cp "tools/fsaw/bin/fsaw$BINEXT" temp
    cp "tools/gxc/bin/gxc$BINEXT" temp
    archive "libsawyer-tools-$ARTEFACT_OS-$PLATFORM$ARTEFACT_EXT" temp
    rm -rf temp
fi
