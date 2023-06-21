source ./activate
make clean
cd vm
make check
sleep 2
echo TEST USER_PROG COMPLETE
cd ..
