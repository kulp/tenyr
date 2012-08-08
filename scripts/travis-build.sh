#!/bin/bash
wget http://mirror.pnl.gov/ubuntu//pool/main/b/bison/libbison-dev_2.5.dfsg-2.1_i386.deb
wget http://mirror.pnl.gov/ubuntu//pool/main/b/bison/bison_2.5.dfsg-2.1_i386.deb
dpkg -i libbison-dev_2.5.dfsg-2.1_i386.deb
dpkg -i bison_2.5.dfsg-2.1_i386.deb
make
