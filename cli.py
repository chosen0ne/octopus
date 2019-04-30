#!/bin/env python
# coding=utf8
#
#
# @file:    cli
# @author:  chosen0ne(louzhenlin86@126.com)
# @date:    2019-01-04 11:42:27

import sys
import time
import socket
import io

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
def connect():
    s.connect(('127.0.0.1', 8080))

def send(data):

    data_len = len(data)
    o = bytearray()
    for i in xrange(4):
        # o.write(data_len & 0xFF)
        o.append(data_len & 0xFF)
        data_len = data_len >> 8

    # o.write(data)
    o += data
    s.send(o)
    resp = s.recv(1024)
    print "recv %d bytes: %s" %(len(resp), resp)

    # o.close()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'USAGE: python cli.py DATA'
        sys.exit(1)

    connect()

    for i in xrange(1, len(sys.argv)):
        send(sys.argv[i])
