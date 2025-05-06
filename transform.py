#!/usr/bin/env python3

import numpy as np
from difflib import SequenceMatcher


def match_length(a: list, b: list, value):
    diff = abs(len(a) - len(b))
    if len(a) > len(b):
        b.extend([value] * diff)
    elif len(a) < len(b):
        a.extend([value] * diff)


def main() -> None:
    with open("etiss/etiss_trace.txt", "r", encoding="utf-8") as etiss_trace, open(
        "verilator/trace.txt", "r", encoding="utf-8"
    ) as verilator_trace, open("match.txt", "w", encoding="utf-8") as match_out:
        etiss_asm = []
        etiss_instrs = []
        verilator_asm = []
        for line in verilator_trace:
            split_line = line.strip().split(",")
            if len(split_line) > 3:
                verilator_asm.append(int(split_line[1], 16))

        for line in etiss_trace:
            split_line = line.split(" ")
            if len(split_line) > 3:
                etiss_asm.append(int(split_line[3], 2))
                etiss_instrs.append(split_line[1])

        longest_match = SequenceMatcher(
            None, etiss_asm, verilator_asm
        ).find_longest_match()

        match_start_etiss = longest_match.a
        match_end_etiss = longest_match.a + longest_match.size

        match_start_verilator = longest_match.b
        match_end_verilator = longest_match.b + longest_match.size

        etiss_initial = etiss_asm[:match_start_etiss]
        etiss_instrs_initial = etiss_instrs[:match_start_etiss]
        verilator_initial = verilator_asm[:match_start_verilator]
        match_length(etiss_initial, verilator_initial, -1)

        etiss_trailing = etiss_asm[match_end_etiss:]
        etiss_instrs_trailing = etiss_instrs[match_end_etiss:]
        verilator_trailing = verilator_asm[match_end_verilator:]
        match_length(etiss_trailing, verilator_trailing, -1)

        initial = [
            f"{e_i:8}: {e:08x} {v:08x}\n"
            for (e_i, e, v) in zip(
                etiss_instrs_initial, etiss_initial, verilator_initial
            )
        ]

        matching = [
            f"{ins:8} : {asm:08x}\n"
            for (asm, ins) in zip(
                etiss_asm[match_start_etiss:match_end_etiss],
                etiss_instrs[match_start_etiss:match_end_etiss],
            )
        ]

        trailing = [
            f"{e_i:8}: {e:08x} {v:08x}\n"
            for (e_i, e, v) in zip(
                etiss_instrs_trailing, etiss_trailing, verilator_trailing
            )
        ]

        match_out.write("Initial:\n")
        match_out.writelines(initial)
        match_out.write("Matching:\n")
        match_out.writelines(matching)
        match_out.write("Trailing:\n")
        match_out.writelines(trailing)


if __name__ == "__main__":
    main()
