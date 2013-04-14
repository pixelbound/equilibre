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
    print("Packet has length %d" % len(packet))

def main():
    parser = argparse.ArgumentParser(description='Interpret EQ packet files.')
    parser.add_argument('files', metavar='FILE', type=str, nargs="+",
                   help='packet files to interpret')
    args = parser.parse_args()
    for file in args.files:
        print("Reading packet '%s'." % file)
        with open(file, "rb") as f:
            packet_info(args, f.read())     

if __name__ == "__main__":
    main()
