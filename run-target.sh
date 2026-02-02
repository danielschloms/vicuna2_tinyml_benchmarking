#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

MEM_W=32
VLEN=64
VLANE_W=32
MEM_SIZE=805306368 # 0x30000000
# MEM_SIZE=4194304 # 0x00200000
MEM_LATENCY=1
EXTRA_CYCLES=1

ARCH="rv32im_zve32x"
VLEN="64"
VLANE_W="32"
TARGET_SW=""
TIME=""

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
    --target)
      TARGET_SW="$2"
      shift # past argument
      shift # past value
      ;;
    --time)
      TIME="time "
      shift
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

echo -e "Running ${BLUE}$TARGET_SW${NC}"

# Check if target software exists

# Verilator executable & args
PRJ_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ZVL_STRING="zvl${VLEN}b"
VLANE_STRING="vlane${VLANE_W}"
GCC_ARCH_SUBPATH="${ARCH}/${ZVL_STRING}"
HW_ARCH_SUBPATH="${GCC_ARCH_SUBPATH}/${VLANE_STRING}"

MODEL_BUILD_DIR="$PRJ_DIR/build_model/${HW_ARCH_SUBPATH}"
PROG_DIR="$PRJ_DIR/build_from_other/${GCC_ARCH_SUBPATH}/prog"
PROG_FILE="$PROG_DIR/prog_$TARGET_SW.txt"
COMPARISON_DIR="$WS_PATH/Perf_Comparison/comparison"
TRACE_DIR="$COMPARISON_DIR/verilator/${HW_ARCH_SUBPATH}"
mkdir -p $TRACE_DIR

INSTR_TRACE_FILE="$TRACE_DIR/${TARGET_SW}_trace.txt"
# MEM_TRACE_FILE="$TRACE_DIR/${TARGET_SW}_mem_trace.csv"
MEM_TRACE_FILE="/dev/null"
SIGNAL_TRACE_FILE="$TRACE_DIR/${TARGET_SW}_signal_trace.vcd"

VERILATOR_EXE="$MODEL_BUILD_DIR/verilated_model"
VERILATOR_ARGS="$PROG_FILE $MEM_W $MEM_SIZE $MEM_LATENCY $EXTRA_CYCLES $INSTR_TRACE_FILE $MEM_TRACE_FILE $SIGNAL_TRACE_FILE"
# VERILATOR_ARGS="$PROG_FILE $MEM_W $MEM_SIZE $MEM_LATENCY $EXTRA_CYCLES"
VERILATOR_INVOKE="$VERILATOR_EXE $VERILATOR_ARGS"

${TIME}${VERILATOR_INVOKE}
echo -e "${MAGENTA}Done${NC}"

