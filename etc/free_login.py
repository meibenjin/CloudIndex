#!/usr/bin/python
# this python file runs on control node

import os
import pexpect

pwd = "111111"
file_path = raw_input()
file = open(file_path,"r")
lines = file.readlines()
for line in lines:
    child = pexpect.spawn('ssh-copy-id -i /root/.ssh/id_rsa.pub root@' + line)
    message = ''
    try:
        i = child.expect(['[Pp]assword:','continue connecting (yes/no?)'])
        if i == 0:
            child.sendline(pwd)
        elif i == 1:
            child.sendline('yes')
            child.expect('[Pp]assword')
            child.sendline(pwd)
        else:
            pass
    except pexpect.EOF:
        message = child.read()
        child.close()
    else:
        message = child.read()
        child.expect(pexpect.EOF)
        child.close()
