#!/bin/bash

# Convenience script to run cargo2android.py with the appropriate arguments in the crosvm directory
# and all subdirectories with Cargo.toml files.

set -e

cargo2android() {
  cargo2android.py --run --device --tests --dependencies $@
  rm -r cargo.out target.tmp
}

# Run in the main crosvm directory.
cargo2android --no-subdir

for dir in */src
do
  base=`dirname $dir`
  echo "$base"
  cd "$base"
  # If the subdirectory has more subdirectories with crates, then pass --no-subdir and run it in
  # each of them too.
  if compgen -G "*/Cargo.toml" > /dev/null
  then
    cargo2android --global_defaults=crosvm_defaults --add_workspace --no-subdir

    for dir in */Cargo.toml
    do
      sub_base=`dirname $dir`
      echo "$base/$sub_base"
      cd "$sub_base"
      cargo2android --global_defaults=crosvm_defaults --add_workspace
      cd ..
    done
  else
    cargo2android --global_defaults=crosvm_defaults --add_workspace
  fi

  cd ..
done
