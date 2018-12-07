#!/usr/bin/python
import os,time,select

box = os.open("/dev/box_ipc", os.O_RDWR | os.O_NONBLOCK)
reg=bytearray(b'\x00\x11\x11\x00\x00\x00\x00\x00')
os.write(box,reg)
print "write and flush"
m3=bytearray(b'\xff\xff\xff\xff\x02\x00\x00\x00'"<msg><M3 /></msg>")
print "message",m3
os.write(box,m3)
#time.sleep(1)
epoll = select.epoll()
epoll.register(box, select.EPOLLIN)
try:
 while True:
  events = epoll.poll(1)
  for event in events:
    if select.EPOLLIN:
      print "read",os.read(box,50)
finally:
 epoll.unregister(box)
 epoll.close()
 os.close(box)
