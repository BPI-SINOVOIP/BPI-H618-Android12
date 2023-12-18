#!/bin/bash

generate_hash_header(){
    # Get the current directory commit
	local _commitHash=`git log -n 1 --abbrev=10 --pretty=format:"%h" -- $1`
	if [[ -n $(git diff --stat $1) ]]
	then
		echo "#define CI_INFO \"$_commitHash-dirty\""
	else
		echo "#define CI_INFO \"$_commitHash\""
	fi
}

generate_hash_header $1
