#! /usr/bin/python3
# -*- coding: utf-8 -*-

import socket
import struct

MCAST_GRP = '224.0.0.5'
MCAST_PORT = 8101

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# listen ONLY to MCAST_GRP
sock.bind((MCAST_GRP, MCAST_PORT))

mreq = struct.pack("=4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton('10.0.0.61'))
sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

while True:
    print(sock.recv(10240))
