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
LM_ChatMessageRequest = 0x01
LM_LoginRequest = 0x02
LM_ServerListRequest = 0x04
LM_ChatMessageResponse = 0x16
LM_LoginResponse = 0x17
LM_ServerListResponse = 0x18
LM_Types = {value: name for name, value in globals().items() if name.startswith("LM_")}

# Table used for a byte-wise 32-bit CRC calculation using the Ehternet polynomial (0x04C11DB7).
CRC32Lookup = [
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
]

class Parameter(object):
    def __init__(self, name, code, value):
        self.name = name
        self.code = code
        self.value = value

class Message(object):
    def __init__(self, ns, type):
        self.ns = ns
        self.type = type
        self.params = collections.OrderedDict()
        self.body = None
    
    def add_param(self, name, code, value=None):
        self.params[name] = Parameter(name, code, value)
    
    def serialize(self, crc32_fn=None):
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
            data += self.body
        # Add checksum at the end of the packet.
        if (self.ns == "SM") and (self.type not in (SM_SessionRequest, SM_SessionResponse)):
            data += struct.pack("H", crc32_fn(data))
        return data
    
    def deserialize(self, data):
        if self.ns == "SM":
            byte_order = "!"
        else:
            byte_order = "<"
        patterns = [param.code for param in self.params.values()]
        pattern = byte_order + "".join(patterns)
        total_size = struct.calcsize(pattern)
        #print("deserialize() -> pattern: %s total_size: %d data: %s" % (pattern, total_size, repr(data)))
        values = struct.unpack(pattern, data[0:total_size])

        # Assign the deserialized values to the message parameters.
        param_names = list(self.params)
        value_pos = 0
        for param_index in range(0, len(patterns)):
            name = param_names[param_index]
            pattern = patterns[param_index]
            num_values = len(pattern)
            if num_values == 1:
                param_value = values[value_pos]
            else:
                param_value = values[value_pos:value_pos + num_values]
            self.params[name].value = param_value
            value_pos += num_values

        # Any remaining data belongs to the message body.
        if len(data) > total_size:
            self.body = data[total_size:]
    
    def __getitem__(self, name):
        param = self.params[name]
        return param.value

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

class SessionMessage(Message):
    def __init__(self, type):
        super(SessionMessage, self).__init__("SM", type)

class LoginMessage(Message):
    def __init__(self, type):
        super(LoginMessage, self).__init__("LM", type)

