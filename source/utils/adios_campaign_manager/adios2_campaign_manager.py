#!/usr/bin/env python3

import argparse
import glob
import json
import sqlite3
import zlib
from datetime import datetime
from os import chdir, getcwd, remove, stat
from os.path import exists, isdir
from re import sub
from socket import getfqdn
from time import time

# from adios2.adios2_campaign_manager import *

ADIOS_ACA_VERSION = "1.0"

def SetupArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command", help="Command: create/update/delete",
        choices=['create', 'update', 'delete', 'info'])
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
    parser.add_argument("--hostname", "-n",
                        help="Host name unique for hosts in a campaign",
                        required=False)
    args = parser.parse_args()

    # default values
    args.update = False
    args.CampaignFileName = args.campaign_store + "/" + \
        args.project + "_" + args.app + "_" + args.shot + ".aca"
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


def IsADIOSDataset(dataset):
    if not isdir(dataset):
        return False
    if not exists(dataset + "/" + "md.idx"):
        return False
    if not exists(dataset + "/" + "data.0"):
        return False
    return True


def compressFile(f):
    compObj = zlib.compressobj()
    compressed = bytearray()
    blocksize = 1073741824  # 1GB #1024*1048576
    len_orig = 0
    len_compressed = 0
    block = f.read(blocksize)
    while block:
        len_orig += len(block)
        cBlock = compObj.compress(block)
        compressed += cBlock
        len_compressed += len(cBlock)
        block = f.read(blocksize)
    cBlock = compObj.flush()
    compressed += cBlock
    len_compressed += len(cBlock)

    return compressed, len_orig, len_compressed


def decompressBuffer(buf: bytearray):
    data = zlib.decompress(buf)
    return data


def AddFileToArchive(args: dict, filename: str, cur: sqlite3.Cursor, dsID: int):
    compressed = 1
    try:
        f = open(filename, 'rb')
        compressed_data, len_orig, len_compressed = compressFile(f)

    except IOError:
        print(f"ERROR While reading file {filename}")
        return

    statres = stat(filename)
    ct = statres.st_ctime

    cur.execute('insert into bpfile values (?, ?, ?, ?, ?, ?, ?)',
                (dsID, filename, compressed, len_orig, len_compressed, ct, compressed_data))
    con.commit()

    # test
    # if (filename == "dataAll.bp/md.0"):
    #    data = decompressBuffer(compressed_data)
    #    of = open("dataAll.bp-md.0", "wb")
    #    of.write(data)
    #    of.close()


def AddDatasetToArchive(args: dict, dataset: str, cur: sqlite3.Cursor, hostID: int, dirID: int):
    if (IsADIOSDataset(dataset)):
        print(f"Add dataset {dataset} to archive")
        statres = stat(dataset)
        ct = statres.st_ctime
        curDS = cur.execute('insert into bpdataset values (?, ?, ?, ?)',
                            (hostID, dirID, dataset, ct))
        dsID = curDS.lastrowid
        cwd = getcwd()
        chdir(dataset)
        mdFileList = glob.glob('*md.*')
        profileList = glob.glob('profiling.json')
        files = mdFileList + profileList
        for f in files:
            AddFileToArchive(args, f, cur, dsID)
        chdir(cwd)
    else:
        print(f"WARNING: Dataset {dataset} is not an ADIOS dataset. Skip")


def ProcessJsonFile(args: dict, jsonlist: list, cur: sqlite3.Cursor, hostID: int, dirID: int):
    for entry in jsonlist:
        # print(f"Process entry {entry}:")
        if isinstance(entry, dict):
            if "name" in entry:
                AddDatasetToArchive(
                    args, entry['name'], cur, hostID, dirID)
        else:
            print(f"WARNING: your object is not a dictionary, skip : {entry}")


def GetHostName():
    host = getfqdn()
    if host.startswith("login"):
        host = sub('^login[0-9]*\\.', '', host)
    if host.startswith("batch"):
        host = sub('^batch[0-9]*\\.', '', host)
    shorthost = host.split('.')[0]
    return host, shorthost


