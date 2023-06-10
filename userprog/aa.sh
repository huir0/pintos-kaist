cd ..
source ./activate
cd userprog
make clean
make
cd build
pintos -v -k -T 300 -m 20   --fs-disk=10 -p tests/filesys/base/syn-read:syn-read -p tests/filesys/base/child-syn-read:child-syn-read -- -q   -f run syn-read < /dev/null 2> tests/filesys/base/syn-read.errors > tests/filesys/base/syn-read.output
perl -I../.. ../../tests/filesys/base/syn-read.ck tests/filesys/base/syn-read tests/filesys/base/syn-read.result