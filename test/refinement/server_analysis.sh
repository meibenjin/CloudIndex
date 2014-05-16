#!/bin/bash

for i in $(seq 0 14)
do
    start=$(($i * 400 +1))
    end=$((($i+1) * 400))

    receive_ref=`cat server.log| sed -n "${start}, ${end}p" | grep -o -E 'data spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    need_ref_cnt=`cat server.log| sed -n "${start}, ${end}p" | grep -o -E 'need refinement trajs count:[0-9\.]+ $' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    refinement=`cat server.log| sed -n "${start}, ${end}p" | grep -o -E 'refinement spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    #echo $receive_ref
    #echo $need_ref_cnt
    echo $refinement
    #echo $receive_ref + $refinement | bc
done
