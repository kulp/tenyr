#!/bin/bash
mirror=http://mirror.pnl.gov/ubuntu/

wget $mirror/pool/main/b/bison/libbison-dev_2.5.dfsg-2.1_i386.deb \
     $mirror/pool/main/b/bison/bison_2.5.dfsg-2.1_i386.deb \
     $mirror/pool/main/e/eglibc/libc-bin_2.15-0ubuntu10_i386.deb \
     $mirror/pool/main/e/eglibc/libc6_2.15-0ubuntu10_i386.deb

sudo dpkg -i libc-bin_2.15-0ubuntu10_i386.deb libc6_2.15-0ubuntu10_i386.deb \
             libbison-dev_2.5.dfsg-2.1_i386.deb bison_2.5.dfsg-2.1_i386.deb

make MAKE_VERBOSE=1
