#!/bin/bash

if [ "$1" == "" ]
then
	echo Usage: $0 1BtcAddress
else
	mv src/build_config.h src/build_config_old.h
	grep -v ADDRESS src/build_config_old.h > src/build_config.h
	echo '#define' ADDRESS \"$1\" >> src/build_config.h
	qrencode -s 4 "bitcoin:$1" -o $1-raw.png
	convert $1-raw.png -gravity center -extent 144x168 -background white resources/src/images/background.png
fi
