#!/bin/bash
CURDIR=$(cd $(dirname $0) && pwd)
export PYTHONHOME="$CURDIR/../python3"
export DYLD_LIBRARY_PATH="$CURDIR/../python3/lib:$DYLD_LIBRARY_PATH"
"$CURDIR/lldb" "$@"
