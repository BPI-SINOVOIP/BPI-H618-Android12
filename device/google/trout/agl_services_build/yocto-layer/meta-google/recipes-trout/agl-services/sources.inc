# TODO(chenhaosjtuacm): set it to the correct license, license file is needed
LICENSE = "CLOSED"

# Since repo is not working correctly in our case (b/165933540), we simulate
# the behavior of device/google/trout/agl_services_build/repo_manifest.xml
# in the unpacking task.

ANDROID_REMOTE = "git://partner-android.googlesource.com"
BORINGSSL_REMOTE = "git://boringssl.googlesource.com"
GITHUB_REMOTE = "git://github.com"

FETCH_CODE_PREFIX = "src"

S = "${WORKDIR}/${FETCH_CODE_PREFIX}"

# Pull the source from git server remote/name to path
def trout_git_uri(d, remote, name, path, revision):
    return '{}/{};protocol=https;nobranch=1;' \
           'destsuffix={}/{};rev={}'.format(
               d.getVar(remote), name,
               d.getVar('FETCH_CODE_PREFIX'), path, revision)

# Non-Trout Android code revisions are matching those in Trout R QPR1 PDK
# Except hardware/interfaces (TODO: chenhaosjtuacm, 166706927)
# TODO (chenhaosjtuacm): generate SRC_URI from the manifest

SRC_URI = "\
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'device/google/trout', 'device/google/trout', 'rvc-qpr-dev')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/external/fmtlib', 'third_party/fmtlib', 'aa6fa3aa882afe47f7af990b0c6d747a01cd6104')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/external/grpc-grpc', 'third_party/grpc-grpc', '75bfcd05212333d0f4247865e3a61eaf6092d932')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/external/jsoncpp', 'third_party/jsoncpp', 'ceed690fefd71a34154781da01b3bb0294a486eb')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/external/protobuf', 'third_party/protobuf', '2305946dc587fb5766d207c7e3981375b2aaf5ce')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/hardware/interfaces', 'hardware/interfaces', '229e07f9b922522e1612b6dd930919cf58c8865f')} \
    ${@trout_git_uri(d, 'ANDROID_REMOTE', 'platform/system/core', 'third_party/android/system_core', 'b1ee3a99d1fc3d41606555e29da484df8a66ed71')} \
    ${@trout_git_uri(d, 'BORINGSSL_REMOTE', 'boringssl', 'third_party/boringssl', '0b710a305b42b67522003a314dea3e3868485665')} \
    ${@trout_git_uri(d, 'GITHUB_REMOTE', 'c-ares/c-ares', 'third_party/cares', '3be1924221e1326df520f8498d704a5c4c8d0cce')} \
    ${@trout_git_uri(d, 'GITHUB_REMOTE', 'gflags/gflags', 'third_party/gflags', '28f50e0fed19872e0fd50dd23ce2ee8cd759338e')} \
    ${@trout_git_uri(d, 'GITHUB_REMOTE', 'madler/zlib', 'third_party/zlib', 'cacf7f1d4e3d44d871b605da3b647f07d718623f')} \
"

SRC_SYMLINKS = "\
   device/google/trout/agl_services_build/build.sh:build.sh \
   device/google/trout/agl_services_build/CMakeLists.txt:CMakeLists.txt \
   device/google/trout/agl_services_build/cmake:cmake \
   device/google/trout/agl_services_build/toolchain:toolchain \
   device/google/trout/hal/audiocontrol/aidl/1.0:audiocontrol \
   device/google/trout/hal/dumpstate/1.1:dumpstate_server \
   device/google/trout/hal/vehicle/2.0:vehicle_hal_server \
   device/google/trout/hal/common/agl/watchdog:watchdog_test_server \
   hardware/interfaces/automotive/vehicle/2.0/default:third_party/default_native_vehicle_hal \
"

def force_create_symlink(src, dst):
    src = os.path.relpath(src, os.path.dirname(dst))
    tmplink = '{}.templink'.format(dst)
    os.symlink(src, tmplink)
    os.rename(tmplink, dst)

python do_unpack_append() {
    os.chdir(d.getVar('S'))
    linkfile_rules = d.getVar('SRC_SYMLINKS').split()
    for src, dst in map(lambda x: x.split(':'), linkfile_rules):
        force_create_symlink(src, dst)
}
