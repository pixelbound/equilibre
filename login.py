# Copyright (C) 2013 PiB <pixelbound@gmail.com>
# 
# EQuilibre is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import collections
import binascii
import time
import sys
import argparse
import network

def client_login(args):
    client = network.LoginClient()
    if args.dump_packets:
        client.dump_prefix = "packet_login_%s" % time.strftime("%Y%m%d_%H-%M-%S")
    client.connect((args.host, args.port))
    with client:
        stage = 0
        server_by_id = {}
        session_key = None
        client.begin_get_chat_message()
        response = client.receive()
        while response:
            handled = False
            if (stage == 0) and (response.type == network.LM_ChatMessageResponse):
                # Waiting for a chat message response.
                print("Chat message: %s" % client.end_get_chat_message(response))
                stage = 1
                handled = True
                client.begin_login(args.user, args.password)
            elif (stage == 1) and (response.type == network.LM_LoginResponse):
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
            elif (stage == 2) and (response.type == network.LM_ServerListResponse):
                # Waiting for a server list response.
                servers = client.end_list_servers(response)
                print("%d servers online." % len(servers))
                for i, server in enumerate(servers):
                    server_by_id[server.runtime_id] = server
                    print("%d: %s (%d players, ID=%d)" % (i, server.name,
                        server.players, server.runtime_id))
                # Login in the first server in the list.
                handled = True
                if servers:
                    stage = 3
                    sequence = 5
                    client.begin_play(servers[0].runtime_id, sequence)
                else:
                    print("No server to connect to, exiting.")
                    break
            elif (stage == 3) and (response.type == network.LM_PlayResponse):
                # Waiting for a play response.
                handled = True
                success, status, server_id, sequence = client.end_play(response)
                server = server_by_id[server_id]
                if success:
                    print("Server '%s' (ID=%d) allowed play." % (server.name, server_id))
                    # HACK: give the login server a chance to tell the world
                    # server that we can log in.
                    time.sleep(0.1)
                    client_play(args, server, sequence, session_key)
                else:
                    print("Server '%s' (ID=%d) disallowed play, reason = %d." %
                        (server.name, server_id, status))
                    break
            if not handled:
                print(response)
            response = client.receive()

def client_play(args, server, sequence, session_key):
    client = network.WorldClient()
    if args.dump_packets:
        client.dump_prefix = "packet_world_%s" % time.strftime("%Y%m%d_%H-%M-%S")
    server_addr = (server.host, 9000)
    client.connect(server_addr)
    with client:
        stage = 0
        client.begin_login(sequence, session_key)
        response = client.receive()
        while response:
            handled = False
            #if (stage == 0) and (response.type == LM_ChatMessageResponse):
            #    # Waiting for a login response.
            #    print("Chat message: %s" % client.end_get_chat_message(response))
            #    stage = 1
            #    handled = True
            #    client.begin_login(username, password)
            if not handled:
                print(response)
            response = client.receive()

def main():
    parser = argparse.ArgumentParser(description='Connect to an EQEmu login server.')
    parser.add_argument("-H", "--host", default="localhost")
    parser.add_argument("-P", "--port", type=int, default=5998)
    parser.add_argument("-u", "--user", default='user')
    parser.add_argument("-p", "--password", default='password')
    parser.add_argument("--dump-packets", action='store_true', default=False)
    args = parser.parse_args()
    try:
        client_login(args)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
