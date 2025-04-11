#!/usr/bin/bash

# Terminal color
MAGENTA='\033[0;35m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

TARGET_SW="scalar_add"
MEM_W=512
MEM_SIZE=4194304
MEM_LATENCY=1
EXTRA_CYCLES=1

# Arguments
if [ "$#" -eq 1 ] && [ "$#" -lt 2 ]; then
    TARGET_SW=$1
elif [ "$#" -gt 1 ]; then
    echo "Usage: run-target.sh [target]"
    echo "Default target is scalar_add"
    exit 1
fi

# Check if target software exists

# Verilator executable & args
PRJ_DIR="$WS_PATH/vicuna2_tinyml_benchmarking"
MODEL_BUILD_DIR="$PRJ_DIR/build_model/build"
SW_BUILD_DIR="$PRJ_DIR/build_custom/build/custom_sources"
PROG_FILE="$SW_BUILD_DIR/prog_$TARGET_SW.txt"
INSTR_TRACE_FILE="$WS_PATH/traces/verilator/trace.txt"

VERILATOR_EXE="$MODEL_BUILD_DIR/verilated_model"
VERILATOR_ARGS="$PROG_FILE $MEM_W $MEM_SIZE $MEM_LATENCY $EXTRA_CYCLES $INSTR_TRACE_FILE"
VERILATOR_INVOKE="$VERILATOR_EXE $VERILATOR_ARGS"

eval $VERILATOR_INVOKE

echo -e "${MAGENTA}Done${NC}"

