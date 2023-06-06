#!/usr/bin/env python3

import argparse
import glob
from os.path import basename, exists, isdir

#from adios2.adios2_campaign_manager import *


def SetupArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "FILE", help="Name of the input file (.bp, .bp/md.idx, " +
        ".bp/md.0 or .bp/data.XXX)")
    parser.add_argument("--verbose", "-v",
                        help="More verbosity", action="count")
    parser.add_argument("--project", "-p",
                        help="Project name",
                        required=True)
    parser.add_argument("--app", "-a",
                        help="Application name",
                        required=True)
    parser.add_argument("--shot", "-s",
                        help="Shot name",
                        required=True)
    parser.add_argument("--campaign_store", "-c",
                        help="Path to local campaign store",
                        required=True)
    args = parser.parse_args()

    # default values
    args.update = False
    args.CampaignFileName = args.campaign_store+"/" + \
        args.project+"_"+args.app+"_"+args.shot+".acm"
    args.LocalCampaignDir = "adios-campaign"

    # print("Verbosity = {0}".format(args.verbose))
    print(f"Campaign File Name = {args.CampaignFileName}")
    return args


def CheckFileName(args):
    if not exists(args.FILE):
        print("ERROR: File " + args.FILE + " does not exist", flush=True)
        exit(1)


def DumpIndexTableFile(args):
    indexFileList = glob.glob(args.idxFileName)
    if len(indexFileList) > 0:
        DumpIndexTable(indexFileList[0])
    else:
        print("There is  no BP4 Index Table file as " + args.idxFileName)


if __name__ == "__main__":

    args = SetupArgs()
    CheckFileName(args)
    # print(args)

    DumpIndexTableFile(args)
