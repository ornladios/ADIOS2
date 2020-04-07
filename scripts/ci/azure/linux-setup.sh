#!/bin/bash

if [[ "$SYSTEM_JOBNAME" =~ .*intel.* ]]
then
  echo "Setting up compiler license"
  mkdir -p /opt/intel/licenses
  echo "${INTEL_LICENSE_FILE_CONTENT}" | base64 -d | sudo tee /opt/intel/licenses/license.lic > /dev/null
fi
