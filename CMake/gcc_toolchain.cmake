cmake_minimum_required(VERSION 3.10)

set(RISCV_TOOLCHAIN_PREFIX "$ENV{RISCV_NO_MLIB}/${RISCV_ARCH}" CACHE STRING "optional prefix for the riscv toolchain in case it is not on the path")
set(RISCV_TOOLCHAIN_BASENAME "riscv32-unknown-elf" CACHE STRING "base name of the toolchain executables")

set(CMAKE_OBJCOPY $ENV{RISCV_NO_MLIB}/${RISCV_ARCH}/bin/${RISCV_TOOLCHAIN_BASENAME}-objcopy)
set(CMAKE_OBJDUMP $ENV{RISCV_NO_MLIB}/${RISCV_ARCH}/bin/${RISCV_TOOLCHAIN_BASENAME}-objdump)

if("${RISCV_TOOLCHAIN_PREFIX}" STREQUAL "")
    set(RISCV_TOOLCHAIN "${RISCV_TOOLCHAIN_BASENAME}")
else()
    set(RISCV_TOOLCHAIN "${RISCV_TOOLCHAIN_PREFIX}/bin/${RISCV_TOOLCHAIN_BASENAME}")
endif()
if(WIN32)
    set(EXE_EXT ".exe")
endif()
set(CMAKE_C_COMPILER "${RISCV_TOOLCHAIN}-gcc${EXE_EXT}")
set(CMAKE_CXX_COMPILER "${RISCV_TOOLCHAIN}-g++${EXE_EXT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__riscv__ -march=${RISCV_ARCH} -mabi=${RISCV_ABI}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__riscv__ -march=${RISCV_ARCH} -mabi=${RISCV_ABI}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -D__riscv__ -march=${RISCV_ARCH} -mabi=${RISCV_ABI}")
set(CMAKE_EXE_LINKER_FLAGS "-march=${RISCV_ARCH} -mabi=${RISCV_ABI}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ${RISCV_ARCH})

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__riscv__ -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__riscv__ -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL}")
# # set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI}")

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL} -nostartfiles")