def AddHostName(longHostName, shortHostName):
    res = cur.execute(
        'select rowid from host where hostname = "' + shortHostName + '"')
    row = res.fetchone()
    if row is not None:
        hostID = row[0]
        print(f"Found host {shortHostName} in database, rowid = {hostID}")
    else:
        curHost = cur.execute('insert into host values (?, ?)',
                              (shortHostName, longHostName))
        hostID = curHost.lastrowid
        print(f"Inserted host {shortHostName} into database, rowid = {hostID}")
    return hostID


def Info(args: dict, cur: sqlite3.Cursor):
    res = cur.execute('select id, name, version, ctime from info')
    info = res.fetchone()
    t = datetime.fromtimestamp(float(info[3]))
    print(f"{info[1]}, version {info[2]}, created on {t}")

    res = cur.execute('select rowid, hostname, longhostname from host')
    hosts = res.fetchall()
    for host in hosts:
        print(f"hostname = {host[1]}   longhostname = {host[2]}")
        res2 = cur.execute(
            'select rowid, name from directory where hostid = "' + str(host[0]) + '"')
        dirs = res2.fetchall()
        for dir in dirs:
            print(f"    dir = {dir[1]}")
            res3 = cur.execute(
                'select rowid, name, ctime from bpdataset where hostid = "' + str(host[0]) +
                '" and dirid = "' + str(dir[0]) + '"')
            bpdatasets = res3.fetchall()
            for bpdataset in bpdatasets:
                t = datetime.fromtimestamp(float(bpdataset[2]))
                print(f"        dataset = {bpdataset[1]}     created on {t}")


def Update(args: dict, cur: sqlite3.Cursor):
    longHostName, shortHostName = GetHostName()
    if args.hostname is not None:
        shortHostName = args.hostname

    hostID = AddHostName(longHostName, shortHostName)

    rootdir = getcwd()
    # curHost = cur.execute('insert into host values (?, ?)',
    #                      (shortHostName, longHostName))
    # hostID = curHost.lastrowid

    curDir = cur.execute('insert into directory values (?, ?)',
                         (hostID, rootdir))
    dirID = curDir.lastrowid
    con.commit()

    jsonlist = MergeJsonFiles(jsonFileList)

    # print(f"Merged json = {jsonlist}")
    ProcessJsonFile(args, jsonlist, cur, hostID, dirID)

    con.commit()


def Create(args: dict, cur: sqlite3.Cursor):
    epoch = int(time())
    cur.execute(
        "create table info(id TEXT, name TEXT, version TEXT, ctime INT)")
    cur.execute('insert into info values (?, ?, ?, ?)',
                ("ACA", "ADIOS Campaign Archive", ADIOS_ACA_VERSION, epoch))
    cur.execute("create table host" +
                "(hostname TEXT PRIMARY KEY, longhostname TEXT)")
    cur.execute("create table directory" +
                "(hostid INT, name TEXT, PRIMARY KEY (hostid, name))")
    cur.execute("create table bpdataset" +
                "(hostid INT, dirid INT, name TEXT, ctime INT" +
                ", PRIMARY KEY (hostid, dirid, name))")
    cur.execute("create table bpfile" +
                "(bpdatasetid INT, name TEXT, compression INT, lenorig INT" +
                ", lencompressed INT, ctime INT, data BLOB" +
                ", PRIMARY KEY (bpdatasetid, name))")
    Update(args, cur)


def MergeJsonFiles(jsonfiles: list):
    result = list()
    for f1 in jsonfiles:
        with open(f1, 'r') as infile:
            result.extend(json.load(infile))
    return result


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

    if (args.command == "create"):
        print("Create archive")
        if exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} already exist")
            exit(1)
    elif (args.command == "update" or args.command == 'info'):
        print(f"{args.command} archive")
        if not exists(args.CampaignFileName):
            print(f"ERROR: archive {args.CampaignFileName} does not exist")
            exit(1)

    con = sqlite3.connect(args.CampaignFileName)
    cur = con.cursor()

    if (args.command == "info"):
        Info(args, cur)
    else:
        CheckLocalCampaignDir(args)
        # List the local campaign directory
        jsonFileList = glob.glob(args.LocalCampaignDir + '/*.json')
        if len(jsonFileList) == 0:
            print("There are no campaign data files in  " + args.LocalCampaignDir)
            exit(2)

        if (args.command == "create"):
            Create(args, cur)
        elif (args.command == "update"):
            Update(args, cur)

    cur.close()
    con.close()
