#!/bin/bash
####################################################################################
# usage:
#   takes two parmeter: a timestep descrption, and a adios profiler file
#   e.g. ./script.sh script/dir t0 path/to/profile
####################################################################################
if [ $# -ne 3 ]; then
    echo "Usage: $0 <script_dir> <timestep_description> <profile_file>"
    exit 1
fi

script_home=$1
time_deco=$2
file_name=$3

# Validate files
if [ ! -f "${script_home}/extract.sh" ]; then
    echo "Error: extract.sh not found in ${script_home}"
    exit 1
fi
if [ ! -f "${script_home}/draw.sh" ]; then
    echo "Error: draw.sh not found in ${script_home}"
    exit 1
fi
if [ ! -f "${file_name}" ]; then
    echo "Error: Profile file '${file_name}' not found"
    exit 1
fi

# shellcheck source=/dev/null
source "${script_home}"/extract.sh  all "$file_name" || {
    echo "Error: Failed to source ${script_home}/extract.sh"
    exit 1
}

job_id=single
aggType="ew"
if grep -q "InitAgg-tls" "${file_name}"; then
    aggType="tls"
fi

if grep -q "InitAgg-ews" "${file_name}"; then
    aggType="ews"
fi

mkdir outs/"${time_deco}"
mv outs/*_* outs/"${time_deco}"

base_name=$(basename "${file_name}" | cut -d. -f1)

echo "Data extracted, now plotting.."
# shellcheck source=/dev/null
source "${script_home}"/draw.sh "${job_id}" "${script_home}"  outs "${time_deco}" "${aggType}" "${base_name}" || {
    echo "Error: Failed to source ${script_home}/draw.sh"
    exit 1
}

echo "Finished. plots are in: plots/${job_id}/${aggType}/${time_deco}"


