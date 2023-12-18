This directory and its subdirectories are a partial fork of the
[`bazel-skylib`](https://github.com/bazelbuild/bazel-skylib)
github repository, for experimental use with Bazel builds.

Not all files in `bazel-skylib` are included. When a file in this directory diverges
from `bazel-skylib`, add a comment containing `Divergence from bazel-skylib` which
explains the need for this divergence.

It should be a goal to follow HEAD `bazel-skylib` as closely as possible, with
necessary changes made upstream ASAP.
