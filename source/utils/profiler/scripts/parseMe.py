#!/usr/bin/env python3

import argparse
import textwrap
import os

parser = argparse.ArgumentParser(description="ADIOS json file parser inputs")

TAGS = {
    "input_dir": "dataDir",  # key name for input direcory is "dataDir"
    "rank": "whichRank",  # which rank to peek profile
    "out": "plotPrefix",  # for output file name
    "showfig": "showFig",  # call plt.show or not
    "attr": "jsonAttr",  # key refer to specific json attr
    # the "zeroif" key refers to a min number to be treated as 0.
    # will process data until first rank <= this value is spotted
    "zero": "zeroIf",
    "level": "levelAxis",
}

TAGS_DEFAULT = {
    "input_dir": "outs/",  # key name for input direcory defaults to "outs/"
    "rank": 0,  # by default observe rank 0 activites for plotStack.py
    "out": "plot",  # output dir
    "showfig": "False",  # do not call plt.show by default
    "attr": "ES",  # default json attr to look at is ES
    "zero": "0.000001",  # ignore if smaller than
    "level": "False",  # whether left/right axises (if both exist) should display same range
}

parser.add_argument(
    "ioTypes",
    nargs="+",
    help="A list of known adios types in the output directory.\n"
    "Will be accessed as [" + TAGS["input_dir"] + "]/[ioType]_<secs/nCalls>_[profileTag]"
    "e.g. outs/flatten_nCalls_ES",
)

helpWithDefaults = ""
for k in TAGS:
    helpWithDefaults += TAGS[k] + "(default: " + str(TAGS_DEFAULT[k]) + ")  "

parser.add_argument(
    "--set", metavar="key=value", nargs="+", help="Accepted options:\n" + helpWithDefaults
)
args = parser.parse_args()

# print (args.set)
# print (args.file_key)


def parse_var(s):
    """
    Parse a key, value pair, separated by '=' into a tuple
    """
    items = s.split("=")
    key = items[0].strip()  # we remove blanks around keys, as is logical
    if len(items) > 1:
        # rejoin the rest:
        value = "=".join(items[1:])
    return (key, value)


def parse_vars(items):
    """
    Parse a series of key-value pairs and return a dictionary
    """
    d = {}

    if items:
        for item in items:
            key, value = parse_var(item)
            d[key] = value
    return d


# parse the key-value pairs

command_options = parse_vars(args.set)

for k in TAGS:
    if TAGS[k] not in command_options:
        command_options[TAGS[k]] = TAGS_DEFAULT[k]

if not command_options[TAGS["input_dir"]].endswith("/"):
    command_options[TAGS["input_dir"]] += "/"
if not os.path.isdir(command_options[TAGS["input_dir"]]):
    parser.error(f"Input directory '{command_options[TAGS['input_dir']]}' does not exist")

# command_options[TAGS["rank"]] = int(command_options[TAGS["rank"]])
# command_options[TAGS["zero"]] = float(command_options[TAGS["zero"]])
try:
    command_options[TAGS["rank"]] = int(command_options[TAGS["rank"]])
    if command_options[TAGS["rank"]] < 0:
        parser.error(f"Rank ({command_options[TAGS['rank']]}) must be non-negative")
except ValueError:
    parser.error(f"Rank must be an integer, got '{command_options[TAGS['rank']]}'")

try:
    command_options[TAGS["zero"]] = float(command_options[TAGS["zero"]])
    if command_options[TAGS["zero"]] < 0:
        parser.error(f"Zero threshold ({command_options[TAGS['zero']]}) must be non-negative")
except ValueError:
    parser.error(f"Zero threshold must be a number, got '{command_options[TAGS['zero']]}'")


# Safely convert levelAxis to boolean
level_val = command_options[TAGS["level"]].lower()
if level_val not in ("true", "false"):
    parser.error(f"Level must be 'True' or 'False', got '{command_options[TAGS['level']]}'")
command_options[TAGS["level"]] = level_val == "true"


showFig_val = command_options[TAGS["showfig"]].lower()
if showFig_val not in ("true", "false"):
    parser.error(f"showFig must be 'True' or 'False', got '{command_options[TAGS['level']]}'")
command_options[TAGS["showfig"]] = showFig_val == "true"

# print (command_options)
