#!/usr/bin/env python3

import numpy as np
import sys
from difflib import SequenceMatcher
import decoder
import csv
import os


class TerminalColors:
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


V_ARITH_INSTRS = ["vadd_vv", "vsetvli"]
TARGET_SW = "bench"
START_LABEL = "address_match_start"
END_LABEL = "address_match_end"

VERILATOR_DUMP_DIR = (
    f"{os.environ["WS_PATH"]}/vicuna2_tinyml_benchmarking/build_from_other/vector"
)
VERILATOR_DUMP_FILE = f"{VERILATOR_DUMP_DIR}/{TARGET_SW}_dump.txt"

ETISS_DUMP_DIR = (
    f"{os.environ["WS_PATH"]}/gen_perfsim/target_sw/examples/Vicuna/custom/dump"
)
ETISS_DUMP_FILE = f"{ETISS_DUMP_DIR}/{TARGET_SW}.dump"


def read_addresses() -> dict:
    """Returns a dictionary with start and end addresses."""
    verilator_start = 0
    verilator_end = 0
    etiss_start = 0
    etiss_end = 0
    with open(VERILATOR_DUMP_FILE) as verilator_dump:
        for line in verilator_dump:
            if f"<{START_LABEL}>:" in line:
                verilator_start = int(line.split(" ")[0], 16)
            if f"<{END_LABEL}>:" in line:
                verilator_end = int(line.split(" ")[0], 16)

    with open(ETISS_DUMP_FILE) as etiss_dump:
        for line in etiss_dump:
            if f"<{START_LABEL}>:" in line:
                etiss_start = int(line.split(" ")[0], 16)
            if f"<{END_LABEL}>:" in line:
                etiss_end = int(line.split(" ")[0], 16)

    print(f"RTL Start Address: {verilator_start:08x}")
    print(f"RTL End Address: {verilator_end:08x}")
    print(f"ETISS Start Address: {etiss_start:08x}")
    print(f"ETISS End Address: {etiss_end:08x}")

    return {
        "e_start": etiss_start,
        "e_end": etiss_end,
        "v_start": verilator_start,
        "v_end": verilator_end,
    }


def match_length(a: list, b: list, value) -> None:
    diff = abs(len(a) - len(b))
    if len(a) > len(b):
        b.extend([value] * diff)
    elif len(a) < len(b):
        a.extend([value] * diff)


def write_out(
    outfile_path,
    matching: list[str],
    initial: list[str] | None = None,
    trailing: list[str] | None = None,
    etiss_cycles: tuple[int, int] = (0, 0),
    verilator_cycles: int = 0,
) -> None:
    with open(outfile_path, "w", encoding="utf-8") as outfile:

        if initial:
            outfile.write("Initial:\n")
            outfile.write("-" * 39 + "\n")
            outfile.write("ETISS              | Verilator\n")
            outfile.write("-" * 39 + "\n")
            outfile.writelines(initial)
            outfile.write("-" * 39 + "\n")

        outfile.write("Matching:\n")
        outfile.write("-" * 66 + "\n")
        outfile.write(
            "Instr E  | Asm E    | Instr V  | Asm V    | Delta ETISS | Delta RTL   | Diff ETISS - RTL\n"
        )
        outfile.write("-" * 66 + "\n")
        outfile.writelines(matching)
        outfile.write("-" * 66 + "\n")
        outfile.write(
            f"ETISS WB cycles:   {etiss_cycles[0]} | RTL cycles: {verilator_cycles} | Diff: {etiss_cycles[0] - verilator_cycles}\n"
        )
        outfile.write(
            f"ETISS DISP cycles: {etiss_cycles[1]} | RTL cycles: {verilator_cycles} | Diff: {etiss_cycles[1] - verilator_cycles}\n"
        )
        outfile.write("-" * 66 + "\n")

        if trailing:
            outfile.write("Trailing:\n")
            outfile.write("-" * 39 + "\n")
            outfile.write("ETISS              | Verilator\n")
            outfile.write("-" * 39 + "\n")
            outfile.writelines(trailing)


