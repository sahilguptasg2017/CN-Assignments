from socket import *
import sys
import threading
import time
def Client(connectionSocket):
    try :
        print("Thread ID = " , threading.get_ident() , "handling connetion from" , connectionSocket.getpeername() , "at" , time.ctime()) 
        messageRecieved = connectionSocket.recv(1024).decode()
        filename = messageRecieved.split()[1]
        with open(filename[1:]) as f:
            output = f.read()
        # print(output)
        connectionSocket.send("HTTP/1.1 200 OK\r\n".encode())
        connectionSocket.send("Content-Type: text/html\r\n".encode())
        connectionSocket.send("\r\n".encode())
        for i in range(0 , len(output)):
            connectionSocket.send(output[i].encode())
        connectionSocket.send("\r\n".encode())
        connectionSocket.close()
    except IOError:
        connectionSocket.send("HTTP/1.1 404 Not Found\r\n".encode())
        connectionSocket.send("Content-Type: text/html\r\n".encode())
        connectionSocket.send("\r\n".encode())      
        connectionSocket.send("<html><body><h1>404 Not Found</h1></body></html>\r\n".encode())
        connectionSocket.send("\r\n".encode())
        connectionSocket.close()
    # return cnt
serverSocket = socket(AF_INET , SOCK_STREAM)
serverSocket.bind(('' , 6789))
serverSocket.listen(10)
print('Ready to serve...')
cnt = 0 
while True:
    connectionSocket , addr = serverSocket.accept()
    # cnt += 1
    # print(f'Connection at {cnt}: From {addr} , Thread ID = {threading.get_ident()}')
    clientThread = threading.Thread(target = Client , args = (connectionSocket,))
    # print(cnt)      
    clientThread.start()
serverSocket.close()
sys.exit()