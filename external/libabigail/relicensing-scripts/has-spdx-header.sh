#!/bin/sh
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# Author: Dodji Seketeli <dodji@redhat.com>

file=
prog=$0
display_file_name=
be_quiet=
show_no_spdx=

display_usage()
{
    echo "$prog: [options] file"
    echo "  where options can be:"
    echo "   -h|--help		display this help"
    echo "   -f|--file		prefix output with file name"
    echo "   -q|--quiet		emit no output if no license was found"
    echo "   --show-no-spdx	show file name if it has no spdx header"
}

emit_output_no_license()
{
    if test x$show_no_spdx != x; then
	echo $display_file_name
    elif test x$be_quiet = x; then
	if test "x$display_file_name" = x; then
	    echo "NO-LICENSE"
	else
	    echo "$display_file_name: NO-LICENSE"
	fi
    fi

    exit 1
}

emit_output_with_license()
{
    license=$1
    if test x$show_no_spdx != x; then
	:
    elif test "x$display_file_name" = x; then
	echo "$license"
    else
	echo "$display_file_name: $license"
    fi
    exit 0
}

main()
{
    license=$(head --lines=5 $file | sed -n -E "s/^.*(SPDX-License-Identifier:)[ 	]*([^*/]+).*$/\2/p")

    if test "x$license" = x; then
	emit_output_no_license
    else
	emit_output_with_license "$license"
    fi
}

while test $# -ge 1
do
    case "$1" in
	-h|--help)
	    display_usage
	    exit 0
	    ;;

	-f|--file)
	    if test x$2 = x; then
		>&2 display_usage
		exit 1
	    fi
	    display_file_name=$2
	    shift
	    ;;

	-q|--quiet)
	    be_quiet=yes
	    shift
	    ;;

	--show-no-spdx)
	    if test x$2 = x; then
		>&2 display_usage
		exit 1
	    fi
	    show_no_spdx=yes
	    display_file_name=$2
	    shift
	    ;;

	*)
	    break
	    ;;
    esac
done

if test $# -lt 1; then
    >&2 display_usage
    exit 1
fi

file=$1

main
