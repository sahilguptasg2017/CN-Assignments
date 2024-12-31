from http import server
import random
import time
from socket import *
socketClient = socket(AF_INET , SOCK_DGRAM)
socketClient.settimeout(1)
serverAddress = ("127.0.0.1", 12000)
roundTripTimes = []
lossCount = 0
for i in range(10):
    startTime = time.time()
    makeMessage = f'ping {i+1} sent  at {startTime}'
    try:
        socketClient.sendto(makeMessage.encode() , serverAddress)
        responseMessage , serverval   = socketClient.recvfrom(1024)
        responseTime = time.time()
        rountTripTime = responseTime - startTime
        roundTripTimes.append(rountTripTime)
        print(f'Got a reply from {serverval} in {rountTripTime} seconds which is {responseMessage.decode()}')
    except timeout:
        print(f'Request timed out for {i+1} ping')
        lossCount += 1
socketClient.close()
if len(roundTripTimes) > 0:
    print(f'Minimum round trip time is {min(roundTripTimes)} seconds')
    print(f'Maximum round trip time is {max(roundTripTimes)} seconds')
    print(f'Average round trip time is {sum(roundTripTimes)/len(roundTripTimes)} seconds')
else:
    print('None of the pings were successful')
packetLossRate = (lossCount/10)*100 ; print(f'Packet loss rate is {packetLossRate}%')