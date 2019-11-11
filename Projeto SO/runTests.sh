#!/bin/bash
#$1 - inputdir
#$2 - outputdir
#$3 - maxthreads
#$4 - numbuckets
#TEMP - temporary variable to be filtered

for i in $(ls $1)
do
    : > $2/$i-1.txt
    TEMP=''

    echo InputFile=$i NumThreads=1
    TEMP=$(./tecnicofs-nosync $1/$i $2/$i-1.txt 1 1)
    echo $TEMP | grep -o "TecnicoFS.*"
    
    for x in $(seq 2 $3)
    do
        : > $2/$i-$x.txt
        TEMP=''

        echo InputFile=$i NumThreads=$x 
        TEMP=$(./tecnicofs-mutex $1/$i $2/$i-$x.txt $x $4)
        echo $TEMP | grep -o "TecnicoFS.*"      
    done

done