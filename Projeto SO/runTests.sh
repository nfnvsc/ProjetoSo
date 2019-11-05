#!/bin/bash
#$1 - inputdir
#$2 - outputdir
#$3 - maxthreads
#$4 - numbuckets
#tempFile - temporary file to be filtered

for i in $(ls $1)
do
    #reset files
    : > $2/$i-1.txt
    : > tempFile

    echo InputFile=$i NumThreads=1
    ./tecnicofs-nosync $1/$i $2/$i-1.txt 1 1 >> tempFile
    grep "TecnicoFS" tempFile
    
    for x in $(seq 2 $3)
    do
        : > tempFile
        : > $2/$i-$x.txt

        echo InputFile=$i NumThreads=$x 
        ./tecnicofs-mutex $1/$i $2/$i-$x.txt $x $4 >> tempFile
        grep "TecnicoFS" tempFile      
    done
    


done
#remove tempFile
rm tempFile