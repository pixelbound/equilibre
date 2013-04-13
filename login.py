import socket
import struct
import collections
import binascii
import sys
import argparse

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
    
    def has_param(self, name):
        return name in self.params

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
    def __init__(self, addr):
        self.addr = addr
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.crc_key = 0
        self.next_ack_in = 0
        self.next_seq_in = 0
        self.next_seq_out = 0
        self.pending_packets = []
    
    def close(self):
        self.socket.close()

    def receive(self):
        """ Receive a session message from the server. """
        while True:
            packet, crc_removed = self._receive_packet()
            if not packet:
                # XXX Is it really possible to receive zero from recvfrom?
                return None
            session_msg = self._parse_packet(packet, crc_removed)
            if session_msg.type == SM_Ack:
                seq_num = session_msg["SeqNum"]
                if seq_num >= self.next_seq_out:
                    raise Exception("Received ack for message that was not sent: %d (seq_out = %d)"
                        % (seq_num, self.next_seq_out - 1))
                self.next_ack_in = seq_num + 1
            elif session_msg.type == SM_Combined:
                pos = 0
                data = session_msg.body
                while pos < len(data):
                    sub_msg_len = ord(data[pos])
                    pos += 1
                    if (pos + sub_msg_len) > len(data):
                        raise Exception("Sub-message length out of range.")
                    sub_msg_data = data[pos:pos + sub_msg_len]
                    pos += sub_msg_len
                    self.pending_packets.append((sub_msg_data, True))
            elif self._has_seq_num(session_msg.type):
                # Only accept messages in order.
                seq_num = session_msg["SeqNum"]
                if self.next_seq_in == seq_num:
                    self._send_ack(seq_num)
                    return session_msg
            else:
                return session_msg
    
    def send(self, msg):
        """ Send a session message to the server. """
        if msg.ns != "SM":
            raise ValueError("Not a session message.")
        elif self._has_seq_num(msg.type):
            msg.add_param("SeqNum", "H", self.next_seq_out)
            self.next_seq_out += 1
        #XXX Figure out why we need to byte-swap the checksum before sending it.
        crc32_fn = lambda data: socket.htons(self._crc32(data) & 0xffff)
        self._send_packet(msg.serialize(crc32_fn))
    
    def _format_addr(self, addr):
        return "%s:%d" % addr
    
    def _send_ack(self, seq_num):
        if seq_num != self.next_seq_in:
            raise ValueError("Sending ack for message that was not received: %d (seq_in = %d)"
                % (seq_num, self.next_seq_in - 1))
        ack_msg = SessionMessage(SM_Ack)
        ack_msg.add_param("SeqNum", "H", seq_num)
        self.send(ack_msg)
        self.next_seq_in += 1
    
    def _send_packet(self, packet):
        """ Send a session packet to the server. """
        print("%s >>> %s" % (self._format_addr(self.addr),
                             binascii.b2a_hex(packet).decode('utf8')))
        self.socket.sendto(bytes(packet), self.addr)
    
    def _receive_packet(self, max_size=1024):
        """ Return a session packet from the server. """
        if self.pending_packets:
            packet, crc_removed = self.pending_packets.pop(0)
            remote = self.addr
        else:
            packet, remote = self.socket.recvfrom(1024)
            crc_removed = False
            print("%s <<< %s" % (self._format_addr(remote),
                                 binascii.b2a_hex(packet).decode('utf8')))
        return packet, crc_removed
    
    def _has_seq_num(self, sm_type):
        """ Determine whether the message has a sequence number parameter and
        whether it should be incremented before sending it. """
        return sm_type in (SM_LoginPacket, )

    def _crc32_round(self, a, b):
        return (a >> 8) ^ CRC32Lookup[(b ^ a) & 0xff]
    
    def _crc32(self, data):
        crc = binascii.crc32(struct.pack("<I", self.crc_key))
        crc = binascii.crc32(data, crc)
        return crc & 0xffffffff
    
    def _parse_packet(self, packet, no_crc=False):
        """ Parse a session packet. """
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
        elif msg_type == SM_Ack:
            msg.add_param("SeqNum", "H")

        # Deserialize the session data, copying any unknown data to the body.
        msg.deserialize(packet)
        return msg

