#!/bin/python
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# This program generates the copy commands you should use to update
# the reference data for tests <build-dir>/tests/runtest* that emit an
# output that is compared against a reference output.
# 
# It takes in argument the diff result emitted by
# <build-dir>/tests/runtest*, and emits on standard output a series of
# 'cp <src> <dest>' commands to execute to update reference data of
# the test.

import fileinput
import re
import sys
import getopt


def display_usage():
    sys.stderr.write("usage: prog-name [options] <file-name|-->\n");
    sys.stderr.write(" options:\n");
    sys.stderr.write("  -h  display this help\n");
    sys.stderr.write(" argument:\n");
    sys.stderr.write("  <file-name>  the file to read from\n");
    sys.stderr.write("  if no argument, then reads from stdin\n");

    sys.exit(2)

def main():
    input_file = None

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi", ["help"])
    except getopt.GetoptError as err:
        print(str(err))
        display_usage()

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            display_usage()
        else:
            # unknown option.
            display_usage()

    if input_file == None and len(args) and args[0] != None:
        input_file = open(args[0], 'r')
    elif input_file == None:
        input_file = sys.stdin

    if input_file != None:
        process(input_file)
    else:
        display_usage()


def process(input_file):
    source = ""
    dest = ""
    for line in input_file:
        m = re.match(r'^--- (.*?)\t', line)
        if m:
            dest = m.group(1)
        else:
            m = re.match(r'^\+\+\+ (.*?)\t', line)
            if m:
                source = m.group(1)
                sys.stdout.write("cp " + source + " " + dest + "\n");

if __name__ == "__main__":
    main()
