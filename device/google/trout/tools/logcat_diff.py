#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# A tool that can take two logcat files from an Android device
# and produce a compact rendering of the delta between them
# Namely, it strips most of the information from the output
# (timestamp, process id, ...) that is often not critical in
# reconstructing the state of the device operation
# and then compares the component and message for each line

# It requires 'diff' and 'colordiff' to be installed

import argparse
import os.path
import re
import subprocess
import sys
import tempfile

RE_TEXT = '^[\d]+-[\d]+ [\d]+:[\d]+:[\d]+.[\d]+[ ]+[\d]+[ ]+[\d]+ [DIEWF] (?P<component>.+): (?P<message>.*)$'

# TODO(egranata): should we also support a by-component mode?
class Logcat(object):
    def __init__(self, fp, keep_mismatches=False):
        self.filename = fp
        self.mismatches = 0
        self.messages = []
        self.rgx = re.compile(RE_TEXT, re.MULTILINE)
        with open(fp, 'r') as f:
            for line in f.readlines():
                match = self.rgx.match(line)
                if match:
                    component = match.group('component')
                    message = match.group('message')
                    self.messages.append("%s: %s" % (component, message))
                elif keep_mismatches:
                    self.messages.append(line)
                else:
                    self.mismatches += 1
    def save(self):
        # Keep this file around so we can use it by name elsewhere
        with tempfile.NamedTemporaryFile(mode='w',
            delete=False, prefix=os.path.basename(self.filename)) as f:
            for msg in self.messages:
                f.write("%s\n" % msg)
            return f

def entry():
    parser = argparse.ArgumentParser(description='Compare two Android logs')
    parser.add_argument('files', metavar='<file>', type=str, nargs=2,
                    help='The two files to compare')
    parser.add_argument('--keep-misformatted-lines', '-k', action='store_true', default=False)
    args = parser.parse_args()

    logcat1 = Logcat(args.files[0],
        keep_mismatches=args.keep_misformatted_lines)
    logcat2 = Logcat(args.files[1],
        keep_mismatches=args.keep_misformatted_lines)

    file1 = logcat1.save()
    file2 = logcat2.save()

    if logcat1.mismatches != 0 or logcat2.mismatches != 0:
        print("%d lines were ignored; run with --keep-misformatted-lines to include them" \
            % (logcat1.mismatches + logcat2.mismatches))

    out = subprocess.getoutput('diff --minimal -u -W 200 "%s" "%s" | colordiff' % ( \
            file1.name, file2.name))
    print(out)

    os.remove(file1.name)
    os.remove(file2.name)

if __name__ == '__main__':
    entry()
