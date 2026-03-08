#!/bin/sh

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Add to PYTHONPATH the location of the campaign manager scripts

python3 ./campaign_many_files.py

python3 -m hpc_campaign_manager campaign_many_files.aca delete --campaign  create dataset data*.bp
python3 -m hpc_campaign_manager --hostname localhost campaign_many_files_localhost.aca delete --campaign  create dataset data*.bp


python3 -m hpc_campaign_manager campaign_many_files_localhost.aca info 
