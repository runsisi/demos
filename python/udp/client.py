#!/usr/bin/python3
# -*- coding: utf-8 -*-

import argparse
import socket
import time

def pretty_bytes(bytes):
    for unit in ['B', 'KiB', 'MiB', 'GiB']:
        if bytes < 1024.0 or unit == 'GiB':
            break
        bytes /= 1024.0
    return f"{bytes:.2f} {unit}"


parser = argparse.ArgumentParser('c.py')
parser.add_argument(
    '-c', '--count',
    type=int,
    default=1000,
    dest='count',
    help='count'
)
parser.add_argument(
    '-p', '--port',
    type=int,
    default=62000,
    dest='port',
    help='server port'
)
parser.add_argument(
    '-l', '--length',
    type=int,
    default=1024,
    dest='length',
    help='packet length'
)
parser.add_argument(
    '-t', '--time',
    type=int,
    default=0,
    dest='time',
    help='run time in second'
)
parser.add_argument(
    'addr',
    help='server address'
)

args = parser.parse_args()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(1.0)

m = b'-' * args.length
addr = (args.addr, args.port)

start = time.monotonic_ns()
stop = start + args.time * 1000_000_000

last_time = curr_time = start
last_bytes = 0

count = args.count

while count > 0 or curr_time < stop:
    sock.sendto(m, addr)

    last_bytes += args.length

    count -= 1
    curr_time = time.monotonic_ns()

    if curr_time - last_time >= 1000_000_000:
        speed = last_bytes * 1000_000_000 / (curr_time - last_time)
        print('{}ps'.format(pretty_bytes(speed)))

        last_time = curr_time
        last_bytes = 0

if last_bytes > 0 and curr_time > last_time:
    speed = last_bytes * 1000_000_000 / (curr_time - last_time)
    print('{}ps'.format(pretty_bytes(speed)))
