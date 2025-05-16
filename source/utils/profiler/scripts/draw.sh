#!/bin/bash
################################################################
# Usage: ./script.sh <jobID> <scriptsHome> <extractedFilesBase> <timeStep> <aggType>
# Example: ./script.sh 12345 /path/to/scripts /path/to/extracted 0 <aggType>
################################################################

# Validate input parameters
if [ $# -lt 5 ]; then
    echo "Usage: $0 <jobID> <scriptsHome> <extractedFilesBase> <timeStep> <aggType>"
    return 1
fi

jobID="$1"
scriptsHome="$2"
timeStep="$4"
extractedFilesLoc="$3/${timeStep}"
# TLS EWS etc
aggType=$5

key1="default"
key2="flatten"
key3="joined"

if [ $# -ge 6 ]; then
    key1=$6
fi

if [ $# -ge 7 ]; then
   key2=$7
fi
   
if [ $# -ge 8 ]; then
   key3=$8
fi

prefix12=${key1}_${key2}
prefix23=${key2}_${key3}
prefix13=${key1}_${key3}

# Create directory structure
mkdir -p "plots" || { echo "Error: Cannot create plots directory"; return 1; }
mkdir -p "plots/${jobID}" || { echo "Error: Cannot create plots/${jobID}"; return 1; }
mkdir -p "plots/${jobID}/${aggType}" || { echo "Error: Cannot create plots/${jobID}/${aggType}"; return 1; }

currPlotDest="plots/${jobID}/${aggType}/${timeStep}"
mkdir -p "${currPlotDest}" || { echo "Error: Cannot create ${currPlotDest}"; return 1; }


detectContent()
{
    local key count
    key="$1"    
    count=$(find "${extractedFilesLoc}" -maxdepth 1 -type f -name "${key}*" | wc -l)

    if [ "${count}" -gt 0 ]; then
	echo "${count} files has ${key}"
	true; return
    else
	false; return
    fi
    
}

justOne()
{
    local filePrefix type1
    filePrefix=$1
    type1=$2

    echo "python3 ${scriptsHome}/plotRanks.py ${type1}  --set dataDir=${extractedFilesLoc} plotPrefix=${currPlotDest}/${filePrefix} jsonAttr=ES"
    python3 "${scriptsHome}"/plotRanks.py "${type1}"  --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES

    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_AWD
    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_AWD logScale=x
    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_aggregate_info levelAxis=True logScale=x

    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather  logScale=xy
    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather  logScale=y
    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather

    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=MetaInfoBcast

    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=PDW
    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=PP logScale=x

    python3 "${scriptsHome}"/plotRanks.py "${type1}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=transport_0.wbytes  whichKind=MB

    if [[ $type1 == async* ]] ; then
	python3 "${scriptsHome}"/plotRanks.py "${type1}"  --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=BS_WaitOnAsync
	python3 "${scriptsHome}"/plotRanks.py "${type1}"  --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=DC_WaitOnAsync1 logScale=x
	python3 "${scriptsHome}"/plotRanks.py "${type1}"  --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=DC_WaitOnAsync2 logScale=x
    fi

    echo "==> plot all the times spent on rank 0: python3 ${scriptsHome}/plotStack.py  ${type1} ${type2} --set dataDir=${extractedFilesLoc}  whichRank=0 plotPrefix=${currPlotDest}/${filePrefix}"
    python3 "${scriptsHome}"/plotStack.py  "${type1}"  --set dataDir="${extractedFilesLoc}"  whichRank=0 plotPrefix="${currPlotDest}"/"${filePrefix}"
    #fi
}

compareTwo()
{
    local filePrefix type1 type2
    filePrefix=$1
    type1=$2
    type2=$3
    
    echo "python3 ${scriptsHome}/plotRanks.py ${type1} ${type2} --set dataDir=${extractedFilesLoc} plotPrefix=${currPlotDest}/${filePrefix} jsonAttr=ES"
        python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES
    
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_AWD
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_AWD logScale=x
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=ES_aggregate_info levelAxis=True logScale=x

    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather  logScale=xy
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather  logScale=y	
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=FixedMetaInfoGather

    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=MetaInfoBcast

    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=PDW
    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=PP logScale=x

    python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=transport_0.wbytes  whichKind=MB

    if [[ $type1 == async* ]] ; then
	python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=BS_WaitOnAsync    
	python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=DC_WaitOnAsync1 logScale=x
	python3 "${scriptsHome}"/plotRanks.py "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}" jsonAttr=DC_WaitOnAsync2 logScale=x
    fi

    echo "==> plot all the times spent on rank 0: python3 ${scriptsHome}/plotStack.py  ${type1} ${type2} --set dataDir=${extractedFilesLoc}  whichRank=0 plotPrefix=${currPlotDest}/${filePrefix}"
    python3 "${scriptsHome}"/plotStack.py  "${type1}" "${type2}" --set dataDir="${extractedFilesLoc}"  whichRank=0 plotPrefix="${currPlotDest}"/"${filePrefix}"

    #fi
}


compareThree()
{
    local filePrefix type1 type2 type3
    filePrefix=$1
    type1=$2
    type2=$3
    type3=$4
    
    echo "==> plot all the times spent on rank 0"
    python3 "${scriptsHome}"/plotStack.py  "${type1}" "${type2}" "${type3}" --set dataDir="${extractedFilesLoc}"  whichRank=0 plotPrefix="${currPlotDest}"/"${filePrefix}"
    
    echo "==> plot numCalls occurred"
    echo "${scriptsHome}/plotCall.py ${type1} ${type2} ${type3} --set dataDir=${extractedFilesLoc} plotPrefix=${currPlotDest}/${filePrefix}"
    python3 "${scriptsHome}"/plotCall.py "${type1}" "${type2}" "${type3}" --set dataDir="${extractedFilesLoc}" plotPrefix="${currPlotDest}"/"${filePrefix}"
}

checkPossibilities()
{
    local prefix typeA typeB typeC
    prefix=$1
    typeA=${prefix}${key1}
    typeB=${prefix}${key2}
    typeC=${prefix}${key3}



    local trueCounter
    trueCounter=0;
    
    has_key1=false
    has_key2=false
    has_key3=false
    
    if detectContent "$typeA"; then
	has_key1=true
	trueCounter=$((trueCounter + 1))
    fi
    
    if detectContent "$typeB"; then
	has_key2=true
	trueCounter=$((trueCounter + 1))
    fi
    if detectContent "$typeC"; then
	has_key3=true
	trueCounter=$((trueCounter + 1))
    fi

    echo "$prefix counter = $trueCounter, $has_key1 $has_key3 $has_key2"
    if [[ trueCounter -ge 2 ]]; then
	if ($has_key1 && $has_key3); then
	    compareTwo "$prefix13" "$typeA" "$typeC"
	fi
	if ($has_key1 && $has_key2); then
	    compareTwo "$prefix12" "$typeA" "$typeB" 
	fi
	if ($has_key3 && $has_key2); then    
	    compareTwo "$prefix23" "$typeC" "$typeB"
	fi
    fi

    if [[ trueCounter -eq 3 ]]; then 
	compareThree djf "$typeA" "$typeC" "$typeB" 
    fi

    if [[ trueCounter -eq 1 ]]; then
	validType=$typeA
	if ${has_key3}; then
	    validType=$typeC
	elif ${has_key2}; then 
	    validType=$typeB
	fi
	justOne "${validType}" "${validType}"  ## first input is file prefix
    fi
}


checkPossibilities ""
checkPossibilities "async"




