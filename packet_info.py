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

import binascii
import argparse
import network

def packet_info(args, packet):
    # XXX SessionState
    session = network.SessionClient(('127.0.0.1', 0))
    session.crc_key = args.crc
    if args.namespace == "LM":
        app_client = network.LoginClient()
    else:
        app_client = network.WorldClient()
        session.compressed = True
    msg = session.parse_packet(packet)
    print(msg)
    print(binascii.b2a_hex(msg.body))
    if msg.type == network.SM_ApplicationPacket:
        app_msg = app_client.parse_packet(msg.body)
        print("    " + str(app_msg))
        print("    " + binascii.b2a_hex(app_msg.body))
    print("")

def main():
    parser = argparse.ArgumentParser(description='Interpret EQ packet files.')
    parser.add_argument('files', metavar='FILE', type=str, nargs="+",
                   help='packet files to interpret')
    parser.add_argument("-c", "--crc", type=int, default=0x11223344,
                   help='CRC key used to verify packets')
    parser.add_argument("-n", "--namespace", default="WM",
                   help='Message namespace (LM for login messages, WM for world messages)')
    args = parser.parse_args()
    for file in args.files:
        print("Reading packet '%s'." % file)
        with open(file, "rb") as f:
            packet_info(args, f.read())     

if __name__ == "__main__":
    main()
