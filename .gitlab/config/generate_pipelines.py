#!/usr/bin/env python3

# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# generate_pipeline.py
#
#   Author: Vicente Adolfo Bolea Sanchez <vicente.bolea@kitware.com>

from datetime import datetime
import argparse
import requests
import time
import re
import urllib3
# Remove annoying warning about insecure connection (self-signed cert).
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


def request_dict(url):
    r = requests.get(url + '?per_page=100', verify=False)
    return r.json()


def make_timestamp(date_str):
    return int(datetime.strptime(date_str,
                                 '%Y-%m-%dT%H:%M:%S.000%z').timestamp())


def is_date_recent(date):
    deadline_sec = int(time.time()) - (args.days * 86400)
    return date > deadline_sec


def find_status_gh(branch):
    gh_commit_sha = branch['commit']['id']
    # Backported branches use the merge head
    if re.fullmatch(r'^pr\d+_.*$', branch['name']):
        gh_commit_sha = branch['commit']['parent_ids'][1]

    # Query GitHub for the status of this commit
    commit = request_dict(gh_url + "/commits/" + gh_commit_sha + "/status")
    if "sha" not in commit:
        return False

    return all(status != args.gh_context for status in commit['statuses'])


parser = argparse.ArgumentParser(
    prog='generate_pipeline.py',
    description='Generate Dynamic pipelines for Gitlab')
parser.add_argument(
    '-u', '--gl-url', required=True,
    help='Base URL for Gitlab remote. Ex: https://code.olcf.ornl.gov/')
parser.add_argument(
    '-n', '--gh-name', required=True,
    help='Full name of the GitHub project. Ex: ornladios/ADIOS2')
parser.add_argument(
    '-c', '--gh-context', default='OLCF Crusher (Frontier)',
    help='Name of the status in GitHub (A.K.A context)')
parser.add_argument(
    '-p', '--project_id', required=True,
    help='Gitlab internal project ID of the project.')
parser.add_argument(
    '-d', '--days', type=int, default=1,
    help='How many days back to search for commits')
parser.add_argument(
    '-m', '--max', type=int, default=2,
    help='Maximum amount of pipelines computed')
parser.add_argument(
    '-f', '--template_file', required=True,
    help='Template file of the pipeline `{branch}` will be substituted')
args = parser.parse_args()


gl_url = args.gl_url + "/api/v4/projects/" + str(args.project_id)
gh_url = 'https://api.github.com/repos/' + args.gh_name

with open(args.template_file, "r") as fd:
    template_str = fd.read()

    branches = request_dict(gl_url + "/repository/branches")
    for branch in branches:
        branch["dt"] = make_timestamp(branch['commit']['committed_date'])

    branches = filter(lambda x: is_date_recent(x["dt"]), branches)
    branches = sorted(branches, key=lambda x: x["dt"], reverse=True)
    branches = filter(find_status_gh, branches)
    branches = list(branches)[:args.max]

    for branch in branches:
        commit_sha = branch['commit']['id']
        print(template_str.format(branch=branch['name'], commit=commit_sha))
