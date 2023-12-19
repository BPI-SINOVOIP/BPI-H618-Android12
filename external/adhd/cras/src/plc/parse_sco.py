# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
A script to extract raw SCO RX packets from btsnoop.
Use 'btmon -S' to dump SCO traffic from btsnoop file.
Trim the btsnoop output to just the SCO traffic period.
Then execute 'python parse-sco.py <btsnoop-output>'
"""

import atexit
import binascii
import os
import re
import sys


class SCOParser:
  """
  Parser for grepping SCO packets
  """

  def __init__(self):
    # On old releases, +CIEV: 4,1 indicates the start point of call session
    # c 31 0d 0a 9a     ..+CIEV: 4,1..
    self.call_start_re = re.compile(r'.*?\+CIEV:\s4,(\d).*?')

    # > SCO Data RX: Handle 257 flags 0x00 dlen 60           #13826 [hci0] 650.388305
    #         00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    #         00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    #         00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    #         00 00 00 00 00 00 00 00 00 00 00 00
    self.sco_rx_re = re.compile(r'.*?SCO\sData\sRX.*?flags\s0x(\d+).*?')
    self.sco_f = None
    self.output_idx = 0
    self.pk_count = 0
    self.pl_count = 0

    atexit.register(self._cleanup)

  def _cleanup(self):
    if self.sco_f is not None:
      print(
          "Current file contains %d packets (%d with erroneous status flag)" %
          (self.pk_count, self.pl_count))
      self.pk_count = 0
      self.pl_count = 0
      self.sco_f.close()

  def _new_session(self):
    if self.sco_f is not None:
      close(self.sco_f)

    new_file = "sco_file_%d" % self.output_idx
    print("Record to %s" % new_file)
    self.sco_f = open(new_file, 'wb')
    self.output_idx += 1

    return self.sco_f

  def parse(self, filename):
    if not os.path.exists(filename):
      print("%s doesn't exist" % filename)
      return

    print("Start parsing %s" % filename)
    parse_rx_data = 0
    with open(filename, "r") as f:
      for line in f.readlines():
        if parse_rx_data > 0:
          self.sco_f.write(binascii.unhexlify(''.join(line[:56].split())))
          parse_rx_data = (parse_rx_data + 1) % 5

        # Start a new session and output following SCO data to a new file
        match = self.call_start_re.search(line)
        if match and (1 == int(match.group(1))):
          self._new_session()
          continue

        match = self.sco_rx_re.search(line)
        if match:
          if self.sco_f is None:
            self._new_session()

          self.pk_count += 1

          status_flag = int(match.group(1))
          hdr = ['01', str(status_flag) + '1', '3c']
          if status_flag != 0:
            self.pl_count += 1

          self.sco_f.write(binascii.unhexlify(''.join(hdr)))
          parse_rx_data = 1


def main(argv):
  if len(argv) < 1:
    print("parse_sco.py [btsnoop.txt]")
    return

  p = SCOParser()
  p.parse(argv[0])


if __name__ == "__main__":
  main(sys.argv[1:])
