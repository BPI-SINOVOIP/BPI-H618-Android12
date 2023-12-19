#!/bin/bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

for i in *.svg;
  do inkscape "$i" --export-plain-svg="${i//svg/plain.svg}"
done
