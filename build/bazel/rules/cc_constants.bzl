# Constants for cc_* rules.
# To use, load the constants struct:
#
#   load("//build/bazel/rules:cc_constants.bzl", "constants")
# Supported hdr extensions in Soong. Keep this consistent with hdrExts in build/soong/cc/snapshot_utils.go
_HDR_EXTS = ["h", "hh", "hpp", "hxx", "h++", "inl", "inc", "ipp", "h.generic"]
_SRC_EXTS = ["c", "cc", "cpp", "S"]
_ALL_EXTS = _SRC_EXTS + _HDR_EXTS
_HDR_EXTS_WITH_DOT = ["." + ext for ext in _HDR_EXTS]
_SRC_EXTS_WITH_DOT = ["." + ext for ext in _SRC_EXTS]
_ALL_EXTS_WITH_DOT = ["." + ext for ext in _ALL_EXTS]

# These are root-relative.
_GLOBAL_INCLUDE_DIRS_COPTS_ONLY_USED_FOR_SOONG_COMPATIBILITY_DO_NOT_ADD_MORE = [
    "/",
]
constants = struct(
    hdr_exts = _HDR_EXTS,
    src_exts = _SRC_EXTS,
    all_exts = _ALL_EXTS,
    hdr_dot_exts = _HDR_EXTS_WITH_DOT,
    src_dot_exts = _SRC_EXTS_WITH_DOT,
    all_dot_exts = _ALL_EXTS_WITH_DOT,
)
