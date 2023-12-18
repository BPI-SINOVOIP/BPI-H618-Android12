#!/bin/bash -eu

JQARGS=""

function print_usage() {
  echo "Usage: query.sh [-C] <command> <graph JSON> [argument]" 1>&2
  echo "  -C: colorized output" 1>&2
  exit 1
}

while getopts "C" arg; do
  case "$arg" in
    C)
      JQARGS="$JQARGS -C"
      shift
      ;;
    *)
      print_usage
  esac
done

if [[ "$#" -lt 2 ]]; then
  print_usage
fi

COMMAND="$1"
GRAPH="$2"

if [[ "$#" -gt 2 ]]; then
  ARG="$3"
else
  ARG=""
fi

LIBDIR="$(dirname "$(readlink -f "$0")")"

jq $JQARGS -L "$LIBDIR" -f "$LIBDIR/$COMMAND".jq "$GRAPH" --arg arg "$ARG"
