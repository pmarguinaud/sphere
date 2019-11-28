#!/bin/bash

for tr in t0049 t0149 t0224 t0479 t1798 t2560 t8000 t16000
do
if [ -f $tr.grb ]
then
echo "==> $f <=="
/usr/bin/time -f 'mem=%Mkb,time=%E' ./main.x $tr.grb
fi
done
