// Copyright TU Wien
// Licensed under the Solderpad Hardware License v2.1, see LICENSE.txt for
// details SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

#include "Vvproc_top.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "Vvproc_top_cv32e40x_core__pi1.h"
#include "Vvproc_top_vproc_top.h"
// #include "Vvproc_top_vproc_core__pi2.h"
// #include "Vvproc_top_vproc_decoder__pi9.h"
// #include "Vvproc_top_vproc_decoder__V800_Cb_X20_Dz3.h"
// #include "Vvproc_top_vproc_decoder__V800_Cb_X40_Dz3.h"

#include "verilated.h"

#ifdef TRACE_VCD
#include "verilated_vcd_c.h"
typedef VerilatedVcdC VerilatedTrace_t;
#else
#ifdef TRACE_FST
#include "verilated_fst_c.h"
typedef VerilatedFstC VerilatedTrace_t;
#else
typedef int VerilatedTrace_t;
#endif
#endif

enum class InstructionPrintStyle { llvm, gcc };

template <std::integral T, size_t size> struct ShiftQueue {

  auto get_last() -> T { return buffer[size - 1]; }

  auto set_first(T value) -> void { buffer[0] = value; }

  auto logical_or_first(T value) -> void { buffer[0] |= value; }

  auto shift() -> void {
    std::rotate(buffer.rbegin(), buffer.rbegin() + 1, buffer.rend());
  }

  auto shift_in(T value) -> void {
    std::rotate(buffer.rbegin(), buffer.rbegin() + 1, buffer.rend());
    buffer[0] = value;
  }

private:
  std::array<T, size> buffer{0};
};

template <std::integral T, size_t size, size_t inner_size>
struct ArrayShiftQueue {
  auto get_last() -> std::array<T, inner_size> { return buffer[size - 1]; }

  auto fill_index(unsigned char value, size_t index) -> void {
    std::fill(std::begin(buffer[index]), std::end(buffer[index]), value);
  }

  auto copy_to_index(unsigned char *src, size_t index) -> void {
    std::memcpy(buffer[index].data(), src, inner_size);
  }

  auto copy_from_index(unsigned char *dest, size_t index) -> void {
    std::memcpy(dest, buffer[index].data(), inner_size);
  }

  auto shift() -> void {
    std::rotate(buffer.rbegin(), buffer.rbegin() + 1, buffer.rend());
  }

private:
  std::array<std::array<T, inner_size>, size> buffer{0};
};

// --- Private globals ---

constexpr auto UART_DATA_REGISTER = 0xFF000000u;
constexpr auto UART_STATUS_REGISTER = 0xFF000004u;

constexpr auto MAIN_ADDRESS = 0x00002000u;
constexpr auto FAIL_MISMATCH_ADDRESS = 0x00000078u;
constexpr auto FAIL_INTERRUPT_ADDRESS = 0x000000074u;
constexpr auto SUCCESS_ADDRESS = 0x0000007Cu;

constexpr auto START_TRACE_ADDRESS = MAIN_ADDRESS;
constexpr auto STALL_CYCLES_THRESHOLD = 10'000;

constexpr auto MEMORY_LATENCY = 1; // TODO: This should be a build argument
constexpr auto MEMORY_WIDTH = 32;  // TODO: This should be a build argument

auto inst_trace_file = std::ofstream{};
auto instr_print_style = InstructionPrintStyle::gcc;

uint64_t end_cnt = 0;   // Number of cycles after address 0 was requested
uint64_t abort_cnt = 0; // Number of cycles since mem_req_o last toggled

bool main_reached = true; // Detect if main has been reached to begin
                          // collecting statistics
bool exiting = false;

// - Variables for stall detection -
int current_IF_PC = 0;
int last_IF_PC = 0;
int current_WB_PC = 0;
int last_WB_PC = 0;
int cycles_stalled = 0;

// - Variables for timing -
int instr_to_retire = -1;
int pc_to_retire = -1;

// - Variables for processor metrics -
uint64_t cycles = 0;       // Cycle count
uint64_t instructions = 0; // Instruction Count

// Cycles stalled due to waiting for a result from the XIF interface
int cycles_stalled_XIF = 0;

// Cycles stalled due to a load/store on the XIF interface
int cycles_stalled_XIF_loadstore = 0;

