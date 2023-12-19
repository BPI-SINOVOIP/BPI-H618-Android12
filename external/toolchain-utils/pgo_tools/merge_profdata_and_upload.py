#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Download profdata from different arches, merge them and upload to gs.

The script is used for updating the PGO profiles for LLVM. The workflow
is that the script will download profdata from different PGO builds, merge
them and then upload it to a gs location that LLVM can access.

The simplest way of using this script, is to run:
    ./merge_profdata_and_upload.py --all_latest_profiles
which will automatically grab profdata from latest PGO generate builders
for three different architectures and merge them. LLVM hash is also
detected automatically from the artifacts.

If you want to specify certain llvm hash, run it with:
    ./merge_profdata_and_upload.py --all_latest_profiles --llvm_hash LLVM_HASH
Note that hash checking will fail if the llvm hash you provided is not the
same as those in artifacts, or llvm hash in different artifacts are not the
same.

To only use profiles from buildbucket tasks for PGO generate, run it with:
    ./merge_profdata_and_upload.py -b amd64/bb_id1 -b arm/bb_id2 ...
The buildbucket id can be found using `bb ls` command after manually launched
builder finishes.

There is a chance that builders only succeeded partially, in this case, you
can run this script to merge both profdata from builder scheduled and manually
launched:
    ./merge_profdata_and_upload.py -l arm -l amd64 -b arm64/bb_id
