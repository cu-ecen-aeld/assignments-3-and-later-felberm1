#!/bin/sh


if [ ! $# -eq 2 ]
	then
		echo "ERROR: Number of arguments wrong: 1. path to directory, 2. text to find in files"
		exit 1
	fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]
	then
		echo "ERROR: $filesdir is not a directory"
		exit 1
	fi

num_files=$(find "$filesdir" -type f | wc -l)

num_match_lines=$(grep -r  "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $num_files and the number of matching lines are $num_match_lines"

exit 0
