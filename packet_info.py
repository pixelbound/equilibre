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
import re

class PacketInfo(object):
    def __init__(self, args):
        self.args = args
        # XXX SessionState
        self.session = network.SessionClient(('127.0.0.1', 0))
        self.session.crc_key = args.crc
        if args.namespace == "LM":
            self.app_client = network.LoginClient()
        else:
            self.app_client = network.WorldClient()
            self.session.compressed = True

    def info(self, packet):
        self.info_session(packet, False, 0)

    def info_session(self, packet, unwrapped, indent):
        indent_txt = " " * indent
        msg = self.session.parse_packet(packet, unwrapped)
        print("%s%s" % (indent_txt, str(msg)))
        if msg.type == network.SM_ApplicationPacket:
            self.info_app(msg.body, indent + 4)
        elif msg.type == network.SM_Combined:
            sub_packets = msg.unpack_combined()
            for sub_packet in sub_packets:
                self.info_session(sub_packet, True, indent + 4)
        else:
            print("%s%s" % (indent_txt, binascii.b2a_hex(msg.body)))
    
    def info_app(self, packet, indent):
        indent_txt = " " * indent
        app_msg = self.app_client.parse_packet(packet)
        print("%s%s" % (indent_txt, str(app_msg)))
        print("%s%s" % (indent_txt, binascii.b2a_hex(app_msg.body)))
        txt = repr(app_msg.body)
        escaped_txt = re.sub(r"\\x[0-9a-fA-F]{2}", ".", txt)
        print("%s%s" % (indent_txt, escaped_txt))

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
            p = PacketInfo(args)
            p.info(f.read())     

if __name__ == "__main__":
    main()
