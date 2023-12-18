#  Copyright (C) 2021 The Android Open Source Project
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

import argparse
from pathlib import Path

def main():
    args_parser = argparse.ArgumentParser(description='Generate index for test apps')
    args_parser.add_argument('--directory', help='Directory containing test apps')
    args = args_parser.parse_args()

    pathlist = Path(args.directory).rglob('*.apk')
    file_names = [p.name for p in pathlist]

    # TODO(scottjonathan): Replace this with a proto with actual details
    with open(args.directory + "/index.txt", "w") as outfile:
        for file_name in file_names:
            print(file_name.rsplit(".", 1)[0], file=outfile)

if __name__ == "__main__":
    main()