class SessionClient(object):
    def __init__(self, addr, session_id):
        self.addr = addr
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.session_id = session_id
        self.active = False
        self.crc_key = 0
        self.client_seq = 0
        self.server_seq = 0
        self.pending_messages = []
    
    def start_session(self):
        """ Initiate a session with the remote server. """
        request = SessionMessage(SM_SessionRequest)
        request.add_param("UnknownA", "I", 0x00000002)
        request.add_param("Session", "I", self.session_id)
        request.add_param("MaxLength", "I", 0x00000200)
        self._send_session(request)
        response = self._receive_session()
        if response.type != SM_SessionResponse:
            raise Exception("Server did not respond with SessionResponse")
        response_id = response.params["Session"].value
        if response_id != self.session_id:
            raise Exception("Server responded with different session ID: 0x%x, ours: 0x%x"
                % (response_id, self.session_id))
        self.crc_key = response.params["Key"].value
        self.active = True
    
    def send(self, login_msg):
        """ Send a login message to the server. """
        if login_msg.ns != "LM":
            raise ValueError("Not a login message.")
        session_msg = SessionMessage(SM_LoginPacket)
        session_msg.add_param("SeqNum", "H", self.client_seq)
        self.client_seq += 1
        session_msg.body = login_msg.serialize()
        self._send_session(session_msg)
    
    def receive(self):
        """ Receive a login message from the server. """
        login_msg = None
        while not login_msg:
            session_msg = self._receive_session()
            if session_msg.type == SM_SessionDisconnect:
                self.active = False
                break
            elif session_msg.type == SM_Combined:
                pos, data = 0, session_msg.body
                while pos < len(data):
                    sub_msg_len = ord(data[pos])
                    pos += 1
                    if (pos + sub_msg_len) > len(data):
                        raise Exception("Sub-message length out of range.")
                    sub_msg_data = data[pos:pos + sub_msg_len]
                    pos += sub_msg_len
                    self.pending_messages.append(self._parse_session(sub_msg_data, True))
            elif session_msg.type == SM_LoginPacket:
                # Only accept messages in order.
                if self.server_seq == session_msg["SeqNum"]:
                    # Parse the login message data, removing the checksum beforehand.
                    login_msg = self._parse_login(session_msg.body)
                    # Acknowledge that we have received this message.
                    self._send_session_ack()
        return login_msg
    
    def format_addr(self, addr):
        return "%s:%d" % addr
    
    def _send_session(self, msg):
        if msg.ns != "SM":
            raise ValueError("Not a session message.")
        #XXX Figure out why we need to byte-swap the checksum before sending it.
        crc32_fn = lambda data: socket.htons(self._crc32(data) & 0xffff)
        packet = msg.serialize(crc32_fn)
        print("%s >>> %s" % (self.format_addr(self.addr),
                             binascii.b2a_hex(packet).decode('utf8')))
        self.socket.sendto(bytes(packet), self.addr)
    
    def _send_session_ack(self):
        ack_msg = SessionMessage(SM_Ack)
        ack_msg.add_param("SeqNum", "H", self.server_seq)
        self._send_session(ack_msg)
        self.server_seq += 1

    def _receive_session(self, max_size=1024):
        if self.pending_messages:
            return self.pending_messages.pop(0)
        response, remote = self.socket.recvfrom(1024)
        print("%s <<< %s" % (self.format_addr(remote),
                             binascii.b2a_hex(response).decode('utf8')))
        return self._parse_session(response)

    def _crc32_round(self, a, b):
        return (a >> 8) ^ CRC32Lookup[(b ^ a) & 0xff]
    
    def _crc32(self, data):
        crc = 0xffffffff
        crc = self._crc32_round(crc, (self.crc_key >> 0))
        crc = self._crc32_round(crc, (self.crc_key >> 8))
        crc = self._crc32_round(crc, (self.crc_key >> 16))
        crc = self._crc32_round(crc, (self.crc_key >> 24))
        for i in range(0, len(data)):
            crc = self._crc32_round(crc, ord(data[i]))
        crc ^= 0xffffff
        return crc & 0xffffffff
    
    def _parse_session(self, packet, no_crc=False):
        """ Parse a session message. """
        # Extract the message type and CRC and validate them.
        msg_type = struct.unpack("!H", packet[0:2])[0]
        if msg_type > 0xff:
            raise ValueError("Message type out of range: 0x%04x" % msg_type)
        has_crc = (msg_type not in (SM_SessionRequest, SM_SessionResponse) and not no_crc)
        if has_crc:
            crc, packet = struct.unpack("!H", packet[-2:])[0], packet[0:-2]
            computed_crc = self._crc32(packet) & 0xffff
            if (crc != 0) and (crc != computed_crc):
                raise ValueError("Invalid CRC: computed 0x%04x, but found 0x%04x." % (computed_crc, crc))
        packet = packet[2:]
        
        # Specify the header fields for the message, if we know about any.
        msg = SessionMessage(msg_type)
        if msg_type == SM_SessionResponse:
            msg.add_param("Session", "I")
            msg.add_param("Key", "I")
            msg.add_param("UnknownA", "B")
            msg.add_param("Format", "B")
            msg.add_param("UnknownB", "B")
            msg.add_param("MaxLength", "I")
            msg.add_param("UnknownC", "I")
        elif msg_type == SM_LoginPacket:
            msg.add_param("SeqNum", "H")

        # Deserialize the session data, copying any unknown data to the body.
        msg.deserialize(packet)
        return msg
    
    def _parse_login(self, packet):
        """ Parse a login message. """
        # Extract the message type.
        msg_type, packet = struct.unpack("<H", packet[0:2])[0], packet[2:]
        msg = LoginMessage(msg_type)
        
        # Call the function that can parse the message, if it exists.
        try:
            msg_name = LM_Types[msg_type]
        except KeyError:
            pass
        else:
            fn_name = "_parse_%s" % msg_name
            if hasattr(self, fn_name):
                fn = getattr(self, fn_name)
                fn(msg, packet)
        return msg
    
    def _parse_LM_ChatMessageResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "BBB")
        msg.add_param("UnknownD", "I")
        msg.deserialize(packet)
    
    def _parse_LM_LoginResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "H")
        msg.deserialize(packet)
        
def client_login(server_addr, user, password):
    session_id = 0x26ec5075 # XXX Should it be random?
    client = SessionClient(addr, session_id)
    client.start_session()
    stage = 0
    request = LoginMessage(LM_ChatMessageRequest)
    request.add_param("UnknownA", "I", 2)
    request.add_param("UnknownB", "I", 0)
    request.add_param("UnknownC", "I", 0x00080000)
    client.send(request)
    response = client.receive()
    while response:
        handled = False
        if stage == 0:
            # Waiting for a chat message response.
            if response.type == LM_ChatMessageResponse:
                stage = 1
                print("Chat message: %s" % response.body)
                handled = True

                # Try to log in.
                request = LoginMessage(LM_LoginRequest)
                request.add_param("UnknownA", "I", 3)
                request.add_param("UnknownB", "I", 2)
                request.add_param("UnknownC", "H", 0)
                # Allowed packet lengths: 20, 28, 36, 44, etc.
                packet_size = (len(password) + len(user) + 14)
                allowed_size = 20
                while allowed_size < packet_size:
                    allowed_size += 8
                padding = (allowed_size - packet_size + 1)
                credentials_chunks = [password, "\x00", user, "\x00" * padding]
                request.body = "".join(credentials_chunks)
                client.send(request)

        elif stage == 1:
            # Waiting for a login response.
            if response.type == LM_LoginResponse:
                stage = 2
        if not handled:
            print(response)
        response = client.receive()

if __name__ == "__main__":
    addr, user, password = ("192.168.0.3", 5998), "foo", "bar"
    client_login(addr, user, password)
