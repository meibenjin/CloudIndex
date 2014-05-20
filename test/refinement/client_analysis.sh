#!/bin/bash

for i in $(seq 1 15)
do
    start=$((($i-1) * 600 +1))
    end=$(($i * 600))

    qt=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'range query spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    rct=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'recreate trajs spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    find_idle_node=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'find idle torus node spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    trajs_count=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'trajs count:[0-9\.]+ $' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    max_trajs_count=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'trajs count:[0-9\.]+ $' | grep -o -E '[0-9\.]+' | awk 'BEGIN{max=-1}{if($1 > max){max = $1}}END{print max}'`

    send_refinement=`cat client.log | sed -n "${start}, ${end}p" | grep -o -E 'send refinement data spend:[0-9\.]+ ms$' | grep -o -E '[0-9\.]+' | awk 'BEGIN{total=0}{total+=$1}END{print total/100}'`

    #echo $qt
    #echo $rct
    #echo $find_idle_node
    #echo $trajs_count
    #echo $max_trajs_count
    echo $send_refinement
    #echo $qt + $rct + $find_idle_node + $send_refinement | bc
done