int instr_offloaded_count = 0;
int vector_loads = 0;
int vector_stores = 0;
int other_vector_ops = 0;

// Average vector length calculations
int sum_vec_lengths = 0;
int sum_vec_lengths_bytes = 0;
float sum_vec_percentage = 0.0;
int num_vec_instr = 0;

// Trace begins at this cycle count.
// TODO: expose to the command line
int cycles_begin_trace = 0;

// --- Private function declarations ---

static void log_cycle(Vvproc_top *top, VerilatedTrace_t *tfp, FILE *fcsv);

/**
 * @brief Set the clock high and evaluate the top level design
 */
auto rising_edge(Vvproc_top *top) -> void;

/**
 * @brief Set the clock low and evaluate the top level design
 */
auto falling_edge(Vvproc_top *top) -> void;

/**
 * @brief Print instruction trace information (IF stage)
 *
 * @param pc The current program counter
 * @param instruction The current instruction
 */
auto print_trace(unsigned pc, unsigned instruction, uint64_t cycle) -> void;

/**
 * @brief Check if IF PC indicates a failed program
 *
 * @param pc_if The IF PC
 *
 * @returns true for failure, false otherwise
 */
auto check_for_fail(int pc) -> bool;

auto read_write_mem(Vvproc_top *top, unsigned char *mem,
                    ArrayShiftQueue<unsigned char, MEMORY_LATENCY,
                                    (MEMORY_WIDTH >> 3)> &mem_rdata_queue,
                    unsigned addr) -> void;

auto test_memory_mapped(Vvproc_top *top,
                        ArrayShiftQueue<unsigned char, MEMORY_LATENCY,
                                        (MEMORY_WIDTH >> 3)> &mem_rdata_queue,
                        unsigned addr) -> bool;

auto print_metrics() -> void;

// TODO: name not entirely accurate
auto check_for_stall() -> bool;

auto check_sew(Vvproc_top *top) -> int;

auto check_lmul(int cur_vec_len_bytes, Vvproc_top *top) -> void;

auto check_vector_result(Vvproc_top *top) -> void;

auto write_dump_file(std::string dump_path, unsigned char *mem, int dump_start,
                     int dump_end) -> void;

auto read_program_file(std::string prog_path, unsigned char *mem,
                       int mem_sz) -> bool;

auto write_reference_file(std::string ref_path, int ref_start, int ref_end,
                          unsigned char *mem) -> void;

/**
 * @brief Update cycle and instruction counts, as well as print trace
 * TODO: Trace printing is something else and belongs somewhere else
 */
auto update_counts(Vvproc_top *top, bool inst_trace_out,
                   int mem_req_o_tmp) -> void;

// --- Main ---

