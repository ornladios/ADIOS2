This is the testing code for coupling NWChem program with the sorting program. Here are the instructions on how to perform the tests:
1. Activate cheetah environment (e.g. source ~/cheetah/venv-cheetah/bin/activate)
2. sh create-campaign.sh (probably need to change campaign-directory for permission issues)
3. cd campaign-directory && cd user
4. vi run-all.sh and add “./” before campaign-env.sh (line 10)
5. sh run-all.sh
6. When done, use “cheetah generate-report campaign-directory” to generate the attached report

