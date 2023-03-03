#!/usr/bin/python3
# -*- coding: utf-8 -*-

import argparse
import socket

parser = argparse.ArgumentParser('client.py')
parser.add_argument(
    'server',
    metavar='SERVER',
    type=str,
    nargs=1,
    help='server address'
)
parser.add_argument(
    '-p', '--port',
    type=int,
    default=3333,
    dest='port',
    help='server port'
)

args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.connect((args.server[0], args.port))
sock.sendall(b'REQUEST\0')

data = sock.recv(1024)

# print(len(data))

sock.close()