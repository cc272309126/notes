#!/bin/bash

if [ $# -ne 2 ]
then
	echo "Usage: $0 config Makefile"
	exit 1
else
	CFG=$1
	MKFILE=$2
fi

notsetfile=notset.mk
rm -f ${notsetfile}

while IFS='' read -r line || [[ -n "${line}" ]]
do
	optionvar=$(echo ${line} | grep -o "\$(CONFIG_[a-zA-Z0-9_]\+)")
	if [ x"" != x"${optionvar}" ]
	then
		option=$(echo ${optionvar} | grep -o "CONFIG_[a-zA-Z0-9_]\+")
		notset=$(grep "# ${option} is not set" ${CFG})
		if [ x"" != x"${notset}" ]
		then
			echo ${line} >> ${notsetfile}
		fi
	fi
done <  ${MKFILE}
  

hadsetfile=hadset.mk
rm -f ${hadsetfile}

while IFS='' read -r line || [[ -n "${line}" ]]
do
	optionvar=$(echo ${line} | grep -o "\$(CONFIG_[a-zA-Z0-9_]\+)")
	if [ x"" != x"${optionvar}" ]
	then
		option=$(echo ${optionvar} | grep -o "CONFIG_[a-zA-Z0-9_]\+")
		hadset=$(grep "${option}=" ${CFG})
		if [ x"" != x"${hadset}" ]
		then
			echo ${line} >> ${hadsetfile}
		fi
	fi
done <  ${MKFILE}
  
