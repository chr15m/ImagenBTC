#!/bin/bash

if [ "$1" == "" ]
then
	echo Usage: $0 1BtcAddress
else
	mv src/build_config.h src/build_config_old.h
	grep -v URL src/build_config_old.h > src/build_config.h
	echo '#define' URL \"https://blockchain.info/q/addressbalance/$1\" >> src/build_config.h
	wget "https://blockchain.info/qr?data=$1&size=128" -O resources/src/images/$1.png
	convert resources/src/images/$1.png -resize 128 resources/src/images/background.png
fi
