#!/bin/bash

REGDB_AUTHOR=allwinner
REGDB_PRIVKEY=${REGDB_AUTHOR}.key.priv.pem
REGDB_PUBKEY=${REGDB_AUTHOR}.key.pub.pem
INPUT_TEXTDB=upstream/db.txt
SHA1_FILE=upstream/sha1sum.txt
SHA1_TEXTDB=$(awk '{print $1}' ${SHA1_FILE})

if [ "$(sha1sum ${INPUT_TEXTDB} | awk '{print $1}')" != "${SHA1_TEXTDB}" ]; then
	echo "${INPUT_TEXTDB} maybe broken, please check!" && \
	exit 1
fi

if [ "$1" == "all" ]; then
	openssl genrsa -out ${REGDB_PRIVKEY} 2048
	openssl rsa -in ${REGDB_PRIVKEY} -out ${REGDB_PUBKEY} -pubout -outform PEM
fi

python -B upstream/db2bin.py regulatory.bin ${INPUT_TEXTDB} ${REGDB_PRIVKEY}
