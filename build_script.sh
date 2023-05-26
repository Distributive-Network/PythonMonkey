# Get number of CPU cores
CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)

DIR="build"
if [  ! -d "$DIR" ]; then
  ### Take action if $DIR exists ###
  mkdir build
fi

cd build
cmake ..
cmake --build . -j$CPUS

# npm is used to load JS components, see package.json
npm i