def read_traces(etiss_trace_path, verilator_trace_path, addrs) -> tuple[dict, dict]:
    with open(verilator_trace_path, "r", encoding="utf-8") as verilator_trace, open(
        "verilator/trace_t", "w", encoding="utf-8"
    ) as new_trace_v:

        verilator = {
            "asm": [],
            "instrs": [],
            "delta": [],
            "cycles": [],
            "start": 0,
            "end": 0,
        }

        verilator_index = 0
        for line in verilator_trace:
            split_line = line.strip().split(",")
            if len(split_line) > 3:
                pc = int(split_line[0], base=16)
                # print(f"{pc:08x}")
                if pc == addrs["v_start"]:
                    print(f"Matched Verilator start: {verilator_index}")
                    verilator["start"] = verilator_index

                if pc == addrs["v_end"]:
                    print(f"Matched Verilator end: {verilator_index}")
                    verilator["end"] = verilator_index

                verilator["asm"].append(int(split_line[1], base=16))
                verilator["delta"].append(int(split_line[3]))
                verilator["cycles"].append(int(split_line[2]))
                instr = decoder.decode(f"{int(split_line[1], 16):032b}")
                if instr:
                    verilator["instrs"].append(instr["instr"])
                    new_trace_v.write(f"{instr["instr"]}, {line}")
                else:
                    verilator["instrs"].append("unknown")
                    new_trace_v.write(f"v_instr, {line}")

                verilator_index += 1

    with open(etiss_trace_path, "r", encoding="utf-8") as etiss_trace, open(
        "etiss/trace_t.txt", "w", encoding="utf-8"
    ) as new_trace_e:

        etiss = {
            "asm": [],
            "instrs": [],
            "delta": [],
            "wb_cycles": [],
            "disp_cycles": [],
            "cycles": 0,
            "start": 0,
            "end": 0,
        }

        etiss_index = 0
        for line in etiss_trace:
            split_line = line.split(" ")
            if len(line) > 3:
                pc = int(split_line[0][2:].lstrip("0")[:-1], base=16)
                if pc == addrs["e_start"]:
                    print(f"Matched ETISS start: {etiss_index}")
                    etiss["start"] = etiss_index
                if pc == addrs["e_end"]:
                    print(f"Matched ETISS end: {etiss_index}")
                    etiss["end"] = etiss_index
                etiss["asm"].append(int(split_line[3], 2))
                etiss["instrs"].append(split_line[1])
                hex_asm = f"{int(split_line[3], 2):08x}"
                split_line[3] = hex_asm
                new_trace_e.write(" ".join(split_line))

                etiss_index += 1
            else:
                print(line)

    with open("etiss/etiss_timing.csv") as etiss_timing:
        reader = csv.reader(etiss_timing)
        previous = 0
        index = 0
        index_wb = -1
        index_disp = 3
        for row in reader:
            if row[0] == "IF_stage":
                continue

            row_i = index_wb
            try:
                if etiss["instrs"][index] in V_ARITH_INSTRS:
                    # Take DISP time if completion is signaled from Dispatch stage
                    # WB time otherwise
                    row_i = index_disp
            except:
                print(f"Error index {index}")
                exit(1)

            disp_cycles = int(row[index_disp])
            wb_cycles = int(row[index_wb])
            cycles = int(row[row_i])
            etiss["wb_cycles"].append(wb_cycles)
            etiss["disp_cycles"].append(disp_cycles)
            etiss["delta"].append(cycles - previous)
            previous = cycles
            index += 1

        etiss["cycles"] = previous

    return (etiss, verilator)


