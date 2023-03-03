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

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(('', args.port))
sock.listen(1)

while True:
    conn, _ = sock.accept()
    with conn:
        data = conn.recv(1024)
        if not data:
            break
        # print(len(data))
        conn.sendall(b'RESPONSE\0')

sock.close()
