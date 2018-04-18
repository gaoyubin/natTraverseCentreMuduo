#!/usr/bin/env python
from socket import *
def client():
    HOST = '192.168.1.103'
    PORT = 2007


    clientsocket = socket(AF_INET,SOCK_STREAM)
    clientsocket.connect((HOST,PORT))
    while True:
    	#data = raw_input('>')
    	#if not data:
    	#	break
        data = "sdfsfs"#'{"id":1,"name":"kurama"}
    	clientsocket.send(data)
    	data = clientsocket.recv(1024)
    	if not data:
    		break
    	print data


if __name__ == "__main__":
	client()
