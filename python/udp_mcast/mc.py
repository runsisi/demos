#! /usr/bin/python3
# -*- coding: utf-8 -*-

import socket
import struct

MCAST_GRP = '224.0.0.5'
MCAST_PORT = 8101

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

mreq = struct.pack("=i4s", 0, socket.inet_aton('10.0.0.61'))
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, mreq)

sock.sendto(b"robot", (MCAST_GRP, MCAST_PORT))
