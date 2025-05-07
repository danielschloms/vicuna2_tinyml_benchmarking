#!/usr/bin/env python3

import numpy as np
import sys
from difflib import SequenceMatcher
import decoder


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
            "Instr    | Asm      | Delta ETISS | Delta RTL   | Diff ETISS - RTL\n"
        )
        outfile.write("-" * 66 + "\n")
        outfile.writelines(matching)
        outfile.write("-" * 66 + "\n")

        if trailing:
            outfile.write("Trailing:\n")
            outfile.write("-" * 39 + "\n")
            outfile.write("ETISS              | Verilator\n")
            outfile.write("-" * 39 + "\n")
            outfile.writelines(trailing)


def read_traces(etiss_trace_path, verilator_trace_path) -> tuple[dict, dict]:
    with open(etiss_trace_path, "r", encoding="utf-8") as etiss_trace, open(
        verilator_trace_path, "r", encoding="utf-8"
    ) as verilator_trace:

        verilator = {"asm": [], "instrs": [], "delta": []}

        etiss = {"asm": [], "instrs": [], "delta": []}

        for line in verilator_trace:
            split_line = line.strip().split(",")
            if len(split_line) > 3:
                verilator["asm"].append(int(split_line[1], 16))
                verilator["delta"].append(int(split_line[3]))
                instr = decoder.decode(bin(int(split_line[1], 16)))
                if instr:
                    verilator["instrs"].append(instr["instr"])
                else:
                    verilator["instrs"].append("unknown")

        for line in etiss_trace:
            split_line = line.split(" ")
            if len(line) > 3:
                etiss["asm"].append(int(split_line[3], 2))
                etiss["instrs"].append(split_line[1])
                # TODO
                etiss["delta"].append(1)

        return (etiss, verilator)


def main() -> None:

    write_trailing = False
    write_initial = False
    if "-t" in sys.argv:
        write_trailing = True
    if "-i" in sys.argv:
        write_initial = True

    etiss, verilator = read_traces("etiss/etiss_trace.txt", "verilator/trace.txt")

    longest_match = SequenceMatcher(
        None, etiss["asm"], verilator["asm"]
    ).find_longest_match()

    match_start_etiss = longest_match.a
    match_end_etiss = longest_match.a + longest_match.size

    match_start_verilator = longest_match.b
    match_end_verilator = longest_match.b + longest_match.size

    matching = [
        f"{ins:8} | {asm:08x} | dE: {d_e:7} | dV: {d_v:7} | Diff: {d_e - d_v}\n"
        for (asm, ins, d_e, d_v) in zip(
            etiss["asm"][match_start_etiss:match_end_etiss],
            etiss["instrs"][match_start_etiss:match_end_etiss],
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

    write_out("match.txt", matching, initial, trailing)


if __name__ == "__main__":
    main()
