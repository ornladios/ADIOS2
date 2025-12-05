#!/bin/bash
####################################
## summarizing for ADIOS writer   ##
####################################
print_bytes() {
	local bytes=${1:-0}
	local precision=${2:-2} # default: 2 decimal places

	if ((bytes < 1024)); then
		printf "%d B\n" "$bytes"
	elif ((bytes < 1048576)); then
		printf "%.${precision}f KiB\n" "$(echo "$bytes / 1024" | bc -l)"
	elif ((bytes < 1073741824)); then
		printf "%.${precision}f MiB\n" "$(echo "$bytes / 1048576" | bc -l)"
	elif ((bytes < 1099511627776)); then
		printf "%.${precision}f GiB\n" "$(echo "$bytes / 1073741824" | bc -l)"
	else
		printf "%.${precision}f TiB\n" "$(echo "$bytes / 1099511627776" | bc -l)"
	fi
}

print_time() {
	local tt=${1:-0}
	local precision=${2:-3} # default: 2 decimal places

	if ((tt < 1000)); then
		printf "%d mus\n" "$tt"
	elif ((tt < 60000000)); then # secs if < 1 minute
		printf "%.${precision}f sec\n" "$(echo "$tt / 1000000" | bc -l)"
	elif ((tt < 600000000)); then # minutes if < 60 minutes
		printf "%.${precision}f min\n" "$(echo "$tt / 60000000" | bc -l)"
	else # >= 1 hr
		printf "%.${precision}f hr\n" "$(echo "$tt/ 3600000000" | bc -l)"
	fi
}

get_min() {
	targetFileName="$1"
	ok=$(jq 'map(. as $obj | {
        rank: $obj.rank,
        sum: ([ $obj.ES_mus, $obj.PDW_mus, $obj.PP_mus ]
              | map(select(type=="number"))
              | add // 0) } ) | min_by(.sum) ' "$targetFileName")
	echo "$ok"
}

get_max() {
	targetFileName="$1"
	ok=$(jq 'map(. as $obj | {
        rank: $obj.rank,
        sum: ([ $obj.ES_mus, $obj.PDW_mus, $obj.PP_mus ]
              | map(select(type=="number"))
              | add // 0) } ) | max_by(.sum) ' "$targetFileName")

	echo "$ok"
}

process_file() {
	targetFileName="$1"

	# Check if the file exists and is readable
	if [[ ! -f "$targetFileName" ]]; then
		echo "Error: File '$targetFileName' does not exist."
		exit 1
	fi

	if [[ ! -r "$targetFileName" ]]; then
		echo "Error: File '$targetFileName' is not readable."
		exit 1
	fi

	rank_count=$(jq '. | length' "${targetFileName}")

	meta_agg_type="TwoLevelAggregationMetadata"

	if grep -q '"ES_MDAgg_AggInfo"' "$targetFileName"; then
		meta_agg_type="SelectiveAggregationMetadata"
	fi

	data_agg_type="EveryoneWrites"
	if grep -q '"InitAgg-ews"' "$targetFileName"; then
		data_agg_type="EveryoneWritesSerial"
	elif grep -q '"InitAgg-tls"' "$targetFileName"; then
		data_agg_type="TwoLevelShared"
	elif grep -q '"InitAgg-dsb"' "$targetFileName"; then
		data_agg_type="DataSizeBased"
	fi

	steps=$(jq -r '.[0].ES.nCalls // empty' "$targetFileName")

	md_bytes=$(jq -r '.[0].transport_1.wbytes// empty' "$targetFileName")

	bytes=$(jq '[ .[] | .transport_0.wbytes | select(type == "number") ] | add // 0' "$targetFileName")

	#max_dur_mus=$(jq 'map([.ES_mus, .PDW_mus, .PP_mus] | map(select(type == "number")) | add // 0) | max // 0' "$targetFileName")
	#min_dur_mus=$(jq 'map([.ES_mus, .PDW_mus, .PP_mus] | map(select(type == "number")) | add // 0) | min // 0' "$targetFileName")
	#r0_dur_mus=$(jq 'map([.ES_mus, .PDW_mus, .PP_mus] | map(select(type == "number")) | add // 0) [0]' "$targetFileName")

	min_dur_info=$(get_min "$targetFileName")
	max_dur_info=$(get_max "$targetFileName")

	echo "=======  High level Summary of ${targetFileName} ======="
	echo "Num Ranks: ${rank_count}. NumSteps: ${steps}"
	echo "   ${data_agg_type} + ${meta_agg_type}"
	echo "      t1  bytes: $(print_bytes "${md_bytes}")"
	echo "      t0  bytes: $(print_bytes "${bytes}")"

	dur="[ES + PP]"
	if grep -q '"PDW"' "$targetFileName"; then
		dur="[ES + PDW]"
	fi

	#echo "   Max ${dur}: = $(print_time $(echo "${max_dur_info}" | jq .sum)) at rank: $(echo "${max_dur_info}" | jq .rank)"
	max_sum=$(jq -r '.sum' <<<"$max_dur_info")
	max_rank=$(jq -r '.rank' <<<"$max_dur_info")
	printf '   Max %s: = %s at rank: %s\n' \
		"$dur" \
		"$(print_time "$max_sum")" \
		"$max_rank"

	#echo "   Min ${dur}: = $(print_time $(echo ${min_dur_info} | jq .sum)) at rank: $(echo ${min_dur_info} | jq .rank)"
	min_sum=$(jq -r '.sum' <<<"$min_dur_info")
	min_rank=$(jq -r '.rank' <<<"$min_dur_info")
	printf '   Min %s: = %s at rank: %s\n' \
		"$dur" \
		"$(print_time "$min_sum")" \
		"$min_rank"

	#echo "   r0  Dur: = $(print_time ${r0_dur_mus})"
}

# main
# Check if exactly one argument is provided
if [[ $# -eq 0 ]]; then
	echo "Usage: $0 <path-to-json-file> [+]"
	echo "Example: $0 data.json"
	exit 1
fi

for file in "$@"; do
	process_file "$file"
done
