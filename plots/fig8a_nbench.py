import os
import argparse
import numpy as np
import matplotlib.pyplot as plt
from typing import List

from common import Variant

parser = argparse.ArgumentParser()
parser.add_argument("--amd-base", "-ab", type=str, help="AMD baseline result (*.res)")
parser.add_argument("--amd-hyper", "-ah", type=str, help="AMD HyperEnclave result (*.res)")
parser.add_argument("--intel-base", "-ib", type=str, help="Intel baseline result (*.res)")
parser.add_argument("--intel-sgx", "-is", type=str, help="Intel SGX result (*.res)")
args = parser.parse_args()

SERIES = [
    "NUMERIC SORT",
    "STRING SORT",
    "BITFIELD",
    "FP EMULATION",
    "FOURIER",
    "ASSIGNMENT",
    "IDEA",
    "HUFFMAN",
    "NEURAL NET",
    "LU DECOMPOSITION",
]


def load_result(fname: str) -> List[float]:
    if not fname or not os.path.exists(fname):
        return []
    res = []
    with open(fname, "r") as f:
        for line in f.readlines():
            data = list(map(lambda s: s.strip(), line.split(":")))
            if len(data) == 4 and data[1] != "" and (data[0] in SERIES or data[0] == ""):
                res.append(float(data[1]))
    return res


def plot(amd_variants: List[Variant], intel_variants: List[Variant], ylabel: str, figname: str):
    fig, axs = plt.subplots(2, 1, figsize=(4, 4))

    total_width = 0.75
    width = total_width / len(amd_variants)
    x = np.arange(len(SERIES))

    legends = {}
    for ax, variants in zip(axs, [amd_variants, intel_variants]):
        ax.set_axisbelow(True)
        ax.grid(True, axis="y", linestyle="dashed", linewidth=0.5)

        ax.set_yticks([i * 0.2 for i in range(0, 7)])
        ax.set_ylim(top=1.4)
        ax.set_xticks(np.arange(len(SERIES)) + 0.3)
        ax.set_xticklabels(SERIES, rotation=30, ha="right", fontsize=8)
        ax.tick_params(axis="x", length=0)
        ax.tick_params(axis="y", direction="in")

        variants = Variant.normalize(variants[0], variants)
        for j, v in enumerate(variants):
            if len(v.data) == 0:
                continue
            rects = ax.bar(x + j * width + (width - total_width) / 2,
                           v.data, width, label=v.label, color=v.color)
            if not v.label in legends:
                legends[v.label] = rects
            if j > 0: # not baseline
                for k, rect in enumerate(rects):
                    text = "%.2f" % v.data[k]
                    xy = (rect.get_x() + rect.get_width() / 2, rect.get_height())
                    xytext = (0, 1 - (j - 1) * 12)
                    ax.annotate(text, xy=xy, xytext=xytext, fontsize=8,
                                textcoords="offset points", ha="center", va="bottom")

    axs[0].text(3.9, 1.235, "AMD")
    axs[1].text(3.9, 1.235, "Intel")
    axs[0].legend(legends.values(), legends.keys(), loc="lower center", bbox_to_anchor=(0.5, 1),
                  ncol=4, columnspacing=1, handletextpad=0.5, borderaxespad=0.2, frameon=False)
    fig.supylabel(ylabel, x=0.01)
    fig.tight_layout()

    plt.subplots_adjust(hspace=0.6, top=0.93, bottom=0.17, right=0.98, left=0.17)
    plt.savefig(figname)
    plt.show()


if __name__ == "__main__":
    print("Plotting %s..." % __file__[:-3])

    print("AMD baseline result: %s" % args.amd_base)
    print("AMD HyperEnclave result: %s" % args.amd_hyper)
    print("Intel baseline result: %s" % args.intel_base)
    print("Intel SGX result: %s" % args.intel_sgx)

    colors = plt.get_cmap("Blues")
    variants = [
        [
            Variant("Baseline", load_result(args.amd_base), colors(0.4)),
            Variant("GU-Enclave", load_result(args.amd_hyper), colors(0.9)),
        ],
        [
            Variant("Baseline", load_result(args.intel_base), colors(0.4)),
            Variant("SGX", load_result(args.intel_sgx), colors(0.7)),
        ]
    ]
    print()
    for vv in variants:
        for v in vv:
            print(v)
        print()

    plot(variants[0], variants[1], "Iterations/sec (normalized)", "fig8a_nbench.pdf")
