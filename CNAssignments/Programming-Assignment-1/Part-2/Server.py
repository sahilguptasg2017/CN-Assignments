import random
from socket import *
import time
from numpy import diff
serverSocket = socket(AF_INET , SOCK_DGRAM)
serverSocket.bind(('' , 12000))
while True:
    rand = random.randint(0 , 10)
    message , address = serverSocket.recvfrom(1024)
    message = message.upper()
    if rand < 4:
        continue
    messageRecieved = message.decode()
    print(messageRecieved)
    pingNumberTemp , pingTime = messageRecieved.split(' SENT  AT ')                                   
    pingNumber = pingNumberTemp[5:] ; pingTime = float(pingTime)
    diffTime = time.time() - pingTime
    messageResponse = f'Ping {pingNumber} recieved at {time.time()} with a delay of {diffTime} seconds'
    serverSocket.sendto(messageResponse.encode() , address)  