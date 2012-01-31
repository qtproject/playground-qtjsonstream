#!/usr/bin/env python

import sys
import asyncore, socket
import json

counter = 0
registrationMessage = ""

class JsonClient(asyncore.dispatcher):

    def connectTCP(self,host,port):
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect( (host, port) )

    def connectLocal(self,socketname):
        self.create_socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.connect(socketname)

    def handle_connect(self):
        global registrationMessage
        if not registrationMessage:
            client.send(registrationMessage)

    def handle_close(self):
        self.close()

    def handle_read(self):
        buf = self.recv(8192);
        print >> sys.stderr, 'receivedData:[',buf,']'
        receivedMessage(self,buf)

def sendMessage(client):
    global counter
    msg = { "text": "Standard text", "number" : counter, "int": 100,
        "float": 100.0, "true": True, "false": False, "array" : ["one", "two", "three"],
        "object" : { "item1": "This is item 1", "item2": "This is item 2"} }
    counter += 1
    print >> sys.stderr, 'sendMessageJSON:', json.dumps(msg)
    client.send(json.dumps(msg))

def receivedMessage(client,data):
    msg = json.loads(data);
    print >> sys.stderr, 'receivedMessage:[',msg,']'
    if 'command' in msg:
        print >> sys.stderr, "command: ", msg['command']
        command = msg['command']
        if command == "exit":
           sys.exit(0)
        elif command == "crash":
            sys.exit(1)
        elif command == "reply":
            sendMessage(client)
        elif command == "flurry":
            if 'count' in msg:
                for count in range(msg['count'], 0):
                    sendMessage();

if __name__ == "__main__":

    socketname = "/tmp/tst_socket"

    # retrive socketname from arguments list
    try:
        i = sys.argv.index('-socket')
        if i + 1 < len(sys.argv):
            socketname = sys.argv[i + 1]
    except ValueError:
        pass
    print >> sys.stderr, "socketname:", socketname

    client = JsonClient()
    client.connectLocal(socketname)

    sendMessage(client)

    asyncore.loop()
