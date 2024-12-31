import socket 
import sys
# print(sys.argv)
if len(sys.argv) != 4:
    print("More/less than 4 arguments provided. Exiting...")
    sys.exit(1)
serverHost = sys.argv[1]
serverPort = int(sys.argv[2])
serverAddress = (serverHost , serverPort)
nameOfFile = sys.argv[3]    

clientSocket = socket.socket(socket.AF_INET , socket.SOCK_STREAM)


clientSocket.connect(serverAddress)
message = "GET /" + nameOfFile + " HTTP/1.1\r\n\r\n"
clientSocket.send(message.encode())
response = b""
while True:
    bytesRecieved = clientSocket.recv(1024) 
    if not bytesRecieved:
        break
    response += bytesRecieved  
print(response.decode())  

clientSocket.close()
