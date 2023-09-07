#!/bin/bash
function Usage()
{
    echo "./build.sh <operation>"
    echo "operation:"
    echo "   debug"
    echo "   clean"
}

function SetCMakeMode()
{
    cmake -DCMAKE_BUILD_TYPE:STRINF=$1 -DBUILD_TEST=$2 -DENABLE_COVERAGE=$3 ${project_dir}
    return $?
}

function DoMake()
{
    make clean
    make -j4
    if [ $? -ne 0 ]; then
        echo "make failed, please check!"
        return 1
    fi
    return 0
}

function DoDebug()
{
    SetCMakeMode "Debug" "OFF" "OFF"
    cmake ..
    DoMake
    if [ $? -ne 0 ]; then
        return 1
    fi
    return 0
}

function DoTest()
{
    SetCMakeMode "Debug" "ON" "OFF"
    DoMake
    if [ $? -ne 0 ]; then
        return 1
    fi
    make test
    return 0
}

function DoClean()
{
    rm -rvf bin CMake* CPack* CTest* Make* cmake* compile* lib src test Testing vendor bin *.ninja
    rm -rvf _deps coverage* *.txt *.dat *.gz
    echo "Clean success!"
    return 0
}

function Main()
{
    build_dir=$(pwd)
    readonly build_dir

    project_dir="${build_dir}/.."
    readonly project_dir

    local operation1=$1
    case "$operation1" in
        debug)
            DoDebug
            return $?
            ;;
        clean)
            DoClean
            return $?
            ;;
        *)
            echo "Invalid operation."
            Usage
            return 1
            ;;
        esac
}

if [ $# -ne 1 ]; then
    Usage
    exit 1
else
    Main $@
    exit $?
fi