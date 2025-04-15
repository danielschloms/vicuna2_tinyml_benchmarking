#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Defaults and setup
ARCH="rv32im_zve32x"
VLEN=1024
VLANE_W=32

# Paths
PRJ_DIR=$WS_PATH/vicuna2_tinyml_benchmarking
SRC_BUILD_DIR=$PRJ_DIR/build_custom/build

rm -rf $SRC_BUILD_DIR
mkdir -p $SRC_BUILD_DIR

# Build programs
cd $SRC_BUILD_DIR

SOURCE_FLAGS="-DMIN_VLEN=$VLEN"

if [ "$#" -eq 1 ] && [ $1 = "debug" ]; then
    cmake .. -DRISCV_ARCH=$ARCH $SOURCE_FLAGS -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS_DEBUG="-g -Og" -DCMAKE_EXPORT_COMPILE_COMMANDS=On
elif [ "$#" -eq 0 ] || [ $1 = "release" ]; then
    cmake .. -DRISCV_ARCH=$ARCH $SOURCE_FLAGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=On
else
    echo "Usage: build.sh [debug | release]"
    exit 1
fi

make -j$(nproc)
echo -e "${MAGENTA}Programs done${NC}"

