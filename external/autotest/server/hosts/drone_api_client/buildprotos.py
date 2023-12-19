"""Deletes the existing bindings, then rebuild using the source .proto file."""

import os
from shutil import copyfile


UP = '../'
PROTO_PATH = 'src/config/proto/chromiumos/config/api/test/tls/'
PROTO_NAME = 'common.proto'
BUILD_CMD = (
    "python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. {}"
    .format(PROTO_NAME))


def delete_old_protos():
    """Delete any existing protos or built proto bindings."""
    for file in os.listdir('.'):
        if 'common' in file:
            os.remove(file)


def copy_proto_from_src():
    """Copy the proto from the src dir to the local dir."""
    proto_dir = get_proto_dir()
    if os.path.isfile(proto_dir):
        copyfile(proto_dir, PROTO_NAME)
    else:
        raise Exception("Protos not found @ {}".format(proto_dir))


def get_proto_dir():
    """Return the full path of the common.proto from TLS."""
    return "{}{}{}".format(UP * get_current_depth(), PROTO_PATH, PROTO_NAME)


def get_current_depth():
    """Return the current depth off /src/ within the file structure."""
    dirs = os.getcwd().split('/')
    src_level = dirs.index('src')
    return len(dirs) - src_level


def create_bindings():
    os.system(BUILD_CMD)


def main():
    delete_old_protos()
    copy_proto_from_src()
    create_bindings()

if __name__ == "__main__":
    main()