In this example, the script will merge profdata from arm and amd64 builder, and
profdata from an arm64 buildbucket task.
"""

from __future__ import print_function

import argparse
import collections
import distutils.spawn
import json
import os
import os.path
import shutil
import subprocess
import sys
import tempfile

_LLVM_PROFDATA = '/usr/bin/llvm-profdata'
_GS_PREFIX = 'gs://'

_LLVMMetadata = collections.namedtuple('_LLVMMetadata', ['head_sha'])


def _fetch_gs_artifact(remote_name, local_name):
  """Fetch single file from remote gs location to local.

  Args:
    remote_name: full gs location to the file.
    local_name: the name of local file to be copied to.
  """
  assert remote_name.startswith(_GS_PREFIX)
  subprocess.check_call(['gsutil', 'cp', remote_name, local_name])


def _get_gs_profdata(remote_profdata, arch):
  """Fetch and extract profdata from remote gs location.

  Args:
    remote_profdata: remote gs location of the profdata tarball.
    arch: directory named with arch to saperate each profdata.

  Returns:
    Local location of the extracted profdata.
  """
  tar = 'llvm_profdata.tar.xz'
  _fetch_gs_artifact(remote_profdata, tar)
  extract_cmd = ['tar', '-xvf', tar]

  profdata_name = subprocess.check_output(extract_cmd).strip()
  # The output of the `tar` command should only contain one line of the
  # extracted profdata name.
  if b'.llvm.profdata' not in profdata_name:
    raise RuntimeError('No profdata in the tarball: %s' % remote_profdata)

  os.mkdir(arch)
  profdata_loc = os.path.join(arch, 'llvm.profdata')
  os.rename(profdata_name, profdata_loc)
  print('Profdata extracted to: %s' % profdata_loc)
  return profdata_loc


def _get_gs_metadata(remote_metadata):
  """Fetch metadata from remote gs location and read the LLVM head_sha.

  Args:
    remote_metadata: remote gs location of the metadata json file.

  Returns:
    LLVM head_sha metadata
  """
  metadata_basename = 'llvm_metadata.json'
  _fetch_gs_artifact(remote_metadata, metadata_basename)

  with open(metadata_basename) as f:
    result = json.load(f)

  return _LLVMMetadata(head_sha=result['head_sha'])


def _find_latest_artifacts(gs_url, arch):
  """Fetch the latest profdata and metadata from a give gs location.

  Args:
    gs_url: a gs location containing one or more artifacts to fetch.
    arch: the arch profdata collected from.

  Returns:
    A tuple of local profdata location and metadata
  """
  assert gs_url.startswith(_GS_PREFIX)
  try:
    # List all artifacts in the gs location and sort by time.
    output = subprocess.check_output(['gsutil', 'ls', '-l', gs_url],
                                     encoding='utf-8').strip().split('\n')
    lines = sorted(output, key=lambda x: x.split()[1], reverse=True)
  except subprocess.CalledProcessError:
    raise RuntimeError('Artifacts not found: %s' % gs_url)

  # Use a loop to go through all artifacts to find the latest profdata.
  # An example of the output of latest builder bucket:
  # pylint: disable=line-too-long
  #   5006528  2020-05-31T10:08:48Z  gs://chromeos-toolchain-artifacts/llvm-pgo/arm/llvm-11.0_pre387436_p20200403-r7-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm.profdata.tar.xz
  #   56  2020-05-31T10:08:48Z  gs://chromeos-toolchain-artifacts/llvm-pgo/arm/llvm-11.0_pre387436_p20200403-r7-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm_metadata.json
  #   5005952  2020-05-24T10:53:34Z  gs://chromeos-toolchain-artifacts/llvm-pgo/arm/llvm-11.0_pre387436_p20200403-r5-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm.profdata.tar.xz
  #   56  2020-05-24T10:53:34Z  gs://chromeos-toolchain-artifacts/llvm-pgo/arm/llvm-11.0_pre387436_p20200403-r5-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm_metadata.json
  # An example for the lines of buildbucket location:
  #   5004260  2020-05-29T09:48:04Z  gs://chromeos-image-archive/arm-pgo-generate-llvm-next-toolchain/R85-13254.0.0-1-8879010326583123168/llvm-11.0_pre387436_p20200403-r7-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm.profdata.tar.xz
  #   56  2020-05-29T09:48:04Z  gs://chromeos-image-archive/arm-pgo-generate-llvm-next-toolchain/R85-13254.0.0-1-8879010326583123168/llvm-11.0_pre387436_p20200403-r7-a8e5dcb072b1f794883ae8125fb08c06db678d56.llvm_metadata.json
  # pylint: enable=line-too-long
  profdata_url = ''
  for line in lines:
    url = line.split()[-1]
    if '.llvm.profdata.tar.xz' in url:
      profile_path = _get_gs_profdata(url, arch)
      profdata_url = url
      break
  if not profile_path or not profdata_url:
    raise RuntimeError('No profdata found from %s' % gs_url)

  metadata_url = profdata_url.replace('.llvm.profdata.tar.xz',
                                      '.llvm_metadata.json')
  metadata = _get_gs_metadata(metadata_url)
  if not metadata:
    raise RuntimeError('No metadata found from %s' % gs_url)
  return metadata, profile_path


def _fetch_from_latest(arch):
  """Fetch artifacts from latest builders.

  Args:
    arch: the arch profdata collected from.

  Returns:
    A tuple of local profdata location and metadata
  """
  print('\nFETCHING LATEST PROFDATA ON %s...' % arch.upper())
  remote_latest = (
      '%schromeos-toolchain-artifacts/llvm-pgo/%s' % (_GS_PREFIX, arch))
  return _find_latest_artifacts(remote_latest, arch)


def _fetch_from_buildbucket(arch, bb):
  """Fetch artifacts from buildbucket task.

  Args:
    arch: the arch profdata collected from.
    bb: buildbucket id.

  Returns:
    A tuple of local profdata location and metadata
  """
  print('\nFETCHING BUILDBUCKET PROFDATA ON %s...' % arch.upper())
  remote_arch = ('%schromeos-image-archive/%s-pgo-generate-llvm-next-toolchain'
                 % (_GS_PREFIX, arch))
  # List all buckets under {arch}-pgo-generate-llvm-next-toolchain and
  # grep with buildbucket id.
  remote_bb = subprocess.check_output(['gsutil', 'ls', remote_arch],
                                      encoding='utf-8').strip().split('\n')
  for line in remote_bb:
    if bb in line:
      return _find_latest_artifacts(line, arch)
  raise RuntimeError('No matched results found in %s with bb: %s' % (arch, bb))


def _merge_profdata(profdata_list, output_name):
  """Merge profdata.

  Args:
    profdata_list: list of profdata location of each arch.
    output_name: name of merged profdata.
  """
  merge_cmd = [_LLVM_PROFDATA, 'merge', '-output', output_name] + profdata_list
  print('\nMerging PGO profiles.\nCMD: %s' % merge_cmd)
  subprocess.check_call(merge_cmd)


def _tar_and_upload_profdata(profdata, name_suffix):
  """Create a tarball of merged profdata and upload to certain gs location.

  Args:
    profdata: location of merged profdata.
    name_suffix: usually the LLVM head_sha.
  """
  tarball = 'llvm-profdata-%s.tar.xz' % name_suffix
  print('Making profdata tarball: %s' % tarball)
  subprocess.check_call(
      ['tar', '--sparse', '-I', 'xz', '-cf', tarball, profdata])

  upload_location = '%schromeos-localmirror/distfiles/%s' % (_GS_PREFIX,
                                                             tarball)

  # TODO: it's better to create a subdir: distfiles/llvm_pgo_profile, but
  # now llvm could only recognize distfiles.
  upload_cmd = [
      'gsutil',
      '-m',
      'cp',
      '-n',
      '-a',
      'public-read',
      tarball,
      upload_location,
  ]
  print('\nUploading tarball to gs.\nCMD: %s\n' % upload_cmd)

  # gsutil prints all status to stderr, oddly enough.
  gs_output = subprocess.check_output(
      upload_cmd, stderr=subprocess.STDOUT, encoding='utf-8')

  # gsutil exits successfully even if it uploaded nothing. It prints a summary
  # of what all it did, though. Successful uploads are just a progress bar,
  # unsuccessful ones note that items were skipped.
  if 'Skipping existing item' in gs_output:
    raise ValueError('Profile upload failed: would overwrite an existing '
                     'profile at %s' % upload_location)


def main():
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '-a',
      '--all_latest_profiles',
      action='store_true',
      help='Merge and upload profiles from the latest builders.')
  parser.add_argument(
      '-l',
      '--latest',
      default=[],
      action='append',
      help='User can specify the profdata from which builder with specific '
      'architecture to download. By default, we merge profdata from arm, '
      'arm64, amd64.')
  parser.add_argument(
      '-b',
      '--buildbucket',
      default=[],
      action='append',
      help='Extra pgo-generate-llvm-next-toolchain buildbucket results to be '
      'used. Format should be: {arch}/{bb_id}.')
  parser.add_argument(
      '-o',
      '--output',
      default='llvm.profdata',
      help='Where to put merged PGO profile. The default is to not save it '
      'anywhere.')
  parser.add_argument(
      '--llvm_hash',
      help='The LLVM hash to select for the profiles. Generally autodetected.')
  args = parser.parse_args()

  if not args.all_latest_profiles and not (args.latest or args.buildbucket):
    parser.error('Please specify whether to use latest profiles or '
                 'profiles from buildbucket')

  if args.all_latest_profiles and (args.latest or args.buildbucket):
    parser.error('--all_latest_profiles cannot be specified together '
                 'with --latest or --buildbucket')

  latest = ['arm', 'arm64', 'amd64'] \
    if args.all_latest_profiles else args.latest

  all_arch_list = latest.copy()
  arch_bb_list = []
  if args.buildbucket:
    for arch_bb in args.buildbucket:
      arch, bb = arch_bb.split('/')
      arch_bb_list.append((arch, bb))
      all_arch_list.append(arch)

  if len(set(all_arch_list)) != len(all_arch_list):
    parser.error('Each arch can be only passed once.')

  if not distutils.spawn.find_executable(_LLVM_PROFDATA):
    sys.exit(_LLVM_PROFDATA + ' not found; are you in the chroot?')

  initial_dir = os.getcwd()
  temp_dir = tempfile.mkdtemp(prefix='merge_pgo')
  success = True
  try:
    os.chdir(temp_dir)
    profdata_list = []
    heads = set()

    def append_artifacts(fetched_tuple):
      llvm_metadata, profdata_loc = fetched_tuple
      if os.path.getsize(profdata_loc) < 512 * 1024:
        raise RuntimeError('The PGO profile in local path %s is suspiciously '
                           'small. Something might have gone '
                           'wrong.' % profdata_loc)
      heads.add(llvm_metadata.head_sha)
      profdata_list.append(profdata_loc)

    for arch in latest:
      append_artifacts(_fetch_from_latest(arch))

    for arch, bb in arch_bb_list:
      append_artifacts(_fetch_from_buildbucket(arch, bb))

    assert heads, "Didn't fetch anything?"

    def die_with_head_complaint(complaint):
      extra = ' (HEADs found: %s)' % sorted(heads)
      raise RuntimeError(complaint.rstrip() + extra)

    llvm_hash = args.llvm_hash
    if not llvm_hash:
      if len(heads) != 1:
        die_with_head_complaint(
            '%d LLVM HEADs were found, which is more than one. You probably '
            'want a consistent set of HEADs for a profile. If you know you '
            "don't, please specify --llvm_hash, and note that *all* profiles "
            'will be merged into this final profile, regardless of their '
            'reported HEAD.' % len(heads))
      llvm_hash, = heads

    if llvm_hash not in heads:
      assert llvm_hash == args.llvm_hash
      die_with_head_complaint(
          "HEAD %s wasn't found in any fetched artifacts." % llvm_hash)

    print('\nUsing LLVM hash: %s' % llvm_hash)

    _merge_profdata(profdata_list, args.output)
    print('Merged profdata locates at %s' % os.path.abspath(args.output))
    _tar_and_upload_profdata(args.output, name_suffix=llvm_hash)
    print('\nMerged profdata uploaded successfully.')
  except:
    success = False
    raise
  finally:
    os.chdir(initial_dir)
    if success:
      print('Clearing temp directory.')
      shutil.rmtree(temp_dir, ignore_errors=True)
    else:
      print('Script fails, temp directory is at: %s' % temp_dir)


if __name__ == '__main__':
  sys.exit(main())
