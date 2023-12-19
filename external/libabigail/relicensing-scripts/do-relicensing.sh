#!/bin/sh
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Author: Dodji Seketeli <dodji@redhat.com>

#This does the actual re-licensing
grep '^\.' relicensing-scripts/files-with-lgplv3.txt | while read file
do
  echo ./relicensing-scripts/replace-spdx-license.sh --from "LGPL-3.0-or-later" --to "Apache-2.0 WITH LLVM-exception" "$file"
done
