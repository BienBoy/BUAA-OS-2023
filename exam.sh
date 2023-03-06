#!/bin/bash

mkdir mydir
chmod +777 mydir
echo 2023 > myfile
mv ./moveme ./mydir
cp ./copyme ./mydir/copied
cat ./readme
gcc bad.c 2> err.txt

mkdir gen
n=10
if [ $# -ne 0 ]
then
	n=$[$1]
fi

while [ $n -ne 0 ]
do
	touch ./gen/$n.txt
	n=$[$n-1]
done

