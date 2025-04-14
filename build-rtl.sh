#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Defaults and setup
ARCH="rv32imf_zve32f"
VLEN=1024
VLANE_W=512
# MODEL_FLAGS="-DVLANE_W=$VLANE_W"

# Paths
PRJ_DIR=$WS_PATH/vicuna2_tinyml_benchmarking
MODEL_BUILD_DIR=$PRJ_DIR/build_model/build
SRC_BUILD_DIR=$PRJ_DIR/build_custom/build

rm -rf $MODEL_BUILD_DIR
rm -rf $SRC_BUILD_DIR

mkdir -p $MODEL_BUILD_DIR
mkdir -p $SRC_BUILD_DIR

# Build Verilator model
cd $MODEL_BUILD_DIR

MODEL_FLAGS="-DVREG_W=$VLEN"

cmake .. -DRISCV_ARCH=$ARCH $MODEL_FLAGS
make -j$(nproc)
echo -e "${MAGENTA}Model done${NC}"

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

