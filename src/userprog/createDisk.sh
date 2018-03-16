#!/bin/bash
make clean
make
cd build
pintos-mkdisk filesys.dsk --filesys-size=2
pintos -v -- -f -q
pintos -v -p ../../examples/echo -a echo -- -q
pintos -v -p ../../examples/pwd -a pwd -- -q
pintos -v -p ../../examples/test -a test -- -q
pintos -v -p ../../examples/retval -a retval -- -q
echo "Now do 'pintos -v -- run 'test'' or any other executable added."
cd ..
