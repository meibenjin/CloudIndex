#!/bin/bash
# automatic create a new instance 
# in openstack cloud platform

export OS_TENANT_NAME=liudehai
export OS_USERNAME=liudehai
export OS_PASSWORD=liudehai
export OS_AUTH_URL="http://localhost:5000/v2.0/"
export SERVICE_ENDPOINT="http://localhost:35357/v2.0"
export SERVICE_TOKEN=ADMIN

if [ $# = 0 ]; then
    echo "need arguments"
    exit
fi

COMMAND=$*

# TODO auto increase the index of the instance name
# such as newhive01 newhive02...

if [ "$COMMAND" = "boot" ]; then
    nova boot --flavor=9 --image=37d33b64-fa8d-45ff-989f-475f7e117171 newhive08
else
    nova $COMMAND
fi
