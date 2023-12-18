This directory and its subdirectories are a partial fork of the
[`rules_cc`]((https://github.com/bazelbuild/rules_cc/)
github repository, for experimental use with Bazel builds.

To reduce the number of dependencies of this directory, not all files in `rules_cc`
are included.

When a file in this directory diverges
from `rules_cc`, add a comment containing `Divergence from rules_cc` which
explains the need for this divergence.

This directory must follow HEAD `rules_cc` as closely as possible, with
necessary changes made upstream ASAP.