int main(int argc, char **argv) {
  fprintf(stderr, "Starting Verilator Main()\n");

  int exit_code = 0;

  // if (argc != 6 && argc != 7 && argc != 8 && argc != 9) {
  if (argc < 6 or argc > 10) {
    fprintf(stderr,
            "Usage: %s PROG_PATHS_LIST MEM_W MEM_SZ MEM_LATENCY EXTRA_CYCLES "
            "[INST_TRACE_FILE] [MEM_TRACE_FILE] [WAVEFORM_FILE]\n",
            argv[0]);
    return 1;
  }

  int csv_out = 0;

  fprintf(stderr, "Hello! Testing: %d\n", argc);

  bool inst_trace_out = false;

  if (argc > 7) {
    csv_out = 1;
  }

  if (argc > 6) {
    inst_trace_out = true;
  }

  int mem_w, mem_sz, mem_latency, extra_cycles;
  {
    char *endptr;
    mem_w = strtol(argv[2], &endptr, 10);
    if (mem_w == 0 || *endptr != 0) {
      fprintf(stderr, "ERROR: invalid MEM_W argument\n");
      return 1;
    }
    mem_sz = strtol(argv[3], &endptr, 10);
    if (mem_sz == 0 || *endptr != 0) {
      fprintf(stderr, "ERROR: invalid MEM_SZ argument\n");
      return 1;
    }
    mem_latency = strtol(argv[4], &endptr, 10);
    if (*endptr != 0) {
      fprintf(stderr, "ERROR: invalid MEM_LATENCY argument\n");
      return 1;
    }
    extra_cycles = strtol(argv[5], &endptr, 10);
    if (*endptr != 0) {
      fprintf(stderr, "ERROR: invalid EXTRA_CYCLES argument\n");
      return 1;
    }
  }

  Verilated::traceEverOn(true);
  // Verilated::commandArgs(argc, argv);

  FILE *fprogs = fopen(argv[1], "r");
  if (fprogs == NULL) {
    fprintf(stderr, "ERROR: opening `%s': %s\n", argv[1], strerror(errno));
    return 2;
  }

  FILE *fcsv;
  if (csv_out == 1) {

    fcsv = fopen(argv[7], "w");
    if (fcsv == NULL) {
      fprintf(stderr, "ERROR: opening `%s': %s\n", argv[7], strerror(errno));
      return 2;
    }
    fprintf(fcsv, "rst_ni;mem_req;mem_addr;pend_vreg_wr_map_o;\n");
  }

  if (inst_trace_out) {
    inst_trace_file.open(argv[6]);
  }

  unsigned char *mem = (unsigned char *)malloc(mem_sz);
  if (mem == NULL) {
    fprintf(stderr, "ERROR: allocating %d bytes of memory: %s\n", mem_sz,
            strerror(errno));
    return 3;
  }

  auto mem_rvalid_queue = ShiftQueue<int32_t, MEMORY_LATENCY>();
  auto mem_rdata_queue =
      ArrayShiftQueue<unsigned char, MEMORY_LATENCY, (MEMORY_WIDTH >> 3)>();
  auto mem_err_queue = ShiftQueue<int32_t, MEMORY_LATENCY>();
  auto mem_ivalid_queue = ShiftQueue<int32_t, MEMORY_LATENCY>();
  auto mem_idata_queue = ShiftQueue<int32_t, MEMORY_LATENCY>();
  auto mem_ierr_queue = ShiftQueue<int32_t, MEMORY_LATENCY>();

  Vvproc_top *top = new Vvproc_top;
  VerilatedTrace_t *tfp = NULL;
#if defined(TRACE_VCD) || defined(TRACE_FST)
  if (argc == 9) {
    tfp = new VerilatedTrace_t;
    top->trace(tfp, 99); // Trace 99 levels of hierarchy
    tfp->open(argv[8]);
  }
#endif

  char *line = NULL, *prog_path = NULL, *ref_path = NULL, *dump_path = NULL;
  size_t line_sz = 0;
  while (getline(&line, &line_sz, fprogs) > 0) {
    // allocate sufficient storage space for the four paths (length of the
    // line, or at least 32 bytes)
    if (line_sz < 32) {
      line_sz = 32;
    }
    prog_path = (char *)realloc(prog_path, line_sz);
    ref_path = (char *)realloc(ref_path, line_sz);
    dump_path = (char *)realloc(dump_path, line_sz);
    strcpy(ref_path, "/dev/null");
    strcpy(dump_path, "/dev/null");

    int ref_start = 0, ref_end = 0, dump_start = 0, dump_end = 0, items;
    items = sscanf(line, "%s %s %x %x %s %x %x", prog_path, ref_path,
                   &ref_start, &ref_end, dump_path, &dump_start, &dump_end);
    if (items == 0 || items == EOF) {
      continue;
    }

    read_program_file(std::string(prog_path), mem, mem_sz);

    write_reference_file(std::string(ref_path), ref_start, ref_end, mem);

    // simulate program execution
    {
      int i;
      top->mem_rvalid_i = 0;
      top->clk_i = 0;
      top->rst_ni = 0;
      for (i = 0; i < 10; i++) {
        rising_edge(top);
        falling_edge(top);
        if (csv_out == 1) {
          log_cycle(top, tfp, fcsv);
        }
      }
      top->rst_ni = 1;
      top->eval();

      while (end_cnt < extra_cycles) {
        // if ABORT_CYCLES is defined, then it specifies the number of cycles
        // after which simulation is aborted in case there is no activity on
        // the memory interface
#ifdef ABORT_CYCLES

        if (abort_cnt >= ABORT_CYCLES) {
          fprintf(stderr,
                  "WARNING: memory interface inactive for %d cycles, "
                  "aborting simulation\n",
                  ABORT_CYCLES);
          exit_code = 1;
          break;
        }

#endif
        // Update last IF_PC
        last_IF_PC = current_IF_PC;
        last_WB_PC = current_WB_PC;

        // Fulfill request on the normal memory port
        // Read memory request
        bool valid = top->mem_addr_o < mem_sz;
        unsigned addr =
            top->mem_addr_o; // remove clearing of bottom address bits. memory
                             // now byte addressible (only works when scalar
                             // core set to work with non-aligned reads)

        if (valid) {
          // Write/read memory content if the address is in the valid range
          read_write_mem(top, mem, mem_rdata_queue, addr);
        } else if (top->mem_req_o) {
          // Test for memory-mapped registers in case of a request for an
          // invalid address
          valid = test_memory_mapped(top, mem_rdata_queue, addr);
        }

        mem_rvalid_queue.set_first(top->mem_req_o);
        mem_err_queue.set_first(!valid);

        // Used to determine when to abort on stall
        // TODO: move to location to be clearer + rename
        int mem_req_o_tmp = top->mem_req_o;

        // Fulfill request on the instruction memory port
        bool valid_instr = top->mem_iaddr_o < mem_sz;

        // Remove clearing of bottom address bits. Memory
        // now byte addressible (only works when scalar
        // core set to work with non-aligned reads)
        unsigned addr_instr = top->mem_iaddr_o;

        if (valid_instr) {
          int32_t idata_value = 0;
          std::memcpy(&idata_value, mem + addr_instr, sizeof(int32_t));
          mem_idata_queue.set_first(idata_value);
        }
        mem_ivalid_queue.set_first(top->mem_ireq_o);
        mem_ierr_queue.set_first(!valid_instr);
        rising_edge(top);

        // Fulfill memory request on main port
        top->mem_rvalid_i = mem_rvalid_queue.get_last();
        unsigned char *mem_port = (unsigned char *)&(top->mem_rdata_i);
        mem_rdata_queue.copy_from_index(mem_port, MEMORY_LATENCY - 1);
        top->mem_err_i = mem_err_queue.get_last();

        // Fullfill memory request on instruction port
        top->mem_irvalid_i = mem_ivalid_queue.get_last();
        top->mem_irdata_i = mem_idata_queue.get_last();
        top->mem_ierr_i = mem_ierr_queue.get_last();
        top->eval();

        // Updating queue for main read port
        mem_rdata_queue.shift();
        mem_rvalid_queue.shift();
        mem_err_queue.shift();

        // Updating queue for instruction read port
        mem_ivalid_queue.shift();
        mem_idata_queue.shift();
        mem_ierr_queue.shift();
        falling_edge(top);

        // Vicuna Linker always puts MAIN (or
        // run_test) at addr 2000.  Wait to check
        // for a stall/abort until this has passed.
        main_reached = (current_WB_PC == START_TRACE_ADDRESS) | main_reached;

        // Need to use PC to exit/abort due to I cache
        current_IF_PC = top->vproc_top->core->pc_if;
        current_WB_PC = top->vproc_top->core->pc_wb;

        //////////
        // Check Exit Conditions
        //////////

        if (check_for_fail(current_WB_PC)) {
          exit_code = 1;
          break;
        }

        // Check for success
        if (end_cnt > 0 || ((top->mem_req_o == 1 || top->mem_ireq_o == 1) &&
                            current_WB_PC == SUCCESS_ADDRESS)) {
          end_cnt++;
          fprintf(stderr, "SUCCESS: TEST PASS - Output Match\n");
          exiting = true;
        }

        // After STALL_CYCLES_THRESHOLD cycles at the same fetch PC, exit
        if (check_for_stall()) {
          exit_code = 1;
          break;
        }

        //////////
        // Outputs + Statistics
        //////////

        // Log File
        if (csv_out == 1 && main_reached && cycles > cycles_begin_trace) {
          // log data once main has been reached and desired start point has
          // been reached
          log_cycle(top, tfp, fcsv);
        }

        // Cycle count and instruction count
        if (main_reached) {
          update_counts(top, inst_trace_out, mem_req_o_tmp);
        }

        // Check if a result from the vector unit is ready and accepted
        // By checking here instead of issue, current VL is correct for
        // vsetvli
        if (top->vproc_top->vcore_result_valid &&
            top->vproc_top->vcore_result_ready && main_reached) {
          check_vector_result(top);
        }
      }

      print_metrics();
    }

    write_dump_file(std::string(dump_path), mem, dump_start, dump_end);
  }

#if defined(TRACE_VCD) || defined(TRACE_FST)
  if (tfp != NULL)
    tfp->close();
#endif
  top->final();
  free(prog_path);
  free(ref_path);
  free(dump_path);
  free(line);
  free(mem);
  if (csv_out == 1) {
    fclose(fcsv);
  }

  fclose(fprogs);

  if (inst_trace_out) {
    // fclose(inst_trace);
    inst_trace_file.close();
  }
  return exit_code;
}