def main() -> None:

    write_trailing = False
    write_initial = False
    if "-t" in sys.argv:
        write_trailing = True
    if "-i" in sys.argv:
        write_initial = True

    etiss, verilator = read_traces(
        "etiss/etiss_asm.txt", "verilator/trace.txt", read_addresses()
    )

    longest_match = SequenceMatcher(
        None, etiss["asm"], verilator["asm"]
    ).find_longest_match()

    print(f"(SequenceMatcher) Matched {longest_match.size} instructions")
    print(
        f"(AddressMatcher) Verilator: ({verilator["start"]}, {verilator["end"]}): {verilator["end"] - verilator["start"]} Instructions"
    )
    print(
        f"(AddressMatcher) ETISS: ({etiss["start"]}, {etiss["end"]}): {etiss["end"] - etiss["start"]} Instructions"
    )
    # read_addresses()

    # match_start_etiss = longest_match.a
    # match_end_etiss = longest_match.a + longest_match.size

    # match_start_verilator = longest_match.b
    # match_end_verilator = longest_match.b + longest_match.size

    # match_size = verilator["end"] - verilator["start"]

    # print(f"Verilator: Start = {verilator["start"]}, End = {verilator["end"]}")
    # print(f"ETISS: Start = {etiss["start"]}, End = {etiss["start"] + match_size}")

    match_start_etiss = etiss["start"]
    match_end_etiss = etiss["end"]

    match_start_verilator = verilator["start"]
    match_end_verilator = verilator["end"]

    sum_diffs = sum(
        [
            d_e - d_v
            for d_e, d_v in zip(
                etiss["delta"][match_start_etiss:match_end_etiss],
                verilator["delta"][match_start_verilator:match_end_verilator],
            )
        ]
    )

    print(f"Sum of differences: {sum_diffs}")
    print(
        f"Final ETISS cycles: WB: {etiss["wb_cycles"][-1]}, DISP: {etiss["disp_cycles"][-1]}"
    )

    eq = {True: TerminalColors.OKGREEN, False: TerminalColors.FAIL}

    matching = [
        (
            f"{ins_e:8} |"
            f" {asm_e:08x} |"
            f" {ins_v:8} |"
            f" {asm_v:08x} |"
            f" dE: {d_e:7} |"
            f" dV: {d_v:7} |"
            f" Diff: {d_e - d_v:3}"
            f"{" (A!)" if asm_e != asm_v else ""}"
            f"{" (I!)" if ins_e != ins_v else ""}"
            f"{" (D!)" if d_e != d_v else ""}"
            f"\n"
        )
        for (asm_e, ins_e, asm_v, ins_v, d_e, d_v) in zip(
            etiss["asm"][match_start_etiss:match_end_etiss],
            etiss["instrs"][match_start_etiss:match_end_etiss],
            verilator["asm"][match_start_verilator:match_end_verilator],
            verilator["instrs"][match_start_verilator:match_end_verilator],
            etiss["delta"][match_start_etiss:match_end_etiss],
            verilator["delta"][match_start_verilator:match_end_verilator],
        )
    ]

    initial = None
    trailing = None

    if write_initial:
        etiss_initial = etiss["asm"][:match_start_etiss]
        etiss_instrs_initial = etiss["instrs"][:match_start_etiss]
        verilator_initial = verilator["asm"][:match_start_verilator]
        verilator_instrs_initial = verilator["instrs"][:match_start_verilator]
        match_length(etiss_initial, verilator_initial, -1)
        match_length(etiss_instrs_initial, verilator_instrs_initial, "-")

        initial = [
            f"{e_i:8}: {e:08x} | {v_i:8}: {v:08x}\n"
            for (e_i, e, v_i, v) in zip(
                etiss_instrs_initial,
                etiss_initial,
                verilator_instrs_initial,
                verilator_initial,
            )
        ]

    if write_trailing:
        etiss_trailing = etiss["asm"][match_end_etiss:]
        etiss_instrs_trailing = etiss["instrs"][match_end_etiss:]
        verilator_trailing = verilator["asm"][match_end_verilator:]
        verilator_instrs_trailing = verilator["instrs"][match_end_verilator:]
        match_length(etiss_trailing, verilator_trailing, -1)
        match_length(etiss_instrs_trailing, verilator_instrs_trailing, "-")

        trailing = [
            f"{e_i:8}: {e:08x} | {v_i:8}: {v:08x}\n"
            for (e_i, e, v_i, v) in zip(
                etiss_instrs_trailing,
                etiss_trailing,
                verilator_instrs_trailing,
                verilator_trailing,
            )
        ]

    etiss_cycles = (
        etiss["wb_cycles"][match_end_etiss - 1] - etiss["wb_cycles"][match_start_etiss],
        etiss["disp_cycles"][match_end_etiss - 1]
        - etiss["disp_cycles"][match_start_etiss],
    )

    write_out(
        "match.txt",
        matching,
        initial,
        trailing,
        etiss_cycles,
        verilator["cycles"][match_end_verilator - 1]
        - verilator["cycles"][match_start_verilator],
    )


if __name__ == "__main__":
    main()
