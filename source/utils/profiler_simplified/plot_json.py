import json
import sys
import os
import numpy as np
import matplotlib.pyplot as plt
import re

# Require at least one JSON file argument
if len(sys.argv) < 2:
    print("Usage: python3 plot_weights.py file1.json file2.json ...")
    sys.exit(1)

json_files = sys.argv[1:]

ranks = []


def getJsonKey(key):
    unit = "MB"
    if key == "dataSize":
        jsonKey = "transport_0"
    elif key == "metadataSize":
        jsonKey = "transport_1"
    else:
        if key.endswith("_mus"):
            jsonKey = key
        else:
            jsonKey = key + "_mus"
        unit = "Sec"
    return jsonKey, unit


def getValue(obj, jsonKey):
    unitSize = 1000000

    if "_mus" in jsonKey:
        w = obj.get(jsonKey)
    elif "transport" in jsonKey:
        if obj.get(jsonKey):
            w = obj.get(jsonKey).get("wbytes")
        else:
            w = 0
        unitSize = 1048576
    else:
        print("Unable to process: ", jsonKey)

    if w:
        return float(w / unitSize)
    else:
        return 0


def retrieve(data, key):
    values = []
    jsonKey, unit = getJsonKey(key)
    for obj in data:
        values.append(getValue(obj, jsonKey))
    return unit, values


def plotKey(key):
    global json_files
    global ranks

    label = key
    for filename in json_files:
        plt.figure(figsize=(10, 4))
        try:
            with open(filename, "r") as f:
                data = json.load(f)
        except Exception as e:
            print(f"Skipping {filename}: {e}")
            continue

        label_name = filename
        if len(filename) > 100:
            label_name = os.path.basename(filename)
        if isinstance(key, str):
            unit, values = retrieve(data, key)
            if len(ranks) == 0:
                ranks = list(range(0, len(values)))
            plt.ylabel(label + " (" + unit + ")")
            plt.plot(ranks, values, marker="o", linestyle="--", label=label_name)

        if isinstance(key, (list, tuple)) and all(isinstance(item, str) for item in key):
            label = ""
            val = np.array([])
            for item in key:
                unit, values = retrieve(data, item)
                if len(val) == 0:
                    val = np.array(values)
                    label = item
                    if len(ranks) == 0:
                        ranks = list(range(0, len(values)))
                else:
                    val += values
                    label += "+" + item
                plt.plot(ranks, val, marker="o", linestyle="--", label=label_name + "_" + label)
            # plt.ylabel(label + " (" + unit + ")") ## label can be tooo long
            plt.ylabel("(" + unit + ")")
        plt.xlabel("Ranks")
        plt.title(" Values Across Ranks")
        plt.xticks(ranks)
        plt.grid(True)
        plt.tight_layout()
        plt.legend()

        output_dir = "python_plots"
        os.makedirs(output_dir, exist_ok=True)
        # Save figure
        output_path = os.path.join(output_dir, label + ".png")
        plt.savefig(output_path)
        print(f"Figure saved to {output_path}")

        plt.show()


def getProfilerComponents(key, data):
    # pattern = re.compile(r"^abc_[^_]+_mus$")
    pattern = re.compile(rf"^{key}_[^_]+_mus$")

    matches = []

    for obj in data:
        if isinstance(obj, dict):
            for key in obj:
                if pattern.match(key):
                    matches.append(key[:-4])  # remove _mus
        break

    return matches


# plot data size written on each rank
plotKey("dataSize")

# plot metadata size written on each rank
plotKey("metadataSize")

# plot end step times on each rank
plotKey("ES")
# plot PDW/PP  times on each rank
plotKey(["PDW", "PP"])
# plot indepent time blocks  on each rank
plotKey(["ES", "PDW", "PP", "BS", "DC"])


####################################################
# one can replace ES with other tags of interest.  #
# we will find all next level subcomponents        #
# and plot out if there is any valid entries       #
####################################################

if len(json_files) == 1:
    print("Exploring different  ES components ")
    testFile = json_files[0]
    with open(testFile) as f:
        data = json.load(f)
    m = getProfilerComponents("ES", data)
    if len(m) > 0:
        plotKey(m)
