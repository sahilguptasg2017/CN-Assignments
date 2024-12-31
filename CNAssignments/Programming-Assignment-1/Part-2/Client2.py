from http import server
import random
import time
from socket import *
socketClient = socket(AF_INET , SOCK_DGRAM)
socketClient.settimeout(1)
serverAddress = ('127.0.0.1', 12000)
roundTripTimes = []
lossCount = 0 ; consecutiveMisses = 0 ; noOfPackets = 0
i = 0
while True:
    noOfPackets += 1
    startTime = time.time()
    makeMessage = f'ping {i+1} sent  at {startTime}'
    try:
        socketClient.sendto(makeMessage.encode() , serverAddress)
        responseMessage , serverval   = socketClient.recvfrom(1024)
        responseTime = time.time()
        rountTripTime = responseTime - startTime
        roundTripTimes.append(rountTripTime)
        print(f'Got a reply from {serverval} in {rountTripTime} seconds which is {responseMessage.decode()}')
        consecutiveMisses = 0
    except timeout:
        consecutiveMisses += 1 ; lossCount += 1
        print(f'Request timed out for Heartbeat {i+1}')
        print(consecutiveMisses)
        if consecutiveMisses == 3:
            print("Server missed 3 consecutive heartbeats. Exiting")
            break
    time.sleep(1)
    i += 1
socketClient.close()
packetLossRate = (lossCount/noOfPackets)*100 ; print(f'Packet loss rate is {packetLossRate}%')
print(f'Number of packets sent before server is down: {noOfPackets}')