class LoginClient(object):
    """ High-level interface to talk to a login server. """
    def __init__(self):
        self.session_id = 0x26ec5075 # XXX Should it be random?
        self.session = None
    
    def __enter__(self):
        pass
    
    def __exit__(self, type, value, tb):
        self.disconnect()
    
    def connect(self, remote_addr):
        """ Initiate a session with the remote server. """
        if self.session:
            pass
        session = SessionClient(remote_addr)
        connected = False
        try:
            request = SessionMessage(SM_SessionRequest)
            request.add_param("UnknownA", "I", 0x00000002)
            request.add_param("Session", "I", self.session_id)
            request.add_param("MaxLength", "I", 0x00000200)
            session.send(request)
            response = session.receive()
            if response.type != SM_SessionResponse:
                raise Exception("Server did not respond with SessionResponse")
            response_id = response["Session"]
            if response_id != self.session_id:
                raise Exception("Server responded with different session ID: 0x%x, ours: 0x%x"
                    % (response_id, self.session_id))
            session.crc_key = response["Key"]
            connected = True
        finally:
            if not connected:
                session.close()
        self.session = session
    
    def disconnect(self):
        if not self.session:
            return
        try:
            request = SessionMessage(SM_SessionDisconnect)
            request.add_param("Session", "I", self.session_id)
            request.add_param("UnknownA", "H", 6)
            self.session.send(request)
        finally:
            self.session.close()
            self.session = None
    
    def receive(self):
        """ Wait until a message has been received from the server. """
        session_msg = self.session.receive()
        if (not session_msg) or (session_msg.type == SM_SessionDisconnect):
            self.active = False
            return None
        elif session_msg.type == SM_LoginPacket:
            return self._parse_packet(session_msg.body)
        else:
            print("Unexpected session message: %s" % session_msg)
            return None
    
    def begin_get_chat_message(self):
        request = LoginMessage(LM_ChatMessageRequest)
        request.add_param("UnknownA", "I", 2)
        request.add_param("UnknownB", "I", 0)
        request.add_param("UnknownC", "H", 8)
        request.add_param("UnknownD", "H", 0)
        self.send(request)
    
    def begin_login(self, username, password):
        request = LoginMessage(LM_LoginRequest)
        request.add_param("UnknownA", "I", 3)
        request.add_param("UnknownB", "I", 2)
        request.add_param("UnknownC", "H", 0)
        # Allowed packet lengths: 20, 28, 36, 44, etc.
        packet_size = (len(password) + len(username) + 14)
        allowed_size = 20
        while allowed_size < packet_size:
            allowed_size += 8
        padding = (allowed_size - packet_size + 1)
        credentials_chunks = [password, "\x00", username, "\x00" * padding]
        request.body = "".join(credentials_chunks)
        self.send(request)
    
    def begin_list_servers(self):
        request = LoginMessage(LM_ServerListRequest)
        request.add_param("UnknownA", "I", 4)
        request.add_param("UnknownB", "I", 0)
        request.add_param("UnknownC", "H", 0)
        self.send(request)
    
    def end_get_chat_message(self, response):
        """ Extract the chat message from the response. """
        return response.body
    
    def end_login(self, response):
        """ Extract the login result from the response.
        This is a tuple with the values: success, user ID and session key. """
        result_fields = struct.unpack("IIi16sI", response.body[0:32])
        status, unknown_a, user_id, key, failed_attempts = result_fields
        if (status != 1) or (user_id == -1):
            return False, user_id, None
        return True, user_id, key

    def end_list_servers(self, response):
        msg = response
        servers = []
        pos = 0
        while msg.body and (pos < len(msg.body)):
            server = ServerInfo()
            server.host, pos = self._read_c_string(msg.body, pos)
            server.type, pos = self._read_field(msg.body, pos, "I")
            server.runtime_id, pos = self._read_field(msg.body, pos, "I")
            server.name, pos = self._read_c_string(msg.body, pos)
            locale_1, pos = self._read_c_string(msg.body, pos)
            locale_2, pos = self._read_c_string(msg.body, pos)
            server.locale = (locale_1, locale_2)
            server.status, pos = self._read_field(msg.body, pos, "I")
            server.players, pos = self._read_field(msg.body, pos, "I")
            servers.append(server)
        return servers

    def send(self, login_msg):
        """ Send a login message to the server. """
        if login_msg.ns != "LM":
            raise ValueError("Not a login message.")
        session_msg = SessionMessage(SM_LoginPacket)
        session_msg.body = login_msg.serialize()
        self.session.send(session_msg)

    def _parse_packet(self, packet):
        """ Parse a login message from a received packet. """
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
    
    def _read_c_string(self, data, pos):
        nul_pos = data.index("\0", pos)
        return data[pos:nul_pos], nul_pos + 1
    
    def _read_field(self, data, pos, pattern):
        size = struct.calcsize(pattern)
        return struct.unpack(pattern, data[pos:pos+size])[0], pos + size
    
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
    
    def _parse_LM_ServerListResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "I")
        msg.add_param("UnknownD", "I")
        msg.add_param("ServerCount", "I")
        msg.deserialize(packet)

class ServerInfo(object):
    def __init__(self):
        self.type = 0
        self.runtime_id = 0
        self.status = 0
        self.players = 0
        self.host = None
        self.name = None
        self.locale = None

def client_login(server_addr, username, password):
    client = LoginClient()
    client.connect(server_addr)
    with client:
        stage = 0
        client.begin_get_chat_message()
        response = client.receive()
        while response:
            handled = False
            if (stage == 0) and (response.type == LM_ChatMessageResponse):
                # Waiting for a chat message response.
                print("Chat message: %s" % client.end_get_chat_message(response))
                stage = 1
                handled = True
                client.begin_login(username, password)
            elif (stage == 1) and (response.type == LM_LoginResponse):
                # Waiting for a login response.
                success, user_id, session_key = client.end_login(response)
                if success:
                    print("Successfully logged in.")
                else:
                    print("Failed to logd in.")
                    break
                stage = 2
                handled = True
                client.begin_list_servers()
            elif (stage == 2) and (response.type == LM_ServerListResponse):
                # Waiting for a server list response.
                handled = True
                servers = client.end_list_servers(response)
                print("%d servers online." % len(servers))
                for i, server in enumerate(servers):
                    print("%d: %s (%d players)" % (i, server.name, server.players))
                break
            if not handled:
                print(response)
            response = client.receive()

def main():
    parser = argparse.ArgumentParser(description='Connect to an EQEmu login server.')
    parser.add_argument("-H", "--host", default="localhost")
    parser.add_argument("-P", "--port", type=int, default=5998)
    parser.add_argument("-u", "--user", default='user')
    parser.add_argument("-p", "--password", default='password')
    args = parser.parse_args()
    try:
        client_login((args.host, args.port), args.user, args.password)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
