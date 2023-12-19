#!/bin/sh
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Author: Dodji Seketeli <dodji@redhat.com>

from_license_id=
to_license_id=
input_file=
prog=$0

display_usage()
{
    echo "$prog: [options] --from <from-spdx-license-id> --to <to-spdx-license-id> <file>"
    echo "  where options can be:"
    echo "   -h|--h		display this help"
}

main()
{
    header=$(head --lines=5 $input_file | grep "SPDX-License-Identifier:")
    if test "x$header" != x; then
	license=$(echo "$header" | sed -r "s/^.*(SPDX-License-Identifier:)[ 	]*([^*/]+).*$/\2/")
    fi

    if test "x$license" != x -a "$license" = "$from_license_id"; then
	sed -i -r "s/$from_license_id/$to_license_id/" $input_file
	exit 0
    fi

    exit 1
}

# This program takes at least 5 arguments
if test $# -lt 5; then
    >&2 display_usage
    exit 1
fi

# Parse parameters
while test $# -gt 1; do
    case "$1" in
	-h|--h)
	    display_usage
	    exit 0
	    ;;

	-f|--from)
	    from_license_id=$2
	    shift
	    ;;

	-t|--to)
	    to_license_id=$2
	    shift
	    ;;

	-*)
	    >&2 display_usage
	    exit 1
	    ;;

	*)
	    input_file="$1"
	    ;;
    esac
    shift
done

if test $# -lt 1; then
    >&2 display_usage
    exit 1
fi

input_file=$1

main
