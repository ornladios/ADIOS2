#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import csv
import sys
import os
import common
import parseMe

# Sample data


def plotBars(ax, ticklabels, summary, timeDesc=""):
    barWidth = 0.3
    dataSize = len(summary[parseMe.args.ioTypes[0]])
    br = {0: np.arange(dataSize)}
    for i in range(1, dataSize):
        br[i] = [x + barWidth for x in br[i - 1]]

    for i, which in enumerate(parseMe.args.ioTypes):
        ax.bar(br[i], summary[which], 0.3, label=which)

    ax.set_ylabel("NumCalls")
    ax.set_title("ADIOS numCalls:" + timeDesc)

    ax.set_xticks([r + barWidth for r in range(dataSize)], ticklabels)

    # Add legend
    ax.legend()


def summarizeFile(inputFile):
    try:
        with open(inputFile, "r") as f:
            data = csv.reader(f, delimiter=",", quotechar="|")
            data1 = [float(row[0]) for row in data if row]  # Handle empty rows
            return int(data1[0]) if data1 else 0  # num of calls are the same for all ranks
    except (FileNotFoundError, IndexError, ValueError) as e:
        print(f"Warning: Could not process {inputFile}: {e}")
        return 0


if __name__ == "__main__":
    print("Script name:", sys.argv[0])
    if len(sys.argv) < 2:
        print("Need Arguments for files")
        sys.exit()

    found_async = [item for item in parseMe.args.ioTypes if "async" in item.lower()]

    if found_async:
        keywords = ["PP", "PDW", "ES", "BS_WaitOnAsync", "DC_WaitOnAsync1", "DC_WaitOnAsync2"]
        ticklabels = [
            "PP",
            "PDW",
            "ES",
            "BS\nWaitOn\nAsync",
            "DC\nWaitOn\nAsync1",
            "DC\nWaitOn\nAsync2",
        ]
    else:
        keywords = ["PP", "PDW", "ES"]
        ticklabels = ["PP", "PDW", "ES"]

    summary = {key: [] for key in parseMe.args.ioTypes}
    # barLabels = list(parseMe.args.ioTypes)

    fig, ax1 = plt.subplots()

    for which in parseMe.args.ioTypes:
        for key in keywords:
            inputFile = (
                parseMe.command_options[parseMe.TAGS["input_dir"]] + which + "_nCalls_" + key
            )
            if os.path.exists(inputFile):
                summary[which].append(summarizeFile(inputFile))
            else:
                summary[which].append(0)

    if len(summary[parseMe.args.ioTypes[0]]) == 0:
        print("Nothing to plot")
        sys.exit()

    plotBars(ax1, ticklabels, summary)
    plt.legend()

    # Show plot
    figName = parseMe.command_options[parseMe.TAGS["out"]] + "_nCalls"
    common.saveMe(figName)
