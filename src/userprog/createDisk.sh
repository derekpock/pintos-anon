#!/bin/bash
make clean
make
cd build
pintos-mkdisk filesys.dsk --filesys-size=2
pintos -f -q
pintos -p ../../examples/echo -a echo -- -q
pintos -p ../../examples/pwd -a pwd -- -q
pintos -p ../../examples/test -a test -- -q
echo "Now do 'pintos -v -- run 'test'' or any other executable added."
cd ..