vluint64_t main_time = 0;

// --- Private function definitions ---

auto rising_edge(Vvproc_top *top) -> void {
  top->clk_i = 1;
  top->eval();
}

auto falling_edge(Vvproc_top *top) -> void {
  top->clk_i = 0;
  top->eval();
}

auto print_trace(unsigned pc, unsigned instruction, uint64_t cycle) -> void {

  // auto pc_if = top->vproc_top->core->pc_if;
  // auto instr_if = top->vproc_top->core->instruction_if;
  static uint64_t last_cycle = 0;

  inst_trace_file << std::hex << std::setw(8) << std::setfill('0') << pc
                  << ", ";
  if (instr_print_style == InstructionPrintStyle::gcc) {
    inst_trace_file << std::setw(8) << std::setfill('0') << instruction;
  } else {
    inst_trace_file << std::setw(2) << std::setfill('0') << (instruction & 0xFF)
                    << " " << std::setw(2) << std::setfill('0')
                    << ((instruction & 0xFF00) >> 8) << " " << std::setw(2)
                    << std::setfill('0') << ((instruction & 0xFF0000) >> 16)
                    << " " << std::setw(2) << std::setfill('0')
                    << ((instruction & 0xFF000000) >> 24);
  }
  inst_trace_file << ", " << std::dec << cycle << ", " << cycle - last_cycle
                  << "\n";

  last_cycle = cycle;
}

