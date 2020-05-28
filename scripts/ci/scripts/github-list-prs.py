#!/usr/bin/env python3

import sys
import requests

import argparse
from github import Github

parser = argparse.ArgumentParser(description="List open PRs")
group = parser.add_mutually_exclusive_group()
group.add_argument("-o", "--open", help="List open PRs",
                    action="store_true", default=True)
group.add_argument("-c", "--closed", help="List closed PRs",
                    action="store_true", default=False)
group.add_argument("-a", "--all", help="List all PRs",
                    action="store_true", default=False)
parser.add_argument("repo", help="GitHub repo (org/repo or user/repo)")
args = parser.parse_args()

if args.open:
    state = "open"
elif args.closed:
    state = "closed"
else: #args.all
    state = "all"

try:
    response = requests.get('https://api.github.com/repos/%s/pulls?state=%s' % (args.repo, state))
    response.raise_for_status()
    prs = response.json()
except:
    sys.exit(1)

for pr in prs:
    print("%d %s" % (pr['number'], pr['head']['ref']))
