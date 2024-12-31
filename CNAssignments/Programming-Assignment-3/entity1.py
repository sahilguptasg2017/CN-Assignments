from enum import Enum
import socket
import time
import random
import threading
from dataclasses import dataclass
import json
from typing import Any, Dict   
from collections import defaultdict

maxSEQ = 7
TotalPacketsToSend = 100

class EventType(Enum):
    frameArrival = 'frame_arrival'
    timeOut = 'timeout'
    networkLayerReady = 'network_layer_ready'

@dataclass
class Packet:
    data : str
    timeval : float

@dataclass
class Frame:
    seq : int
    ack :int
    info: Packet
    def jsonConv(self)->str:
        return json.dumps({
            'info' : {'data': self.info.data, 'timeval': self.info.timeval},    
            'seq' : self.seq,
            'ack' : self.ack    
        })
    @classmethod
    def fromJson(cls , json_str : str) -> 'Frame':
        data = json.loads(json_str)
        return cls(
            info = Packet(data['info']['data'], data['info']['timeval']),
            seq = data['seq'],
            ack = data['ack']
        )

def between(a : int , b :int , c:int)->bool:
    if ((a <= b) and (b < c)) or ((c < a) and (a <= b)) or ((b < c) and (c < a)):
        return True
    return False

class goBackNbidirectional:
    def __init__(self , port1 : int , port2):
        self.buffer = [None] * (maxSEQ + 1)
        self.queue1 = []
        self.sock = socket.socket(socket.AF_INET , socket.SOCK_DGRAM)
        self.sock.bind(('localhost', port1))
        self.remote_addr = ('localhost', port2) 
        self.timer = [0]*(maxSEQ + 1)
        self.timeoutval = 2
        self.nextFrameToSend = 0
        self.ackExpected = 0   
        self.frameExpected = 0
        self.nBuffered = 0 
        self.netWorkLayerReady = True
        self.packetsSend = 0
        self.packetsReceived = 0   
        self.retransmissions = 0   
        self.delay = 0
        self.lock = threading.Lock()
        self.stopEvent = threading.Event()
        self.flag = True
        self.sendTimes = {}  
        self.receiveTimes = {}  
        self.sendAttempts = defaultdict(int)  
        self.lastAckSent = (self.frameExpected + maxSEQ) % (maxSEQ+1)
        self.transmissionsTimes = defaultdict(list)
        self.creationTimes = {}
    def startTimer(self , seq :int):
        self.timer[seq] = time.time()

    def stopTimer(self , seq : int):
        self.timer[seq] = 0

    def helpersender(self , frameno : int , buffer : list):
        if self.packetsSend >= TotalPacketsToSend:
            self.stopEvent.set()
            return
        p = 0.1
        ack_to_send = (self.frameExpected + maxSEQ) % (maxSEQ + 1)
        s = Frame(
            info = buffer[frameno],
            seq = frameno,
            ack = ack_to_send
        )
        print(f"Sending frame {s.seq} with ACK {s.ack}")
        try:
            self.sendAttempts[s.seq] += 1
            self.transmissionsTimes[s.seq].append(time.time())
            if s.seq not in self.sendTimes:
                self.sendTimes[s.seq] = time.time()
                
            if random.random() > p:
                self.sock.sendto(s.jsonConv().encode(), self.remote_addr)
                if self.packetsSend >= TotalPacketsToSend:
                    self.stopEvent.set()
            else:
                print(f'Frame {s.seq} dropped')
                self.sendAttempts[s.seq] += 1 
            self.startTimer(frameno)
            self.packetsSend += 1
        except Exception as e:
            print(f'Error sending frame {s.seq} : {e}')
    def sendAck(self):
        ackToSend = (self.frameExpected + maxSEQ) % (maxSEQ + 1)
        s = Frame(
            info = Packet(data='' , timeval=time.time()),
            seq = -1,
            ack=ackToSend
        )
        print(f'Sending ACK-only frame with ACK{s.ack}')
        try:
            self.sock.sendto(s.jsonConv().encode(), self.remote_addr)
        except Exception as e:
            print(f'Error sending ACK-only frame : {e}')

    def checktimeout(self):
        if self.packetsSend >= TotalPacketsToSend:
            self.stopEvent.set()
            return False
        for i in range(maxSEQ + 1):
            if self.timer[i] != 0:
                if time.time() - self.timer[i] > self.timeoutval:
                    return True
        return False
    
    def networkLayer(self):
        if self.packetsSend >= TotalPacketsToSend:
            exit(0)
                        
        cnt = 0
        T1 = 0.1 ; T2 = 0.2
        while not self.stopEvent.is_set():
            if self.netWorkLayerReady:
                time.sleep(random.uniform(T1 , T2))
                packet = Packet(
                    data = f"Packet_Entity_2_{cnt}",
                    timeval = time.time()
                )
                self.creationTimes[f'Packet_Entity_2_{cnt}'] = packet.timeval
                self.queue1.append((EventType.networkLayerReady , packet))
                cnt += 1
            time.sleep(0.01)

    def reciever(self):
        if self.packetsSend >= TotalPacketsToSend:
            exit(0)
        self.sock.settimeout(1.25)
        T3 = 0.1 ; T4 = 0.2
        while not self.stopEvent.is_set():
            try:
                data , unused = self.sock.recvfrom(1024)
                frame = Frame.fromJson(data.decode())
                time.sleep(random.uniform(T3 , T4))
                self.queue1.append((EventType.frameArrival , frame))
            except socket.timeout:
                continue
            except Exception as e:
                print(f'Error receiving frame : {e}')
                break

    def mainsender(self):
        while not self.stopEvent.is_set() or len(self.queue1) > 0:
            if len(self.queue1) > 0:
                eventtype , data = self.queue1.pop(0)
                if eventtype == EventType.networkLayerReady:
                    if self.nBuffered < maxSEQ:
                        self.buffer[self.nextFrameToSend] = data
                        self.nBuffered += 1
                        self.helpersender(self.nextFrameToSend, self.buffer)
                        self.nextFrameToSend = (self.nextFrameToSend + 1) % (maxSEQ + 1)
                elif eventtype == EventType.frameArrival:
                    frame = data
                    if frame.seq != -1:
                        if frame.seq == self.frameExpected:
                            self.packetsReceived += 1
                            self.receiveTimes[frame.seq] = time.time()
                            if hasattr(frame.info , 'data'):
                                self.receiveTimes[frame.info.data] = time.time() 
                            print(f"Frame {frame.seq} received in order")
                            self.frameExpected = (self.frameExpected + 1) % (maxSEQ + 1)
                        else:
                            print(f"Frame {frame.seq} received out of order")
                    if frame.ack != self.lastAckSent:
                        print(f"Received ACK {frame.ack}")
                        while self.ackExpected != (frame.ack + 1) % (maxSEQ + 1):
                            self.nBuffered -= 1
                            self.stopTimer(self.ackExpected)
                            self.ackExpected = (self.ackExpected + 1) % (maxSEQ + 1)
                        self.lastAckSent = frame.ack
            if self.checktimeout():
                print(f'--------Timeout, resending window')
                self.retransmissions += 1
                nextFrame = self.ackExpected
                self.sendAttempts[nextFrame] += 1
                for i in range(self.nBuffered):
                    self.helpersender(nextFrame, self.buffer)
                    nextFrame = (nextFrame + 1) % (maxSEQ + 1)
            if self.nBuffered < maxSEQ:
                self.netWorkLayerReady = True
            else:
                self.netWorkLayerReady = False
            if self.packetsSend >= TotalPacketsToSend:
                self.stopEvent.set()
                break
            time.sleep(0.01)

    def CalcStat(self):
        print("Packets sent : ", self.packetsSend)
        print("Retransmissions : ", self.retransmissions)
        total_delay = 0
        valid_delays = 0
        
        for i , time in self.creationTimes.items():
            if i in self.receiveTimes:
                receivetime = self.receiveTimes[i]
                delay = receivetime - time
                total_delay += abs(delay)
                valid_delays += 1
        print(f"Average delay: {total_delay / valid_delays:.2f}" if valid_delays > 0 else "No packets received") 
        
        total_attempts = sum(self.sendAttempts.values())
        avg_attempts = total_attempts / TotalPacketsToSend
        print(f"Average transmission attempts per frame: {avg_attempts:.2f}")

    def thgoback(self):
        threads = [
            threading.Thread(target=self.networkLayer),
            threading.Thread(target=self.reciever),
            threading.Thread(target=self.mainsender)
        ]
        for x in threads:
            x.start()
        for x in threads:
            x.join()
        self.sock.close()
        self.CalcStat()

entity = goBackNbidirectional(port1=5068, port2=5069)
entity.thgoback()