!#/bin/bash
sudo apt-get upgrade -y
sudo apt-get update -y
sudo apt-get upgrade -y
sudo apt-get install build-essential -y
sudo apt-get install automake -y
sudo apt-get install libjansson-dev -y
sudo apt-get install libgmp-dev -y
sudo apt-get install libcurl4-openssl-dev -y
sudo apt-get install libssl-dev -y
sudo apt-get install pthreads -y
sudo apt-get install zlib -y
sudo ./build.sh
sudo ./autogen.sh
CFLAGS="-O3 -march=native -Wall" CXXFLAGS="$CFLAGS -std=gnu++11" ./configure --with-curl
make
./cpuminer -a hodl -o stratum+tcp://hodl.optiminer.pl:5555 -u H9G45zE9nLizaUpqWRuTuMkTxBrY52zoqW -p x
