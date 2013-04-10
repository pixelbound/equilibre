import socket
import struct
import binascii

LM_SessionRequest = 0x01
LM_SessionResponse = 0x02
LM_Combined = 0x03
LM_SessionDisconnect = 0x05
LM_ApplicationPacket = 0x09
LM_Ack = 0x15

class LoginMessage(object):
    def __init__(self, type):
        self.type = type
        self.codes = []
        self.params = []
    
    def add_param(self, code, value):
        self.codes.append(code)
        self.params.append(value)
    
    def serialize(self):
        pattern = "!H"
        pattern += "".join(self.codes)
        params = [self.type]
        params.extend(self.params)
        self.add_param("H", self.type)
        return struct.pack(pattern, *params)

class Client(object):
    def __init__(self, addr):
        self.addr = addr
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    def format_addr(self, addr):
        return "%s:%d" % addr
    
    def send(self, message):
        packet = message.serialize()
        print("%s >>> %s" % (self.format_addr(self.addr),
                             binascii.b2a_hex(packet).decode('utf8')))
        self.socket.sendto(bytes(packet), self.addr)
    
    def receive(self, max_size=1024):
        response, remote = self.socket.recvfrom(1024)
        print("%s <<< %s" % (self.format_addr(remote),
                             binascii.b2a_hex(response).decode('utf8')))

if __name__ == "__main__":
    addr = ("192.168.0.3", 5998)
    client = Client(addr)

    m = LoginMessage(LM_SessionRequest)
    m.add_param("I", 0x00000002)
    m.add_param("I", 0x26ec5075)
    m.add_param("I", 0x00000200)
    
    client.send(m)
    client.receive()
