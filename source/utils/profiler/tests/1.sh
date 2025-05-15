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

source ${script_home}/extract.sh  all $file_name

job_id=single
aggType="ew"
if grep -q "InitAgg-tls" ${file_name}; then
    aggType="tls"
fi

if grep -q "InitAgg-ews" ${file_name}; then
    aggType="ews"
fi

mkdir outs/${time_deco}
mv outs/*_* outs/${time_deco}

base_name=$(basename ${file_name} | cut -d. -f1)

echo "source one.sh ${job_id} ${script_home}  outs ${time_deco} ${aggType} ${base_name}"
source ${script_home}/draw.sh ${job_id} ${script_home}  outs ${time_deco} ${aggType} ${base_name}

echo "Finished. plots are in: plots/${job_id}/${aggType}/${time_deco}"