auto check_for_fail(int pc) -> bool {

  // A jump to address 0x78 is a failed test caused by mismatched output
  if (pc == FAIL_MISMATCH_ADDRESS) {
    fprintf(stderr, "ERROR: TEST FAILURE - Output Mismatch\n");
    return true;
  }

  // A jump to address 0x74 is a failed test caused by an interrupt being
  // called (all other interrupts also funnel here)
  if (pc == FAIL_INTERRUPT_ADDRESS) {
    fprintf(stderr, "ERROR: TEST FAILURE - Interrupt Called\n");
    return true;
  }

  return false;
}

auto read_write_mem(Vvproc_top *top, unsigned char *mem,
                    ArrayShiftQueue<unsigned char, MEMORY_LATENCY,
                                    (MEMORY_WIDTH >> 3)> &mem_rdata_queue,
                    unsigned addr) -> void {
  if (top->mem_req_o && top->mem_we_o) {
    for (int i = 0; i < (MEMORY_WIDTH >> 3); i++) {
      unsigned char *w_port = (unsigned char *)&(top->mem_wdata_o);
      unsigned char *be_port = (unsigned char *)&(top->mem_be_o);
      if ((be_port[i / 8] & (1 << (i % 8)))) {
        mem[addr + i] = w_port[i];
      }
    }
  }

  mem_rdata_queue.copy_to_index(mem + addr, 0);
}

auto test_memory_mapped(Vvproc_top *top,
                        ArrayShiftQueue<unsigned char, MEMORY_LATENCY,
                                        (MEMORY_WIDTH >> 3)> &mem_rdata_queue,
                        unsigned addr) -> bool {

  auto valid = false;
  switch (addr) {
  case UART_DATA_REGISTER:
    valid = true;
    static constexpr auto NO_RECV = 0xf;
    mem_rdata_queue.fill_index(NO_RECV, 0);
    if (top->mem_we_o) {
      uint32_t *w_port = (uint32_t *)&(top->mem_wdata_o);
      putc(*w_port & 0xFF, stdout); // TODO: Verify this still works
    }
    break;

  case UART_STATUS_REGISTER:
    valid = true;
    static constexpr auto TX_READY = 0;
    mem_rdata_queue.fill_index(TX_READY, 0);
    break;

  default:
    break;
  }

  return valid;
}

