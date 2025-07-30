#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

TARGET_SW="bench"
MEM_W=32
MEM_SIZE=805306368 # 0x30000000
# MEM_SIZE=4194304 # 0x00200000
MEM_LATENCY=1
EXTRA_CYCLES=1

# Arguments
if [ "$#" -eq 1 ] && [ "$#" -lt 2 ]; then
    TARGET_SW=$1
elif [ "$#" -gt 1 ]; then
    echo "Usage: run-target.sh [target]"
    exit 1
fi

echo -e "Running ${BLUE}$TARGET_SW${NC}"

# Check if target software exists

# Verilator executable & args
PRJ_DIR="$WS_PATH/vicuna2_tinyml_benchmarking"
MODEL_BUILD_DIR="$PRJ_DIR/build_model/build"
# SW_BUILD_DIR="$PRJ_DIR/build_custom/build/custom_sources"
PROG_DIR="$PRJ_DIR/build_from_other/prog"
PROG_FILE="$PROG_DIR/prog_$TARGET_SW.txt"
TRACE_COMP_DIR="$WS_PATH/vicuna2_tinyml_benchmarking/trace_comparison"
TRACES_DIR="$TRACE_COMP_DIR/verilator"
INSTR_TRACE_FILE="$TRACES_DIR/${TARGET_SW}_trace.txt"
MEM_TRACE_FILE="$TRACES_DIR/${TARGET_SW}_mem_trace.csv"
SIGNAL_TRACE_FILE="$TRACES_DIR/${TARGET_SW}_signal_trace.vcd"

VERILATOR_EXE="$MODEL_BUILD_DIR/verilated_model"
VERILATOR_ARGS="$PROG_FILE $MEM_W $MEM_SIZE $MEM_LATENCY $EXTRA_CYCLES $INSTR_TRACE_FILE $MEM_TRACE_FILE $SIGNAL_TRACE_FILE"
VERILATOR_INVOKE="$VERILATOR_EXE $VERILATOR_ARGS"

eval $VERILATOR_INVOKE
echo -e "${MAGENTA}Done${NC}"

