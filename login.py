import socket
import struct
import collections
import binascii

LM_SessionRequest = 0x01
LM_SessionResponse = 0x02
LM_Combined = 0x03
LM_SessionDisconnect = 0x05
LM_ApplicationPacket = 0x09
LM_Ack = 0x15

LM_Types = {value: name for name, value in globals().items() if name.startswith("LM_")}

class Parameter(object):
    def __init__(self, name, code, value):
        self.name = name
        self.code = code
        self.value = value

class LoginMessage(object):
    def __init__(self, type):
        self.type = type
        self.params = collections.OrderedDict()
    
    def add_param(self, name, code, value=None):
        self.params[name] = Parameter(name, code, value)
    
    def serialize(self):
        patterns = ["!H"]
        args = [self.type]
        for param in self.params.values():
            patterns.append(param.code)
            args.append(param.value)
        return struct.pack("".join(patterns), *args)
    
    def deserialize(self, data):
        patterns = ["!H"]
        args = [self.type]
        for param in self.params.values():
            patterns.append(param.code)
        args = struct.unpack("".join(patterns), data)
        param_names = list(self.params)
        for name, arg in zip(param_names, args[1:]):
            self.params[name].value = arg
    
    def __str__(self):
        chunks = []
        try:
            msg_name = LM_Types[self.type]
        except KeyError:
            msg_name = "LoginMessage_%02x" % self.type
            
        param_chunks = []
        for param in self.params.values():
            param_chunks.append("%s=%s" % (param.name, param.value))
        chunks.append("<")
        chunks.append(msg_name)
        chunks.append("(%s)" % ", ".join(param_chunks))
        chunks.append(">")
        return "".join(chunks)

class Client(object):
    def __init__(self, addr):
        self.addr = addr
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.crc_key = 0
    
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
        return self.parse(response)
    
    def crc16(self, data):
        # XXX Implement this. Not that the login server minds if zero.
        return 0
    
    def parse(self, packet):
        # Extract the message type and CRC and validate them.
        msg_type = struct.unpack("!H", packet[0:2])[0]
        if msg_type > 0xff:
            raise ValueError("Message type out of range: 0x%04x" % msg_type)
        has_crc = msg_type not in (LM_SessionRequest, LM_SessionResponse)
        if has_crc:
            crc, packet = struct.unpack("!H", packet[-2:])[0], packet[0:-2]
            computed_crc = self.crc16(packet, self.key)
            if (crc != 0) and (crc != computed_crc):
                raise ValueError("Invalid CRC: computed 0x%04x, but found 0x%04x." % (computed_crc, crc))
        msg = LoginMessage(msg_type)
        
        # Call the function that can parse the message, if it exists.
        try:
            msg_name = LM_Types[msg_type]
        except KeyError:
            pass
        else:
            fn_name = "parse_%s" % msg_name
            if hasattr(self, fn_name):
                fn = getattr(self, fn_name)
                fn(msg, packet)
        return msg
    
    def parse_LM_SessionResponse(self, msg, data):
        msg.add_param("Session", "I")
        msg.add_param("Key", "I")
        msg.add_param("UnknownA", "B")
        msg.add_param("Format", "B")
        msg.add_param("UnknownB", "B")
        msg.add_param("MaxLength", "I")
        msg.add_param("UnknownC", "I")
        msg.deserialize(data)

if __name__ == "__main__":
    addr = ("192.168.0.3", 5998)
    client = Client(addr)

    m = LoginMessage(LM_SessionRequest)
    m.add_param("UnknownA", "I", 0x00000002)
    m.add_param("Session", "I", 0x26ec5075)
    m.add_param("MaxLength", "I", 0x00000200)
    
    client.send(m)
    m2 = client.receive()
    print(m2)
