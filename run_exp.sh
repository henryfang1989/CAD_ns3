#!/bin/bash -e
windowSize=(2000) #8000 32000)
queueSize=(2000) #8000 32000)
segSize=(128 256 512)
maxPacketSize=(512) #256 512)
maxTh=(1600) #6400 25600)
minTh=(1000) #4000 16000)
echo "Droptail Queue ==================================="
for i in ${windowSize[@]};
do
    for j in ${queueSize[@]};
    do
    	for k in ${segSize[@]};queueType
        do
       	    for l in ${maxPacketSize[@]};
        	do
       		   for m in ${maxTh[@]};
       		   do
            	   for n in ${minTh[@]};
       			      do
        				./waf --run 'P22_2 --windowSize='${i}' --queueSize='${j}' --segSize='${k}' --maxPacketSize='${l}' --maxTh='${m}' --minTh='${n}' --queueType=droptail'
                        #./waf --run 'P22_2 --windowSize='${i}' --queueSize='${j}' --segSize='${k}' --maxPacketSize='${l}' --maxTh='${m}' --minTh='${n}' --queueType=RED'
    				done
    			done
			done
		done
	done
done
echo "RED Queue ==================================="
for i in ${windowSize[@]};
do
    for j in ${queueSize[@]};
    do
        for k in ${segSize[@]};
        do
            for l in ${maxPacketSize[@]};
            do
               for m in ${maxTh[@]};
               do
                   for n in ${minTh[@]};
                      do
                       # ./waf --run 'P22_2 --windowSize='${i}' --queueSize='${j}' --segSize='${k}' --maxPacketSize='${l}' --maxTh='${m}' --minTh='${n}' --queueType=droptail'
                        ./waf --run 'P22_2 --windowSize='${i}' --queueSize='${j}' --segSize='${k}' --maxPacketSize='${l}' --maxTh='${m}' --minTh='${n}' --queueType=RED'
                    done
                done
            done
        done
    done
done
