
set(RISCV_GCC_PREFIX "$ENV{RISCV_NO_MLIB}/${RISCV_ARCH}")
set(RISCV_GCC_BASENAME "riscv32-unknown-elf")

set(CMAKE_C_COMPILER clang-18)
set(CMAKE_CXX_COMPILER clang-18)
set(CMAKE_ASM_COMPILER clang-18)
set(CMAKE_OBJCOPY llvm-objcopy-18)
set(CMAKE_OBJDUMP llvm-objdump-18)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=riscv32 -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --gcc-toolchain=${RISCV_GCC_PREFIX} --sysroot=${RISCV_GCC_PREFIX}/${RISCV_GCC_BASENAME}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=riscv32 -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=${RISCV_GCC_PREFIX} --sysroot=${RISCV_GCC_PREFIX}/${RISCV_GCC_BASENAME}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --target=riscv32 -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -mcmodel=${RISCV_CMODEL}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --gcc-toolchain=${RISCV_GCC_PREFIX} --sysroot=${RISCV_GCC_PREFIX}/${RISCV_GCC_BASENAME}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -march=${RISCV_ARCH}${RISCV_ARCH_VL} -mabi=${RISCV_ABI} -fuse-ld=lld -mcmodel=${RISCV_CMODEL} -nostartfiles")
