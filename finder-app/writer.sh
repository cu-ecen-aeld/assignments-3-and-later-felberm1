#!/bin/bash

if [ ! $# -eq 2 ]
	then
		echo "ERROR: Number of arguments wrong: 1. path to file including filename, 2. text to write in file"
		exit 1
	fi

writefile=$1
writestr=$2

mkdir -p  "$(dirname "$writefile")"
echo "$writestr" > "$writefile"

if [ $? -ne 0 ]
	then
		echo "ERROR: file creation failed"
		exit 1
	fi

exit 0
