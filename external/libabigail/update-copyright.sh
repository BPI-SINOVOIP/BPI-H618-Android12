#!/bin/sh
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

oldyear=2019
newyear=2020

for dir in src include tools tests; do
    for ext in cc h; do
	find $dir -maxdepth 1 -name *.$ext \
	    -exec sed -i -r \
	     "s/(Copyright \(C\) .*?-)$oldyear (.*?\.)/\1$newyear \2/" \
	     {} \; \
	     -exec sed -i -r \
	     "s/(Copyright \(C\)) ($oldyear) (.*?\.)/\1 $oldyear-$newyear \3/" \
	     {} \;
    done
done