auto print_metrics() -> void {
  fprintf(stderr, "Total Cycles: %lu\n", cycles);
  fprintf(stderr, "Instruction Count: %lu CPI : %f \n\n", instructions,
          ((float)(cycles)) / ((float)instructions));

  fprintf(stderr, "Number of Vector Instructions Executed: %d  \n",
          num_vec_instr);
  fprintf(stderr, "AVG VL Elements: %f  \n",
          ((float)(sum_vec_lengths)) / ((float)num_vec_instr));
  fprintf(stderr, "AVG VL Bytes: %f  \n\n",
          ((float)(sum_vec_lengths_bytes)) / ((float)num_vec_instr));
  fprintf(stderr, "AVG VREG Usage %%: %f  \n\n",
          ((float)(sum_vec_percentage)) / ((float)num_vec_instr) * 100);

  fprintf(stderr, "Vector Loads     : %d\n", vector_loads);
  fprintf(stderr, "Vector Stores    : %d\n", vector_stores);
  fprintf(stderr, "Other Vector Ops : %d\n", other_vector_ops);
}

auto check_for_stall() -> bool {
  if (current_IF_PC == last_IF_PC) {
    cycles_stalled++;
  } else {
    cycles_stalled = 0;
  }

  if (cycles_stalled >=
      STALL_CYCLES_THRESHOLD) { // TODO: Expose this to the command line
    fprintf(stderr, "ERROR: SIMULATION STALLED FOR %i CYCLES AT IF_PC = 0x%x\n",
            STALL_CYCLES_THRESHOLD, current_IF_PC);
    return true;
  }

  return false;
}

auto check_sew(Vvproc_top *top) -> int {
  // Sew stored in VTYPE bits [5:3]
  static constexpr auto sew_bit_offset = 3;
  static constexpr auto sew_bitmask = 0b111000;
  static constexpr auto min_sew = 8;
  int cur_vec_len_bytes = 0;
  auto sew_bits = (top->vproc_top->csr_vtype_o & sew_bitmask) >> sew_bit_offset;

  if (sew_bits >= 0 and sew_bits <= 2) {
    sum_vec_lengths_bytes += (top->vproc_top->csr_vl_o << sew_bits);
    cur_vec_len_bytes = top->vproc_top->csr_vl_o << sew_bits;
  } else {
    fprintf(stderr, "UNSUPPORTED SEW DETECTED\n");
  }

  return cur_vec_len_bytes;
}

auto check_lmul(int cur_vec_len_bytes, Vvproc_top *top) -> void {
  static constexpr auto lmul_bitmask = 0b111;
  auto lmul = top->vproc_top->csr_vtype_o & lmul_bitmask;
  auto fractional_bit = lmul >> 2;
  auto value_bits = lmul & 0b11;

  if (fractional_bit) {
    // Fractional LMUL:
    // value_bits = 1 ... shift right by 3
    // value_bits = 2 ... shift right by 2
    // value_bits = 3 ... shift right by 1
    auto shift_amount = 3 - (value_bits - 1);
    sum_vec_lengths_bytes +=
        (float)cur_vec_len_bytes /
        (float)(top->vproc_top->csr_vlen_b_o >> shift_amount);
  } else {
    // Multiplicative LMUL
    // value_bits = x ... shift left by x
    auto shift_amount = value_bits;
    sum_vec_lengths_bytes +=
        (float)cur_vec_len_bytes /
        (float)(top->vproc_top->csr_vlen_b_o << shift_amount);
  }
}

auto check_vector_result(Vvproc_top *top) -> void {
  num_vec_instr++;
  // Running sum of number of elements in vectors
  sum_vec_lengths += top->vproc_top->csr_vl_o;

  auto cur_vec_len_bytes = check_sew(top);
  check_lmul(cur_vec_len_bytes, top);
}

auto write_dump_file(std::string dump_path, unsigned char *mem, int dump_start,
                     int dump_end) -> void {

  FILE *ftmp = fopen(dump_path.c_str(), "w");
  if (ftmp == NULL) {
    fprintf(stderr, "ERROR: opening `%s': %s\n", dump_path.c_str(),
            strerror(errno));
  }
  int addr;
  for (addr = dump_start; addr < dump_end; addr += 4) {
    int data = mem[addr] | (mem[addr + 1] << 8) | (mem[addr + 2] << 16) |
               (mem[addr + 3] << 24);
    fprintf(ftmp, "%08x\n", data);
  }
  fclose(ftmp);
}

