#! /bin/bash
#
# @file         build.sh
# @author       Giovanni Tedesco <giovanni@distributive.network>
# @date         Aug 2022

cd `dirname "$0"`
topDir=`pwd`

# Get number of CPU cores
CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)

DIR="build"
if [  ! -d "$DIR" ]; then
  ### Take action if $DIR exists ###
  mkdir build
fi

cd build
if [[ "$OSTYPE" == "msys"* ]]; then # Windows
  cmake .. -T ClangCL # use Clang/LLVM toolset for Visual Studio
else
  cmake .. 
fi
cmake --build . -j$CPUS --config Release

# npm is used to load JS components, see package.json
cd "${topDir}/python/pythonmonkey/"
npm i
