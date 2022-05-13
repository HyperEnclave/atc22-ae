import os
import csv
import argparse
import numpy as np
import matplotlib.pyplot as plt
from typing import List, Tuple

from common import Variant

parser = argparse.ArgumentParser()
parser.add_argument("--amd-base", "-ab", type=str,  help="AMD baseline result (throughput.csv)")
parser.add_argument("--amd-gu", "-ag", type=str, help="AMD GU-Enclave result (throughput.csv)")
parser.add_argument("--amd-hu", "-ah", type=str, help="AMD HU-Enclave result (throughput.csv)")
parser.add_argument("--intel-base", "-ib", type=str, help="Intel baseline result (throughput.csv)")
parser.add_argument("--intel-sgx", "-is", type=str, help="Intel SGX result (throughput.csv)")
parser.add_argument("--memory", "-m", type=str, help="Memoey usage result (*.csv)")
args = parser.parse_args()


YLIM_AMD = 8500
YLIM_INTEL = 15000


def load_result(fname: str) -> List[Tuple[float, float]]:
    if not fname or not os.path.exists(fname):
        return []
    res = []
    with open(fname, "r") as f:
        reader = csv.reader(f)
        next(reader)
        # keep only the bufsize and the median
        res = [(float(row[0]) / 1000, float(row[-1])) for row in reader]
    res = np.array(res)[1:]  # skip 50K
    return res


def plot(amd_variants: List[Variant], intel_variants: List[Variant],
         xlabel: str, ylabel: str, figname: str):
    fig, axs = plt.subplots(2, 1, figsize=(4, 4))

    legends = {}
    for i, (ax, variants) in enumerate(zip(axs, [amd_variants, intel_variants])):
        ax.set_axisbelow(True)
        ax.grid(True, axis='y', linestyle='dashed', linewidth=0.5)

        slowdowns = Variant.normalize(variants[0], variants)
        for j, v in enumerate(variants):
            if len(v.data) == 0:
                continue
            lines, = ax.plot(v.data[:, 0], v.data[:, 1], v.marker, label=v.label, color=v.color, ms=4, lw=1)
            if not v.label in legends:
                legends[v.label] = lines
            if v.label == "GU-Enclave" or v.label == "SGX":
                for data, sd in zip(v.data[1:], slowdowns[j].data[1:]):
                    x, y, sd = data[0], data[1], sd[1]
                    print("%s: rec_count=%s, throughput=%s, slowdown=%s" % (v.label, x, y, sd))
                    text = "%.2f" % sd
                    fontsize = 8
                    xytext = (0, -12)
                    ax.annotate(text, xy=(x, y), xytext=xytext, fontsize=fontsize,
                                textcoords="offset points", ha='center', va='bottom')

        ax.set_xticks([i * 20 for i in range(0, 11)])
        ax.set_xlim(left=0)

        if i == 0:
            ax.text(100, 65 * 0.81, 'AMD')
            ax.set_ylim(top=65, bottom=0)
            ax.set_yticks([i * 10 for i in range(0, 7)])
        else:
            ax.text(100, 97.5 * 0.81, 'Intel')
            ax.set_ylim(top=97.5, bottom=0)
            ax.set_yticks([i for i in range(0, 98, 15)])


    # plot memory usage
    if args.memory and os.path.exists(args.memory):
        with open(args.memory, "r") as f:
            reader = csv.reader(f)
            # skip first to lines
            next(reader)
            next(reader)
            mem_use = np.array([(int(row[0]) / 1000, int(row[2], 16) / 1024 / 1024) for row in reader])
        print("Memory usage:", mem_use)
        for ax in axs:
            ax.set_frame_on(False)
            ax.set_zorder(1)
        mem_color = plt.get_cmap('Greens')(0.6)
        axs_mem = [axs[0].twinx(), axs[1].twinx()]
        for i, ax in enumerate(axs_mem):
            lines, = ax.plot(mem_use[:, 0], mem_use[:, 1], '.-', color=mem_color, lw=0.5, ms=4)
            ax.set_yticks([0, 100, 200, 300])
            ax.set_ylim(top=325)
            if i == 0:
                legends["Memory usage"] = lines
        fig.text(0.95, 0.35, 'Memory Usage (MB)', fontsize=12, rotation=-90)

    labels = list(legends.keys())
    handlers = list(legends.values())
    if len(labels) >= 4:
        labels[1], labels[2], labels[3] = labels[3], labels[1], labels[2]
        handlers[1], handlers[2], handlers[3] = handlers[3], handlers[1], handlers[2]
    axs[0].legend(handlers, labels, loc='lower center', bbox_to_anchor=(0.5, 1),
                  ncol=3, columnspacing=1, handletextpad=0.5, borderaxespad=0, labelspacing=0.3, frameon=False)
    fig.supylabel(ylabel, x=0.01)
    fig.supxlabel(xlabel, x=0.55, y=0)

    plt.tight_layout()
    plt.subplots_adjust(hspace=0.2, top=0.90, bottom=0.11, left=0.14, right=0.86)
    plt.savefig(figname)
    plt.show()


if __name__ == "__main__":
    print("Plotting %s..." % __file__[:-3])

    print("AMD baseline result: %s" % args.amd_base)
    print("AMD GU-Enclave result: %s" % args.amd_gu)
    print("AMD HU-Enclave result: %s" % args.amd_hu)
    print("Intel baseline result: %s" % args.intel_base)
    print("Intel SGX result: %s" % args.intel_sgx)

    colors = plt.get_cmap("Blues")
    variants = [
        [
            Variant("Baseline", load_result(args.amd_base), colors(0.5), "x--"),
            Variant("GU-Enclave", load_result(args.amd_gu), colors(0.75), "s-"),
            Variant("HU-Enclave", load_result(args.amd_hu), colors(0.95), "o-"),
        ],
        [
            Variant("Baseline", load_result(args.intel_base), colors(0.5), "x--"),
            Variant("SGX", load_result(args.intel_sgx), colors(0.65), "^-"),
        ]
    ]
    print()
    for vv in variants:
        for v in vv:
            print(v)
        print()

    plot(variants[0], variants[1], "Number of records (x1000)", "Throughput (kop/s)", "fig8b_sqlite.pdf")
