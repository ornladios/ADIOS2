#!/usr/bin/env python3

import argparse
import glob
import json
import zipfile
from io import StringIO
from os import getcwd, remove
from os.path import basename, exists, isdir
from re import sub
from socket import getfqdn
from subprocess import check_call

# from adios2.adios2_campaign_manager import *


def SetupArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command", help="Command: create/update/delete", choices=['create', 'update', 'delete'])
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
    args.LocalCampaignDir = "adios-campaign/"

    # print("Verbosity = {0}".format(args.verbose))
    print(f"Campaign File Name = {args.CampaignFileName}")
    return args


def CheckCampaignStore(args):
    if not isdir(args.campaign_store):
        print("ERROR: Campaign directory " + args.campaign_store +
              " does not exist", flush=True)
        exit(1)


def CheckLocalCampaignDir(args):
    if not isdir(args.LocalCampaignDir):
        print("ERROR: Shot campaign data '" + args.LocalCampaignDir +
              "' does not exist. Run this command where the code was executed.", flush=True)
        exit(1)


def DeleteFileFromArchive(args: dict, filename: str, campaignArchive: list):
    cmd = ['zip', '-d', args.CampaignFileName] + [filename]
    check_call(cmd)
    # reload the modified archive
    campaignArchive[0] = zipfile.ZipFile(args.CampaignFileName, mode="a",
                                         compression=zipfile.ZIP_DEFLATED, allowZip64=True)


def AddJsonToArchive(args: dict, jsondict: dict, campaignArchive: list, content: list):
    filename = "campaign.json"
    print(f"Add {filename} to archive: {campaignArchive[0].filename}")
    if (filename in content):
        print(f"Found existing {filename} in archive")
        DeleteFileFromArchive(args, filename, campaignArchive)

    jsonStr = json.dumps(jsondict)
    campaignArchive[0].writestr(filename, jsonStr)


def IsADIOSDataset(dataset):
    if not isdir(dataset):
        return False
    if not exists(dataset+"/"+"md.idx"):
        return False
    if not exists(dataset+"/"+"data.0"):
        return False
    return True


def AddFileToArchiveIfItExists(args: dict, filename: str, campaignArchive: list):
    if (exists(filename)):
        campaignArchive[0].write(filename)


def AddDatasetToArchive(args: dict, dataset: str, campaignArchive: list):
    if (IsADIOSDataset(dataset)):
        print(f"Add dataset {dataset} to archive")
        mdFileList = glob.glob(dataset + '/*md.*')
        for f in mdFileList:
            if (exists(f)):
                campaignArchive[0].write(f)
    else:
        print(f"Dataset {dataset} is not an ADIOS dataset. Skip")


def ProcessJsonFile(args, jsonlist, campaignArchive):
    #    with open(jsonfile) as f:
    #        d = json.load(f)
    #print(f"Process {jsondata}:")
    #d = json.load(jsonIO.getvalue())
    for entry in jsonlist:
        print(f"Process entry {entry}:")
        if isinstance(entry, dict):
            if "name" in entry:
                AddDatasetToArchive(args, entry['name'], campaignArchive)

        else:
            print("your object is not a dictionary")


def MergeJsonFiles(jsonfiles: list):
    result = list()
    for f1 in jsonfiles:
        with open(f1, 'r') as infile:
            result.extend(json.load(infile))
    return result


def GetHostName():
    host = getfqdn()
    if host.startswith("login"):
        host = sub('^login[0-9]*\.', '', host)
    if host.startswith("batch"):
        host = sub('^batch[0-9]*\.', '', host)
    shorthost = host.split('.')[0]
    return host, shorthost


if __name__ == "__main__":

    args = SetupArgs()
    CheckCampaignStore(args)

    if (args.command == "delete"):
        if exists(args.CampaignFileName):
            print(f"Delete archive {args.CampaignFileName}")
            remove(args.CampaignFileName)
            exit(0)
        else:
            print(f"ERROR: archive {args.CampaignFileName} does not exist")
            exit(1)

    CheckLocalCampaignDir(args)

    # List the local campaign directory
    jsonFileList = glob.glob(args.LocalCampaignDir + '/*.json')
    if len(jsonFileList) == 0:
        print("There are no campaign data files in  " + args.LocalCampaignDir)
        exit(2)

    if (args.command == "create"):
        print("Create archive")
        mode = 'x'
        if exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} already exist")
            exit(1)
    elif (args.command == "update"):
        print("Update archive is not implemented yet")
        exit(2)
        mode = 'a'
        if not exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} does not exist")
            exit(1)

    campaignArchive = zipfile.ZipFile(args.CampaignFileName, mode=mode,
                                      compression=zipfile.ZIP_DEFLATED, allowZip64=True)
    content = campaignArchive.namelist()

    longHostName, shortHostName = GetHostName()

    # Pass archive in a list in case it has to be reopened (after delete)
    clist = [campaignArchive]
    print(f"Archive list: {content}")
    jsonlist = MergeJsonFiles(jsonFileList)
    jsondict = {
        "id": "ACM",
        "name": "ADIOS Campaign Archive",
        "version": "1.0",
        "stores": [
            {
              "hostname": shortHostName,
              "longhostname": longHostName,
              "rootdir": getcwd(),
              "files": jsonlist
            }
        ]}

    print(f"Merged json = {jsondict}")
    AddJsonToArchive(args, jsondict, clist, content)
    ProcessJsonFile(args, jsonlist, clist)
#    for jsonfile in jsonFileList:
#        AddJsonToArchive(args, jsonfile, clist, content)
#        ProcessJsonFile(args, jsonfile, clist)

    campaignArchive.close()
