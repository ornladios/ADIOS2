#!/usr/bin/env python3

# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# generate_pipeline.py
#
#  Created: May 19, 2023
#   Author: Vicente Adolfo Bolea Sanchez <vicente.bolea@kitware.com>

from datetime import datetime
import argparse
import requests
import time
import re
import urllib3
# Remove annoying warning about insecure connection (self-signed cert).
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


def is_date_after(date, days):
    deadline_sec = int(time.time()) - (days * 86400)
    utc_dt = datetime.strptime(date, '%Y-%m-%dT%H:%M:%SZ')
    timestamp_sec = (utc_dt - datetime(1970, 1, 1)).total_seconds()
    return timestamp_sec > deadline_sec


def request_dict(url):
    r = requests.get(url + '?per_page=100', verify=False)
    return r.json()


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


with open(args.template_file, "r") as fd:
    template_str = fd.read()
    gl_url = args.gl_url + "/api/v4/projects/" + str(args.project_id)
    gh_url = 'https://api.github.com/repos/' + args.gh_name
    branches = request_dict(gl_url + "/repository/branches")
    num_pipeline = 0
    for branch in branches:
        # Convert to ISO 8601 date format.
        date_stamp = branch['commit']['committed_date'].split('.')[0] + "Z"
        if num_pipeline < args.max and is_date_after(date_stamp, args.days):
            commit_sha = branch['commit']['id']
            # Backported branches use the merge head
            gh_commit_sha = commit_sha
            if re.fullmatch(r'^pr\d+_.*$', branch['name']):
                gh_commit_sha = branch['commit']['parent_ids'][1]

            # Quit if GitHub does not have the commit
            if 'sha' not in request_dict(gh_url + "/commits/" + gh_commit_sha):
                continue

            # Query GitHub for the status of this commit
            commit = request_dict(gh_url + "/commits/" +
                                  gh_commit_sha + "/status")
            status_found = False
            for status in commit['statuses']:
                if status['context'] == args.gh_context:
                    status_found = True
            if not status_found:
                num_pipeline += 1
                print(template_str.format(
                    branch=branch['name'], commit=commit_sha))
