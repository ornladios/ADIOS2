#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import csv
import sys
import os
import parseMe
import common


def processFile(inputFile, ax, color, tag):
    data = csv.reader(open(inputFile, "r"), delimiter=",", quotechar="|")

    data1 = np.array([row[0] for row in data], dtype=float)

    idx = np.argmax(data1 < parseMe.command_options[parseMe.TAGS["zero"]])

    if idx == 0:
        idx = data1.size

    if idx > 10:
        ax.plot(data1[0:idx], "--o", color=color)
        ax.set_xticks([0, data1.size - 1])
        # ax.set_xticks([]) #  no x ticks
    else:
        if color == "r":
            ax.plot(range(idx), data1[0:idx], "--o", color=color)
        else:
            ax.plot(range(idx), data1[0:idx], "-x", color=color)
        ax.set_xticks(range(0, idx))
        # print (data1[0:idx])
    ax.set_ylabel(tag, c=color)
    ax.set_xlabel("ranks")
    ax.tick_params(axis="y", labelcolor=color)
    ax.grid()

    if len(logScaleAxis) == 0:
        ax.ticklabel_format(useOffset=False)

    if "x" in logScaleAxis:
        ax.set_xscale("log")

    if "y" in logScaleAxis:
        ax.set_yscale("log")


if __name__ == "__main__":
    print("Script name:", sys.argv[0])
    if len(sys.argv) == 1:
        print("Need Arguments for files")
        sys.exit()

    fig, ax1 = plt.subplots()
    if len(parseMe.args.ioTypes) > 2:
        print("Unable to do 3 way comparison")
        sys.exit()
    elif len(parseMe.args.ioTypes) == 2:
        ax2 = ax1.twinx()

    # note: logscale and ticklabel_format cannot be present together. python error
    logScaleAxis = ""
    if "logScale" in parseMe.command_options:
        logScaleAxis = parseMe.command_options["logScale"]

    jsonAttrStr = parseMe.command_options[parseMe.TAGS["attr"]]
    #  'secs' or  'nCalls' or 'MB'
    whichKind = "secs"
    if "whichKind" in parseMe.command_options:
        whichKind = "MB"
    for i in range(0, len(parseMe.args.ioTypes)):
        ioType = parseMe.args.ioTypes[i]

        inputFile = (
            parseMe.command_options[parseMe.TAGS["input_dir"]] +
            ioType +
            "_" +
            whichKind +
            "_" +
            jsonAttrStr
        )
        currLabel = ioType + " " + jsonAttrStr + " ( " + whichKind + " )"
        if i == 0:
            processFile(inputFile, ax1, "r", currLabel)
        else:
            processFile(inputFile, ax2, "gray", currLabel)

    if len(parseMe.args.ioTypes) > 1:
        levelUp = False

        bottom_ax1, top_ax1 = ax1.get_ylim()
        bottom_ax2, top_ax2 = ax2.get_ylim()

        levelUp = parseMe.command_options[parseMe.TAGS["level"]]
        # else:
        #    if ( max(top_ax1, top_ax2)/min(top_ax1, top_ax2)  < 2):
        #        levelUp = true
        if levelUp:
            ax1.set_ylim(min(bottom_ax1, bottom_ax2), max(top_ax1, top_ax2))
            ax2.set_ylim(min(bottom_ax1, bottom_ax2), max(top_ax1, top_ax2))

    # plt.title('Bar Plot of Two Columns')
    # plt.legend()
    # plt.tight_layout()

    figName = parseMe.command_options[parseMe.TAGS["out"]] + "_" + jsonAttrStr
    figName = figName.replace(".", "_")
    if len(logScaleAxis) > 0:
        figName = figName + "_log_" + logScaleAxis

    common.saveMe(figName)
