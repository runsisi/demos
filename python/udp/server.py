#!/usr/bin/python3
# -*- coding: utf-8 -*-

import argparse
import socket

parser = argparse.ArgumentParser('server.py')
parser.add_argument(
    '-p', '--port',
    type=int,
    default=3333,
    dest='port',
    help='server port'
)

args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', args.port))

while True:
    (buf, _) = sock.recvfrom(65536)
    print(f"received {len(buf)} bytes")
