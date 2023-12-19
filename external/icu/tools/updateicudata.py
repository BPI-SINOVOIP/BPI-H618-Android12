#!/usr/bin/python -B

"""Regenerates (just) ICU data files used in the Android system image."""

from __future__ import print_function

import os
import sys

import i18nutil
import icuutil


# Run with no arguments from any directory, with no special setup required.
def main():
  icu_dir = icuutil.icuDir()
  print('Found icu in %s ...' % icu_dir)

  makeIcuDataFiles()

  # if icu4c/source/data/misc/langInfo.txt is re-generated, the binary data files need to be
  # re-generated. makeIcuDataFiles() are called until it coverages because the re-generation
  # depends icu4j, and icu4j depends on the bigit nary data files.
  while (icuutil.RequiredToMakeLangInfo()):
    makeIcuDataFiles()


  print('Look in %s for new data files' % icu_dir)
  sys.exit(0)

def makeIcuDataFiles():
  i18nutil.SwitchToNewTemporaryDirectory()
  icu_build_dir = '%s/icu' % os.getcwd()

  icuutil.PrepareIcuBuild(icu_build_dir)

  icuutil.MakeAndCopyIcuDataFiles(icu_build_dir)


if __name__ == '__main__':
  main()
