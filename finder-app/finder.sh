#!/bin/sh
# Finder script for assignment 1
# Author: Bilal Moussa
if [ $# -lt 2 ]
then
	echo "Wrong number of arguments, usage: finder.sh path search_string"
	exit 1
else
	# Check if the directory does NOT exist
	if [ ! -d "$1" ]; then
		echo "The path '$1' does not represent a directory on the filesystem."
	else
		echo "The path '$1' is a valid directory."
		echo "path: $1 searching for:$2"
	fi
fi

# Count the total number of files in the directory and subdirectories
file_count=$(find "$1" -type f | wc -l)

# Count the number of matching lines that contain the search string
matching_lines_count=$(grep -r "$2" "$1" 2>/dev/null | wc -l)

# Print the results
echo "The number of files are $file_count and the number of matching lines are $matching_lines_count."
