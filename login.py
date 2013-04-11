import socket
import struct
import collections
import binascii

# Session message types.
SM_SessionRequest = 0x01
SM_SessionResponse = 0x02
SM_Combined = 0x03
SM_SessionDisconnect = 0x05
SM_LoginPacket = 0x09
SM_Ack = 0x15
SM_Types = {value: name for name, value in globals().items() if name.startswith("SM_")}

# Login message types.
LM_GetChatMessage = 0x01
LM_Login = 0x02
LM_ListServers = 0x04
LM_Types = {value: name for name, value in globals().items() if name.startswith("LM_")}

class Parameter(object):
    def __init__(self, name, code, value):
        self.name = name
        self.code = code
        self.value = value

class Message(object):
    def __init__(self, type, ns):
        self.type = type
        self.ns = ns
        self.params = collections.OrderedDict()
        self.body = None
    
    def add_param(self, name, code, value=None):
        self.params[name] = Parameter(name, code, value)
    
    def serialize(self):
        patterns = []
        if self.ns == "SM":
            patterns.append("!")
        else:
            patterns.append("<")
        patterns.append("H")
        args = [self.type]
        for param in self.params.values():
            patterns.append(param.code)
            args.append(param.value)
        data = struct.pack("".join(patterns), *args)
        if self.body:
            data += self.body.serialize()
        # Add checksum at the end of the packet.
        if (self.ns == "SM") and (self.type not in (SM_SessionRequest, SM_SessionResponse)):
            crc = 0
            data += struct.pack("H", crc)
        return data
    
    def deserialize(self, data):
        patterns = ["!H"]
        args = [self.type]
        for param in self.params.values():
            patterns.append(param.code)
        pattern = "".join(patterns)
        total_size = struct.calcsize(pattern)
        args = struct.unpack(pattern, data[0:total_size])
        param_names = list(self.params)
        for name, arg in zip(param_names, args[1:]):
            self.params[name].value = arg
        if len(data) > total_size:
            self.body = data[total_size:]
    
    def __str__(self):
        chunks = []
        if self.ns == "SM":
            try:
                msg_name = SM_Types[self.type]
            except KeyError:
                msg_name = "SessionMessage_%02x" % self.type
        elif self.ns == "LM":
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
        self.client_seq = 0
        self.server_seq = 0
    
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
        return self.parse_session(response)
    
    def crc16(self, data):
        # XXX Implement this. Not that the login server minds if zero.
        return 0
    
    def parse_session(self, packet):
        """ Parse a session message. """
        # Extract the message type and CRC and validate them.
        msg_type = struct.unpack("!H", packet[0:2])[0]
        if msg_type > 0xff:
            raise ValueError("Message type out of range: 0x%04x" % msg_type)
        has_crc = msg_type not in (SM_SessionRequest, SM_SessionResponse)
        if has_crc:
            crc, packet = struct.unpack("!H", packet[-2:])[0], packet[0:-2]
            computed_crc = self.crc16(packet)
            if (crc != 0) and (computed_crc != 0) and (crc != computed_crc):
                raise ValueError("Invalid CRC: computed 0x%04x, but found 0x%04x." % (computed_crc, crc))
        msg = Message(msg_type, "SM")
        
        # Call the function that can parse the message, if it exists.
        try:
            msg_name = SM_Types[msg_type]
        except KeyError:
            pass
        else:
            fn_name = "parse_%s" % msg_name
            if hasattr(self, fn_name):
                fn = getattr(self, fn_name)
                fn(msg, packet)
        return msg
    
    def parse_SM_SessionResponse(self, msg, data):
        msg.add_param("Session", "I")
        msg.add_param("Key", "I")
        msg.add_param("UnknownA", "B")
        msg.add_param("Format", "B")
        msg.add_param("UnknownB", "B")
        msg.add_param("MaxLength", "I")
        msg.add_param("UnknownC", "I")
        msg.deserialize(data)
    
    def create_session_message(self, msg_type):
        return Message(msg_type, "SM")
    
    def create_login_message(self, msg_type):
        msg = self.create_session_message(SM_LoginPacket)
        msg.add_param("SeqNum", "H", self.client_seq)
        self.client_seq += 1
        msg.body = Message(msg_type, "LM")
        return msg

def login(server_addr, user, password):
    client = Client(addr)
    stage = 0
    session = 0x26ec5075 # XXX Should it be random?
    while True:
        if stage == 0:
            request = client.create_session_message(SM_SessionRequest)
            request.add_param("UnknownA", "I", 0x00000002)
            request.add_param("Session", "I", session)
            request.add_param("MaxLength", "I", 0x00000200)
            client.send(request)
        response = client.receive()
        if response.type == SM_SessionResponse:
            stage = 1
            request = client.create_login_message(LM_GetChatMessage)
            request.body.add_param("UnknownA", "I", 2)
            request.body.add_param("UnknownB", "I", 0)
            request.body.add_param("UnknownC", "I", 0x00080000)
            client.send(request)
        #print(response)

if __name__ == "__main__":
    addr = ("192.168.0.3", 5998)
    user = "foo"
    password = "bar"
    login(addr, user, password)
