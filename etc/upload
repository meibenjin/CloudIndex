#!/bin/bash

local_dir=${HOME}/Programming/C/CloudIndex
# be careful! when change upload dir
upload_dir=/home/iir/mbj
remote_shell="rm ${upload_dir}/* -rf"
host_prefix="iir@10.77.30"

function clean_remote_dir()
{
    for i in 101 199 200 201 202 203 204 205
    do 
        remote_host="${host_prefix}.$i"
        ssh ${remote_host} "${remote_shell}"
        echo "clean up directory ${remote_host}:${upload_dir}"
    done
}

function copy_it()
{
    for i in 101 199 200 201 202 203 204 205
    do 
        remote_host="${host_prefix}.$i"
        scp -r -q ${local_dir} ${remote_host}:${upload_dir}
        echo "copy to ${remote_host}"
    done
}

if [ $# = 0 ]; then
    clean_remote_dir
    copy_it
fi

COMMAND=$1

if [ "$COMMAND" = "clean" ]; then
    clean_remote_dir
elif [ "$COMMAND" = "copy" ]; then
    copy_it
fi


