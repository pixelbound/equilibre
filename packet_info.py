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

    def message(self, msg, depth):
        # We print packet file names in verbose mode and need to indent more.
        if self.args.verbose:
            depth += 1
        indent_txt = " " * (depth * 4)
        print("%s%s" % (indent_txt, msg))

    def info(self, file, packet):
        self.info_session(file, packet, False, 0)

    def info_session(self, file, packet, unwrapped, depth):
        msg = self.session.parse_packet(packet, unwrapped)
        verbose_msg = (msg.type in (network.SM_Ack, network.SM_Fragment))
        if (not self.args.verbose) and verbose_msg:
            return
        if (depth == 0) and self.args.verbose:
            print("Packet '%s'" % file)
        self.message(str(msg), depth)
        if msg.type == network.SM_ApplicationPacket:
            self.info_app(msg.body, depth + 1)
        elif msg.type == network.SM_Combined:
            sub_packets = msg.unpack_combined()
            for sub_packet in sub_packets:
                self.info_session(file, sub_packet, True, depth + 1)
        elif msg.type == network.SM_Fragment:
            pass
        elif msg.body:
            self.message(binascii.b2a_hex(msg.body), depth)
    
    def info_app(self, packet, depth):
        app_msg = self.app_client.parse_packet(packet)
        self.message(str(app_msg), depth)
        if app_msg.body:
            self.message(binascii.b2a_hex(app_msg.body), depth)
            txt = repr(app_msg.body)
            escaped_txt = re.sub(r"\\x[0-9a-fA-F]{2}", ".", txt)
            self.message(escaped_txt, depth)

def main():
    parser = argparse.ArgumentParser(description='Interpret EQ packet files.')
    parser.add_argument('files', metavar='FILE', type=str, nargs="+",
                   help='packet files to interpret')
    parser.add_argument("-c", "--crc", type=int, default=0x11223344,
                   help='CRC key used to verify packets')
    parser.add_argument("-n", "--namespace", default="WM",
                   help='Message namespace (LM for login messages, WM for world messages)')
    parser.add_argument("-v", "--verbose", action='store_true', default=False)
    args = parser.parse_args()
    p = PacketInfo(args)
    for file in args.files:
        with open(file, "rb") as f:
            try:
                p.info(file, f.read())
            except Exception as e:
                print("error while reading packet '%s': %s" % (file, e))

if __name__ == "__main__":
    main()
