#!/bin/sh

# set this to the location of the campaign manager script
CAMPAIGN_MANAGER=hpc_campaign_manager.py

python3 ./campaign_many_files.py

python3 ${CAMPAIGN_MANAGER} campaign_many_files.aca delete --campaign  create dataset data*.bp
python3 ${CAMPAIGN_MANAGER} --hostname localhost campaign_many_files_localhost.aca delete --campaign  create dataset data*.bp


python3 ${CAMPAIGN_MANAGER} campaign_many_files_localhost.aca info 