auto read_program_file(std::string prog_path, unsigned char *mem,
                       int mem_sz) -> bool {
  FILE *ftmp = fopen(prog_path.c_str(), "r");
  if (ftmp == NULL) {
    fprintf(stderr, "WARNING: skipping `%s': %s\n", prog_path.c_str(),
            strerror(errno));
    return false;
  }
  memset(mem, 0, mem_sz);
  char buf[256];
  int addr = 0;
  while (fgets(buf, sizeof(buf), ftmp) != NULL) {
    if (buf[0] == '#' || buf[0] == '/')
      continue;
    char *ptr = buf;
    if (buf[0] == '@') {
      addr = strtol(ptr + 1, &ptr, 16) * 4;
      while (*ptr == ' ')
        ptr++;
    }
    while (*ptr != '\n' && *ptr != 0) {
      int data = strtol(ptr, &ptr, 16);
      int i;
      for (i = 0; i < 4; i++)
        mem[addr + i] = data >> (8 * i);
      addr += 4;
      while (*ptr == ' ')
        ptr++;
    }
  }
  fclose(ftmp);
  return true;
}

auto write_reference_file(std::string ref_path, int ref_start, int ref_end,
                          unsigned char *mem) -> void {
  FILE *ftmp = fopen(ref_path.c_str(), "w");
  if (ftmp == NULL) {
    fprintf(stderr, "ERROR: opening `%s': %s\n", ref_path.c_str(),
            strerror(errno));
  }
  for (int addr = ref_start; addr < ref_end; addr += 4) {
    int data = mem[addr] | (mem[addr + 1] << 8) | (mem[addr + 2] << 16) |
               (mem[addr + 3] << 24);
    fprintf(ftmp, "%08x\n", data);
  }
  fclose(ftmp);
}

auto update_counts(Vvproc_top *top, bool inst_trace_out,
                   int mem_req_o_tmp) -> void {

  abort_cnt = (top->mem_req_o == mem_req_o_tmp) ? abort_cnt + 1 : 0;

  if (!exiting) {
    cycles++;
    if (inst_trace_out and current_WB_PC != last_WB_PC) {
      auto instr = top->vproc_top->core->instruction_wb;
      auto pc = top->vproc_top->core->pc_wb;
      auto opcode = instr & 0b1111111;
      auto ls_width = (instr >> 12) & 0b111;
      static constexpr auto load_fp_opcode = 0x7;
      static constexpr auto store_fp_opcode = 0x27;
      static constexpr auto vector_opcode = 0x57;
      static bool vector_instr_waiting = false;

      // If vector instruction is leaving, print with previous cycle
      if (vector_instr_waiting) {
        int stall = top->vproc_top->core->data_stall_wb;
        print_trace(pc_to_retire, instr_to_retire, cycles - 1);
        vector_instr_waiting = false;
      }

      // Vector loads are differentiated by width, 0 or width > 4 is
      // vector
      auto is_vector_ls =
          (opcode == load_fp_opcode or opcode == store_fp_opcode) and
          (ls_width == 0 or ls_width > 0b100);

      auto is_vset = ((instr >> 12) & 0b111) == 7;

      // Don't print/retire vector loads/stores immediately, wait for
      // next instruction
      if (is_vector_ls or ((opcode == vector_opcode) and not is_vset)) {
        vector_instr_waiting = true;
        instr_to_retire = instr;
        pc_to_retire = pc;
      } else {
        print_trace(pc, instr, cycles);
      }
    }

    if (current_WB_PC != last_WB_PC) {
      instructions++;
    }
  }
}

double sc_time_stamp() { return main_time; }

static void log_cycle(Vvproc_top *top, VerilatedTrace_t *tfp, FILE *fcsv) {
  fprintf(fcsv, "%d;%d;%08X;%08X;%08X;\n", top->rst_ni, top->mem_req_o,
          top->mem_addr_o, top->pend_vreg_wr_map_o, 0);
  main_time++;
#if defined(TRACE_VCD) || defined(TRACE_FST)
  if (tfp != NULL)
    tfp->dump(main_time);
#endif
}
