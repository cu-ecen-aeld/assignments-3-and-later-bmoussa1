#!/bin/sh
# Writer script for assignment 1
# Author: Bilal Moussa

set -e
set -u
WRITESTR=DEFAULT
WRITEDIR=default
username=$(cat ../conf/username.txt)

echo "Calling writer.sh user: $username"

if [ $# -lt 2 ]
then
	echo "Wrong number of arguments, Usage: $0 <full_path_to_file> <write_string>"
	exit 1
else
	# Check if the directory does NOT exist
	# Check if the user has provided the first argument (file path)
	if [ -z "$1" ]; then
		echo "Error: No file path provided."
		echo "Usage: $0 <full_path_to_file> <write_string>"
		exit 1
	else
		echo "The path '$1' is a valid directory."
		echo "Writing: '$2'"
	fi
	WRITESTR=$2
	WRITEDIR=$1
fi

# Check if the file exists
# if [ ! -f "$1" ]; then
    # echo "The file '$1' does not exist. Creating the file."
    # # Create the file
    # touch "$1"
    # # Check if file creation was successful
    # if [ $? -ne 0 ]; then
        # echo "Error: Failed to create the file '$1'."
        # exit 1
    # fi
# fi

# The first argument is the path to the file
echo "$WRITESTR" >> "$WRITEDIR"

# Optionally, show the updated content of the file
echo "Updated content of '$WRITEDIR':"
cat "$WRITEDIR"
