#!/bin/bash
#$1 - inputdir
#$2 - outputdir
#$3 - maxthreads
#$4 - numbuckets
#tempFile - temporary file to be filtered

#reset file
: > tempFile

for i in $(ls $1)
do
    echo InputFile=$i NumThreads=1 >> tempFile
    ./tecnicofs-nosync $1/$i output.o 1 $4 >> tempFile
done

for i in $(ls $1)
do
    for x in $(seq 2 $3)
    do
        echo InputFile=$i NumThreads=$x >> tempFile 
        ./tecnicofs-mutex $1/$i output.o $x $4 >> tempFile

    done
done

grep "TecnicoFS\|InputFile" tempFile > $2
rm tempFile