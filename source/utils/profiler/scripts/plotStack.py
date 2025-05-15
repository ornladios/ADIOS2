#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import csv
import sys
import os
import parseMe
import common


def drawBars(ax, labels, summary, timeDesc):
    """Draw stacked bar chart for ADIOS timing data."""
    width = 0.6
    bottom = np.zeros(len(labels))

    # Stack bars for each keyword
    for key in ["ES", "PP", "PDW"]:
        if summary[key]:  # Only plot if data exists
            ax.bar(labels, summary[key], width, bottom=bottom, label=key)
            bottom += np.array(summary[key])  # Update bottom for stacking

    # Separate bars for other metrics (not stacked)
    ax.bar(labels, summary["ES_aggregate_info"], width / 2, label="ES_aggregate_info")
    ax.bar(
        labels, summary["ES_AWD"], width / 2, bottom=summary["ES_aggregate_info"], label="ES_AWD"
    )
    ax.bar(labels, summary["FixedMetaInfoGather"], width / 3, label="FixedMetaInfoGather")

    ax.set_ylabel("Seconds")
    ax.set_title(f"ADIOS Time: {timeDesc}")
    ax.legend()


def summarizeFile(inputFile, whichRank=0):
    """Read CSV file and return value for specified rank or max."""
    data = csv.reader(open(inputFile, "r"), delimiter=",", quotechar="|")

    data1 = np.array([row[0] for row in data], dtype=float)

    if whichRank < 0:
        return np.max(data1)
    elif whichRank < len(data1):
        return data1[whichRank]
    else:
        return data1[0]


if __name__ == "__main__":
    print("Script name:", sys.argv[0])
    if len(sys.argv) < 2:
        print("Need Arguments for files")
        sys.exit()

    keywords = [
        "PP",
        "PDW",
        "ES",
        "ES_AWD",
        "ES_aggregate_info",
        "MetaInfoBcast",
        "FixedMetaInfoGather",
    ]
    summary = {}
    for key in keywords:
        summary[key] = []

    barLabels = []
    fig, ax1 = plt.subplots()

    # print (parseMe.args.ioTypes)
    whichRank = parseMe.command_options[parseMe.TAGS["rank"]]
    # for i in range(0, len(parseMe.args.ioTypes)):
    # which=sys.argv[i] ## default flatten joined
    #    which=parseMe.args.ioTypes[i]
    for which in parseMe.args.ioTypes:
        barLabels.append(which)
        for key in keywords:
            inputFile = parseMe.command_options[parseMe.TAGS["input_dir"]] + which + "_secs_" + key
            if os.path.exists(inputFile):
                summary[key].append(summarizeFile(inputFile, whichRank))
            else:
                print("No such file: " + inputFile)

    # print ("summary = ", summary)
    # print (barLabels)

    if len(summary["ES"]) == 0:
        print("Nothing to plot")
        sys.exit()

    timeDesc = "Max" if whichRank < 0 else f"Rank {whichRank}"
    drawBars(ax1, barLabels, summary, timeDesc)

    figName = parseMe.command_options[parseMe.TAGS["out"]]
    if figName.endswith("/"):
        figName += "max" if whichRank < 0 else f"rank_{whichRank}"
    else:
        figName += "_max" if whichRank < 0 else f"_rank_{whichRank}"
    figName += ".png"  # Add extension

    common.saveMe(figName)
