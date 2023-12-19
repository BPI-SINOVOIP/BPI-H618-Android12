# OSS-Fuzz for CRAS

This directory contains source code and build scripts for coverage-guided
fuzzers.

Detailed instructions are available at: https://github.com/google/oss-fuzz/blob/master/docs/

## Quick start

### Sudoless Docker
```
sudo adduser $USER docker
```
### Sync to the latest base-builder
```
docker pull gcr.io/oss-fuzz-base/base-builder
```

### Build a container from the adhd directory
```
docker build -t ossfuzz/cras -f cras/src/fuzz/Dockerfile .
```
Add `--no-cache` if you want a complete rebuild.

### Build fuzzers
```
docker run --cap-add=SYS_PTRACE -ti --rm -v /tmp/fuzzers:/out ossfuzz/cras
```

### Look in /tmp/fuzzers to see the executables. Run them like so:
```
docker run --cap-add=SYS_PTRACE -ti -v $(pwd)/cras/src/fuzz/corpus:/corpus \
    -v /tmp/fuzzers:/out ossfuzz/cras /out/rclient_message \
    /corpus -runs=100
```

### Debug in docker

Go into docker console by
```
docker run --cap-add=SYS_PTRACE -ti -v $(pwd)/cras/src/fuzz/corpus:/corpus \
    -v /tmp/fuzzers:/out ossfuzz/cras /bin/bash
```
and start debugging.
