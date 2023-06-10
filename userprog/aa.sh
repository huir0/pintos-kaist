cd ..
source ./activate
cd userprog
make clean
make
cd build
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/exec-read:exec-read -p ../../tests/userprog/sample.txt:sample.txt -p tests/userprog/child-read:child-read -- -q   -f run exec-read < /dev/null 2> tests/userprog/exec-read.errors > tests/userprog/exec-read.output
perl -I../.. ../../tests/userprog/exec-read.ck tests/userprog/exec-read tests/userprog/exec-read.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/multi-child-fd:multi-child-fd -p ../../tests/userprog/sample.txt:sample.txt -p tests/userprog/child-close:child-close -- -q   -f run multi-child-fd < /dev/null 2> tests/userprog/multi-child-fd.errors > tests/userprog/multi-child-fd.output
perl -I../.. ../../tests/userprog/multi-child-fd.ck tests/userprog/multi-child-fd tests/userprog/multi-child-fd.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/rox-child:rox-child -p tests/userprog/child-rox:child-rox -- -q   -f run rox-child < /dev/null 2> tests/userprog/rox-child.errors > tests/userprog/rox-child.output
perl -I../.. ../../tests/userprog/rox-child.ck tests/userprog/rox-child tests/userprog/rox-child.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/rox-multichild:rox-multichild -p tests/userprog/child-rox:child-rox -- -q   -f run rox-multichild < /dev/null 2> tests/userprog/rox-multichild.errors > tests/userprog/rox-multichild.output
perl -I../.. ../../tests/userprog/rox-multichild.ck tests/userprog/rox-multichild tests/userprog/rox-multichild.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/filesys/base/lg-random:lg-random -- -q   -f run lg-random < /dev/null 2> tests/filesys/base/lg-random.errors > tests/filesys/base/lg-random.output
perl -I../.. ../../tests/filesys/base/lg-random.ck tests/filesys/base/lg-random tests/filesys/base/lg-random.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/filesys/base/sm-random:sm-random -- -q   -f run sm-random < /dev/null 2> tests/filesys/base/sm-random.errors > tests/filesys/base/sm-random.output
perl -I../.. ../../tests/filesys/base/sm-random.ck tests/filesys/base/sm-random tests/filesys/base/sm-random.result
pintos -v -k -T 300 -m 20   --fs-disk=10 -p tests/filesys/base/syn-read:syn-read -p tests/filesys/base/child-syn-read:child-syn-read -- -q   -f run syn-read < /dev/null 2> tests/filesys/base/syn-read.errors > tests/filesys/base/syn-read.output
perl -I../.. ../../tests/filesys/base/syn-read.ck tests/filesys/base/syn-read tests/filesys/base/syn-read.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/filesys/base/syn-remove:syn-remove -- -q   -f run syn-remove < /dev/null 2> tests/filesys/base/syn-remove.errors > tests/filesys/base/syn-remove.output
perl -I../.. ../../tests/filesys/base/syn-remove.ck tests/filesys/base/syn-remove tests/filesys/base/syn-remove.result
pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/filesys/base/syn-write:syn-write -p tests/filesys/base/child-syn-wrt:child-syn-wrt -- -q   -f run syn-write < /dev/null 2> tests/filesys/base/syn-write.errors > tests/filesys/base/syn-write.output
perl -I../.. ../../tests/filesys/base/syn-write.ck tests/filesys/base/syn-write tests/filesys/base/syn-write.result
pintos -v -k -T 600 -m 20 -m 20   --fs-disk=10 -p tests/userprog/no-vm/multi-oom:multi-oom -- -q   -f run multi-oom < /dev/null 2> tests/userprog/no-vm/multi-oom.errors > tests/userprog/no-vm/multi-oom.output
perl -I../.. ../../tests/userprog/no-vm/multi-oom.ck tests/userprog/no-vm/multi-oom tests/userprog/no-vm/multi-oom.result