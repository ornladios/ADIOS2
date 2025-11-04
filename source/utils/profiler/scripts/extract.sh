#!/bin/bash
###########################################################################
## sample usage:
## > source extract.sh ES ../jsons/testMe_default ../jsons/testMe_flatten
###########################################################################
outDir=outs
mkdir -p ${outDir}


processFile() {
    local filePath="$1"
    local attrName="$2"


    BASE_NAME=$(basename "$filePath" | cut -d. -f1)
    local key=${BASE_NAME}
    if [[ "$filePath" == *"flatten"* ]]; then
	key="flatten"
    fi
    if [[ "$filePath" == *"default"* ]]; then
	key="default"
    fi
    if [[ "$filePath" == *"joined"* ]]; then
	key="joined"
    fi
    local asyncKey=""
    if [[ "$filePath" == *"async"* ]]; then
	asyncKey="async"
    fi

    echo "Processing $filePath, $attrName key= ${asyncKey}${key}"

    if [[ $attrName == *bytes* ]]; then
	jq -r ".[] | .$attrName"  "$filePath" | awk '{print $1/1048576}' > "${outDir}/${asyncKey}${key}_MB_${attrName}"
    else
	local attrMus="${attrName}_mus"
	local attrNCalls="${attrName}.nCalls"

	#echo "both $attrMus  $attrNCalls"
	jq -r ".[] | .$attrMus" "$filePath" | awk '{print $1/1000000}' >  "${outDir}/${asyncKey}${key}_secs_${attrName}"

	## use awk to divide by 1 to make sure nulls becomes 0 (e.g. if PDW is not present, jq returns null, awk makes it 0)
	jq -r ".[] | .$attrNCalls" "$filePath" | awk '{print $1/1}'> "outs/${asyncKey}${key}_nCalls_${attrName}"
    fi
}

if ((  $# <  2 )); then
    echo "Expecting: $0 jsonProperty file1 .."
else
    #numFiles=$(($# - 1))
    #echo "Num Files: $numFiles"

    if [[ $1 == "all" ]]; then
	knownAttrs=( 'PP' 'PDW' 'ES' 'ES_AWD' 'ES_aggregate_info' 'MetaInfoBcast' 'FixedMetaInfoGather' 'transport_0.wbytes' )
	asyncAttrs=( 'BS_WaitOnAsync' 'DC_WaitOnAsync1' 'DC_WaitOnAsync2' )
    else
	knownAttrs=("$1")
    fi
    echo "Attributes: ${knownAttrs[*]}"

    args=("$@")
    for ((i = 1; i < $#; i++ )); do
	#currFile=$argv[i]
	currFile="${args[$((i))]}"
	for currAttr in "${knownAttrs[@]}"; do
	    processFile "$currFile" "$currAttr"
    done
	if [[ $currFile == *async* ]]; then
	    for tmp in "${asyncAttrs[@]}"; do
		echo "async file: currFile = $currFile, $tmp"
		processFile "$currFile" "$tmp"
	    done
	fi
    done
fi
