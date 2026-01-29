#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
# GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Defaults and setup
ARCH="rv32im_zve32x"
VLEN=64
VLANE_W=32
VMEM_W=32
CLEAN_BEFORE="false"
GENERATOR="Ninja"

# Args
TRACE="Off"
TRACE_FULL="Off"

while [[ $# -gt 0 ]]; do
  case $1 in
    --arch)
      ARCH="$2"
      shift # past argument
      shift # past value
      ;;
    --vlen)
      VLEN="$2"
      shift # past argument
      shift # past value
      ;;
    --vlane_width)
      VLANE_W="$2"
      shift # past argument
      shift # past value
      ;;
    --trace)
      TRACE="On"
      TRACE_FULL="On"
      shift # past argument
      ;;
    --clean)
      CLEAN_BEFORE="true"
      shift # past argument
      ;;
    -*|--*)
      >&2 echo "Unknown option $1"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

# Paths
VICUNA_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_ROOT_DIR="$(dirname "$VICUNA_DIR")"
VERILATOR_DIR=$PROJECT_ROOT_DIR/Third_Party/Verilator
CMAKE_SRC_DIR=$VICUNA_DIR/build_model
MODEL_BUILD_DIR=$CMAKE_SRC_DIR/$ARCH/zvl${VLEN}b/vlane${VLANE_W}

export VERILATOR_DIR=$VERILATOR_DIR

echo -e "${BLUE}Arch:${NC} $ARCH"
echo -e "${BLUE}Model Flags:${NC} $MODEL_FLAGS"

if [[ "$CLEAN_BEFORE" = "true" ]]; then
    rm -rf $MODEL_BUILD_DIR
fi
mkdir -p $MODEL_BUILD_DIR

cmake -S $CMAKE_SRC_DIR -B $MODEL_BUILD_DIR \
    -DRISCV_ARCH=$ARCH \
    -DVREG_W=$VLEN \
    -DVLANE_W=$VLANE_W \
    -DVMEM_W=$VMEM_W \
    -DTRACE=$TRACE \
    -DTRACE_FULL=$TRACE_FULL \
    -G "$GENERATOR"

cmake --build $MODEL_BUILD_DIR -j$(nproc)

echo -e "${MAGENTA}Model done${NC}"
