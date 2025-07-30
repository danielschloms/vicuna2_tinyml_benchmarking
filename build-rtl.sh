#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults and setup
# SW_ARCH="rv32imf_zve32f"
ARCH="rv32imf_zve32f"
VLEN=1024
VLANE_W=32
VMEM_W=32

# Args
TRACE="Off"
TRACE_FULL="Off"
DEBUG="Off"

for arg in "$@"
do
    if [ $arg = "trace" ]; then
        TRACE="On"
        TRACE_FULL="On"
    elif [ $arg = "debug" ]; then
        DEBUG="On"
    else
        echo "Args: [ml, debug, trace]"
        exit 1
    fi
done

MODEL_FLAGS="-DVREG_W=$VLEN -DVLANE_W=$VLANE_W -DVMEM_W=$VMEM_W -DTRACE=$TRACE -DTRACE_FULL=$TRACE_FULL"

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

cmake .. -DRISCV_ARCH=$ARCH $MODEL_FLAGS
make -j$(nproc)
echo -e "${MAGENTA}Model done${NC}"
echo -e "${BLUE}Model Flags:${NC} $MODEL_FLAGS"

# Build programs
# cd $SRC_BUILD_DIR

# MEM_W=32
# SW_TRACE="off"

# SOURCE_FLAGS="-DMIN_VLEN=$VLEN -DMEM_W=$MEM_W -DTRACE=$SW_TRACE"

# if [ "$#" -eq 1 ] && [ $1 = "debug" ]; then
#     cmake .. -DRISCV_ARCH=$SW_ARCH $SOURCE_FLAGS -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS_DEBUG="-g -Og" -DCMAKE_EXPORT_COMPILE_COMMANDS=On
# elif [ "$#" -eq 0 ] || [ $1 = "release" ]; then
#     cmake .. -DRISCV_ARCH=$SW_ARCH $SOURCE_FLAGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=On
# else
#     echo "Usage: build.sh [debug | release]"
#     exit 1
# fi

# make -j$(nproc)
# echo -e "${MAGENTA}Programs done${NC}"

