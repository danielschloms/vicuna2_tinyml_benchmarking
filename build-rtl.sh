#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
# GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults and setup
ARCH="rv32im_zve32x"
FLOAT_ARCH="rv32imf_zve32f"
VLEN=64
VLANE_W=32
VMEM_W=32

# Args
TRACE="Off"
TRACE_FULL="Off"

# VLEN arg
VLEN_NEXT="false"
VLANE_NEXT="false"
ARCH_NEXT="false"
ARCH_SET="false"
for arg in "$@"; do
    if [ "$VLEN_NEXT" = "true" ]; then
        VLEN="$arg"
        VLEN_NEXT="false"
    elif [ "$VLANE_NEXT" = "true" ]; then
        VLANE_W="$arg"
        VLANE_NEXT="false"
    elif [ "$ARCH_NEXT" = "true" ]; then
        ARCH="$arg"
        ARCH_NEXT="false"
        ARCH_SET="true"
    elif [ "$arg" = "vlen" ]; then
        VLEN_NEXT="true"
    elif [ "$arg" = "vlane" ]; then
        VLANE_NEXT="true"
    elif [ "$arg" = "arch" ]; then
        ARCH_NEXT="true"
    elif [ "$arg" = "trace" ]; then
        TRACE="On"
        TRACE_FULL="On"
    elif [ "$arg" = "float" ]; then
        if [ "$ARCH_SET" = "true" ]; then
            >&2 echo "Setting arch & float!"
            exit 1
        fi
        ARCH=$FLOAT_ARCH
    else
        >&2 echo "Args: [trace, float, vlen <VLEN>, vlane <VLANE_W>, arch <ARCH>]"
        exit 1
    fi
done

MODEL_FLAGS="-DVREG_W=$VLEN -DVLANE_W=$VLANE_W -DVMEM_W=$VMEM_W -DTRACE=$TRACE -DTRACE_FULL=$TRACE_FULL"

# Paths
PRJ_DIR=$WS_PATH/vicuna2_tinyml_benchmarking
CMAKE_SRC_DIR=$PRJ_DIR/build_model
MODEL_BUILD_DIR=$CMAKE_SRC_DIR/$ARCH/zvl${VLEN}b/vlane${VLANE_W}

echo -e "${BLUE}Arch:${NC} $ARCH"
echo -e "${BLUE}Model Flags:${NC} $MODEL_FLAGS"

rm -rf $MODEL_BUILD_DIR

mkdir -p $MODEL_BUILD_DIR

# Build Verilator model
cd "$CMAKE_SRC_DIR" || exit

cmake -S . -B $MODEL_BUILD_DIR -DRISCV_ARCH=$ARCH $MODEL_FLAGS
cd "$MODEL_BUILD_DIR" || exit

make -j$(nproc)
echo -e "${MAGENTA}Model done${NC}"
