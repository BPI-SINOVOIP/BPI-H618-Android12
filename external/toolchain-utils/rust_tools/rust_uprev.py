#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to automatically generate a new Rust uprev CL.

This tool is intended to automatically generate a CL to uprev Rust to a
newer version in Chrome OS, including creating a new Rust version or
removing an old version. It's based on
src/third_party/chromiumos-overlay/dev-lang/rust/UPGRADE.md. When using
the tool, the progress can be saved to a JSON file, so the user can resume
the process after a failing step is fixed. Example usage to create a new
version:

1. (inside chroot) $ ./rust_tools/rust_uprev.py
                     --state_file /tmp/state-file.json
                     create --rust_version 1.45.0
2. Step "compile rust" failed due to the patches can't apply to new version
3. Manually fix the patches
4. Execute the command in step 1 again.
5. Iterate 1-4 for each failed step until the tool passes.

Replace `create --rust_version 1.45.0` with `remove --rust_version 1.43.0`
if you want to remove all 1.43.0 related stuff in the same CL. Remember to
use a different state file if you choose to run different subcommands.

If you want a hammer that can do everything for you, use the subcommand
`roll`. It can create a Rust uprev CL with `create` and `remove` and upload
the CL to chromium code review.

See `--help` for all available options.
"""

# pylint: disable=cros-logging-import

import argparse
import pathlib
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Any, Callable, Dict, List, NamedTuple, Optional, T, Tuple

from llvm_tools import chroot, git
RUST_PATH = '/mnt/host/source/src/third_party/chromiumos-overlay/dev-lang/rust'


def get_command_output(command: List[str], *args, **kwargs) -> str:
  return subprocess.check_output(
      command, encoding='utf-8', *args, **kwargs).strip()


class RustVersion(NamedTuple):
  """NamedTuple represents a Rust version"""
  major: int
  minor: int
  patch: int

  def __str__(self):
    return f'{self.major}.{self.minor}.{self.patch}'

  @staticmethod
  def parse_from_ebuild(ebuild_name: str) -> 'RustVersion':
    input_re = re.compile(r'^rust-'
                          r'(?P<major>\d+)\.'
                          r'(?P<minor>\d+)\.'
                          r'(?P<patch>\d+)'
                          r'(:?-r\d+)?'
                          r'\.ebuild$')
    m = input_re.match(ebuild_name)
    assert m, f'failed to parse {ebuild_name!r}'
    return RustVersion(
        int(m.group('major')), int(m.group('minor')), int(m.group('patch')))

  @staticmethod
  def parse(x: str) -> 'RustVersion':
    input_re = re.compile(r'^(?:rust-)?'
                          r'(?P<major>\d+)\.'
                          r'(?P<minor>\d+)\.'
                          r'(?P<patch>\d+)'
                          r'(?:.ebuild)?$')
    m = input_re.match(x)
    assert m, f'failed to parse {x!r}'
    return RustVersion(
        int(m.group('major')), int(m.group('minor')), int(m.group('patch')))


def parse_commandline_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
      description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  parser.add_argument(
      '--state_file',
      required=True,
      help='A state file to hold previous completed steps. If the file '
      'exists, it needs to be used together with --continue or --restart. '
      'If not exist (do not use --continue in this case), we will create a '
      'file for you.',
  )
  parser.add_argument(
      '--restart',
      action='store_true',
      help='Restart from the first step. Ignore the completed steps in '
      'the state file',
  )
  parser.add_argument(
      '--continue',
      dest='cont',
      action='store_true',
      help='Continue the steps from the state file',
  )

  create_parser_template = argparse.ArgumentParser(add_help=False)
  create_parser_template.add_argument(
      '--template',
      type=RustVersion.parse,
      default=None,
      help='A template to use for creating a Rust uprev from, in the form '
      'a.b.c The ebuild has to exist in the chroot. If not specified, the '
      'tool will use the current Rust version in the chroot as template.',
  )
  create_parser_template.add_argument(
      '--skip_compile',
      action='store_true',
      help='Skip compiling rust to test the tool. Only for testing',
  )

  subparsers = parser.add_subparsers(dest='subparser_name')
  subparser_names = []
  subparser_names.append('create')
  create_parser = subparsers.add_parser(
      'create',
      parents=[create_parser_template],
      help='Create changes uprevs Rust to a new version',
  )
  create_parser.add_argument(
      '--rust_version',
      type=RustVersion.parse,
      required=True,
      help='Rust version to uprev to, in the form a.b.c',
  )

  subparser_names.append('remove')
  remove_parser = subparsers.add_parser(
      'remove',
      help='Clean up old Rust version from chroot',
  )
  remove_parser.add_argument(
      '--rust_version',
      type=RustVersion.parse,
      default=None,
      help='Rust version to remove, in the form a.b.c If not '
      'specified, the tool will remove the oldest version in the chroot',
  )

  subparser_names.append('roll')
  roll_parser = subparsers.add_parser(
      'roll',
      parents=[create_parser_template],
      help='A command can create and upload a Rust uprev CL, including '
      'preparing the repo, creating new Rust uprev, deleting old uprev, '
      'and upload a CL to crrev.',
  )
  roll_parser.add_argument(
      '--uprev',
      type=RustVersion.parse,
      required=True,
      help='Rust version to uprev to, in the form a.b.c',
  )
  roll_parser.add_argument(
      '--remove',
      type=RustVersion.parse,
      default=None,
      help='Rust version to remove, in the form a.b.c If not '
      'specified, the tool will remove the oldest version in the chroot',
  )
  roll_parser.add_argument(
      '--skip_cross_compiler',
      action='store_true',
      help='Skip updating cross-compiler in the chroot',
  )
  roll_parser.add_argument(
      '--no_upload',
      action='store_true',
      help='If specified, the tool will not upload the CL for review',
  )

  args = parser.parse_args()
  if args.subparser_name not in subparser_names:
    parser.error('one of %s must be specified' % subparser_names)

  if args.cont and args.restart:
    parser.error('Please select either --continue or --restart')

  if os.path.exists(args.state_file):
    if not args.cont and not args.restart:
      parser.error('State file exists, so you should either --continue '
                   'or --restart')
  if args.cont and not os.path.exists(args.state_file):
    parser.error('Indicate --continue but the state file does not exist')

  if args.restart and os.path.exists(args.state_file):
    os.remove(args.state_file)

  return args


def parse_stage0_file(new_version: RustVersion) -> Tuple[str, str, str]:
  # Find stage0 date, rustc and cargo
  stage0_file = get_command_output([
      'curl', '-f', 'https://raw.githubusercontent.com/rust-lang/rust/'
      f'{new_version}/src/stage0.txt'
  ])
  regexp = re.compile(r'date:\s*(?P<date>\d+-\d+-\d+)\s+'
                      r'rustc:\s*(?P<rustc>\d+\.\d+\.\d+)\s+'
                      r'cargo:\s*(?P<cargo>\d+\.\d+\.\d+)')
  m = regexp.search(stage0_file)
  assert m, 'failed to parse stage0.txt file'
  stage0_date, stage0_rustc, stage0_cargo = m.groups()
  logging.info('Found stage0 file has date: %s, rustc: %s, cargo: %s',
               stage0_date, stage0_rustc, stage0_cargo)
  return stage0_date, stage0_rustc, stage0_cargo


def prepare_uprev(rust_version: RustVersion, template: Optional[RustVersion]
                 ) -> Optional[Tuple[RustVersion, str]]:
  if template is None:
    ebuild_path = get_command_output(['equery', 'w', 'rust'])
    ebuild_name = os.path.basename(ebuild_path)
    template_version = RustVersion.parse_from_ebuild(ebuild_name)
  else:
    ebuild_path = find_ebuild_for_rust_version(template)
    template_version = template

  if rust_version <= template_version:
    logging.info(
        'Requested version %s is not newer than the template version %s.',
        rust_version, template_version)
    return None

  logging.info('Template Rust version is %s (ebuild: %r)', template_version,
               ebuild_path)
  return template_version, ebuild_path


def copy_patches(template_version: RustVersion,
                 new_version: RustVersion) -> None:
  patch_path = os.path.join(RUST_PATH, 'files')
  for f in os.listdir(patch_path):
    if f'rust-{template_version}' not in f:
      continue
    logging.info('Rename patch %s to new version', f)
    new_name = f.replace(str(template_version), str(new_version))
    shutil.copyfile(
        os.path.join(patch_path, f),
        os.path.join(patch_path, new_name),
    )

  subprocess.check_call(['git', 'add', f'files/rust-{new_version}-*.patch'],
                        cwd=RUST_PATH)


def create_ebuild(template_ebuild: str, new_version: RustVersion) -> str:
  shutil.copyfile(template_ebuild,
                  os.path.join(RUST_PATH, f'rust-{new_version}.ebuild'))
  subprocess.check_call(['git', 'add', f'rust-{new_version}.ebuild'],
                        cwd=RUST_PATH)
  return os.path.join(RUST_PATH, f'rust-{new_version}.ebuild')


def update_ebuild(ebuild_file: str, stage0_info: Tuple[str, str, str]) -> None:
  stage0_date, stage0_rustc, stage0_cargo = stage0_info
  with open(ebuild_file, encoding='utf-8') as f:
    contents = f.read()
  # Update STAGE0_DATE in the ebuild
  stage0_date_re = re.compile(r'STAGE0_DATE="(\d+-\d+-\d+)"')
  if not stage0_date_re.search(contents):
    raise RuntimeError('STAGE0_DATE not found in rust ebuild')
  new_contents = stage0_date_re.sub(f'STAGE0_DATE="{stage0_date}"', contents)

  # Update STAGE0_VERSION in the ebuild
  stage0_rustc_re = re.compile(r'STAGE0_VERSION="[^"]*"')
  if not stage0_rustc_re.search(new_contents):
    raise RuntimeError('STAGE0_VERSION not found in rust ebuild')
  new_contents = stage0_rustc_re.sub(f'STAGE0_VERSION="{stage0_rustc}"',
                                     new_contents)

  # Update STAGE0_VERSION_CARGO in the ebuild
  stage0_cargo_re = re.compile(r'STAGE0_VERSION_CARGO="[^"]*"')
  if not stage0_cargo_re.search(new_contents):
    raise RuntimeError('STAGE0_VERSION_CARGO not found in rust ebuild')
  new_contents = stage0_cargo_re.sub(f'STAGE0_VERSION_CARGO="{stage0_cargo}"',
                                     new_contents)
  with open(ebuild_file, 'w', encoding='utf-8') as f:
    f.write(new_contents)
  logging.info(
      'Rust ebuild file has STAGE0_DATE, STAGE0_VERSION, STAGE0_VERSION_CARGO '
      'updated to %s, %s, %s respectively', stage0_date, stage0_rustc,
      stage0_cargo)


def flip_mirror_in_ebuild(ebuild_file: str, add: bool) -> None:
  restrict_re = re.compile(
      r'(?P<before>RESTRICT=")(?P<values>"[^"]*"|.*)(?P<after>")')
  with open(ebuild_file, encoding='utf-8') as f:
    contents = f.read()
  m = restrict_re.search(contents)
  assert m, 'failed to find RESTRICT variable in Rust ebuild'
  values = m.group('values')
  if add:
    if 'mirror' in values:
      return
    values += ' mirror'
  else:
    if 'mirror' not in values:
      return
    values = values.replace(' mirror', '')
  new_contents = restrict_re.sub(r'\g<before>%s\g<after>' % values, contents)
  with open(ebuild_file, 'w', encoding='utf-8') as f:
    f.write(new_contents)


def rust_ebuild_actions(actions: List[str], sudo: bool = False) -> None:
  ebuild_path_inchroot = get_command_output(['equery', 'w', 'rust'])
  cmd = ['ebuild', ebuild_path_inchroot] + actions
  if sudo:
    cmd = ['sudo'] + cmd
  subprocess.check_call(cmd)


def update_manifest(ebuild_file: str) -> None:
  logging.info('Added "mirror" to RESTRICT to Rust ebuild')
  flip_mirror_in_ebuild(ebuild_file, add=True)
  rust_ebuild_actions(['manifest'])
  logging.info('Removed "mirror" to RESTRICT from Rust ebuild')
  flip_mirror_in_ebuild(ebuild_file, add=False)


def update_rust_packages(rust_version: RustVersion, add: bool) -> None:
  package_file = os.path.join(
      RUST_PATH, '../../profiles/targets/chromeos/package.provided')
  with open(package_file, encoding='utf-8') as f:
    contents = f.read()
  if add:
    rust_packages_re = re.compile(r'dev-lang/rust-(\d+\.\d+\.\d+)')
    rust_packages = rust_packages_re.findall(contents)
    # Assume all the rust packages are in alphabetical order, so insert the new
    # version to the place after the last rust_packages
    new_str = f'dev-lang/rust-{rust_version}'
    new_contents = contents.replace(rust_packages[-1],
                                    f'{rust_packages[-1]}\n{new_str}')
    logging.info('%s has been inserted into package.provided', new_str)
  else:
    old_str = f'dev-lang/rust-{rust_version}\n'
    assert old_str in contents, f'{old_str!r} not found in package.provided'
    new_contents = contents.replace(old_str, '')
    logging.info('%s has been removed from package.provided', old_str)

  with open(package_file, 'w', encoding='utf-8') as f:
    f.write(new_contents)


def update_virtual_rust(template_version: RustVersion,
                        new_version: RustVersion) -> None:
  virtual_rust_dir = os.path.join(RUST_PATH, '../../virtual/rust')
  assert os.path.exists(virtual_rust_dir)
  shutil.copyfile(
      os.path.join(virtual_rust_dir, f'rust-{template_version}.ebuild'),
      os.path.join(virtual_rust_dir, f'rust-{new_version}.ebuild'))
  subprocess.check_call(['git', 'add', f'rust-{new_version}.ebuild'],
                        cwd=virtual_rust_dir)


def upload_single_tarball(rust_url: str, tarfile_name: str,
                          tempdir: str) -> None:
  rust_src = f'{rust_url}/{tarfile_name}'
  gsutil_location = f'gs://chromeos-localmirror/distfiles/{tarfile_name}'

  missing_file = subprocess.call(
      ['gsutil', 'ls', gsutil_location],
      stdout=subprocess.DEVNULL,
      stderr=subprocess.DEVNULL,
  )
  if not missing_file:
    logging.info('Rust artifact at %s already exists; skipping download',
                 gsutil_location)
    return

  logging.info('Downloading Rust artifact from %s', rust_src)

  # Download Rust's source
  rust_file = os.path.join(tempdir, tarfile_name)
  subprocess.check_call(['curl', '-f', '-o', rust_file, rust_src])

  # Verify the signature of the source
  sig_file = os.path.join(tempdir, 'rustc_sig.asc')
  subprocess.check_call(['curl', '-f', '-o', sig_file, f'{rust_src}.asc'])
  try:
    subprocess.check_output(['gpg', '--verify', sig_file, rust_file],
                            encoding='utf-8',
                            stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError as e:
    if "gpg: Can't check signature" not in e.output:
      raise RuntimeError(f'Failed to execute `gpg --verify`, {e.output}')

    # If it fails to verify the signature, try import rustc key, and retry.
    keys = get_command_output(
        ['curl', '-f', 'https://keybase.io/rust/pgp_keys.asc'])
    subprocess.run(['gpg', '--import'],
                   input=keys,
                   encoding='utf-8',
                   check=True)
    subprocess.check_call(['gpg', '--verify', sig_file, rust_file])

  # Since we are using `-n` to skip an item if it already exists, there's no
  # need to check if the file exists on GS bucket or not.
  subprocess.check_call(
      ['gsutil', 'cp', '-n', '-a', 'public-read', rust_file, gsutil_location])


def upload_to_localmirror(tempdir: str, rust_version: RustVersion,
                          stage0_info: Tuple[str, str, str]) -> None:
  stage0_date, stage0_rustc, stage0_cargo = stage0_info
  rust_url = 'https://static.rust-lang.org/dist'
  # Upload rustc source
  upload_single_tarball(
      rust_url,
      f'rustc-{rust_version}-src.tar.gz',
      tempdir,
  )
  # Upload stage0 toolchain
  upload_single_tarball(
      f'{rust_url}/{stage0_date}',
      f'rust-std-{stage0_rustc}-x86_64-unknown-linux-gnu.tar.gz',
      tempdir,
  )
  # Upload stage0 source
  upload_single_tarball(
      rust_url,
      f'rustc-{stage0_rustc}-x86_64-unknown-linux-gnu.tar.gz',
      tempdir,
  )
  # Upload stage0 cargo
  upload_single_tarball(
      rust_url,
      f'cargo-{stage0_cargo}-x86_64-unknown-linux-gnu.tar.gz',
      tempdir,
  )


def perform_step(state_file: pathlib.Path,
                 tmp_state_file: pathlib.Path,
                 completed_steps: Dict[str, Any],
                 step_name: str,
                 step_fn: Callable[[], T],
                 result_from_json: Optional[Callable[[Any], T]] = None,
                 result_to_json: Optional[Callable[[T], Any]] = None) -> T:
  if step_name in completed_steps:
    logging.info('Skipping previously completed step %s', step_name)
    if result_from_json:
      return result_from_json(completed_steps[step_name])
    return completed_steps[step_name]

  logging.info('Running step %s', step_name)
  val = step_fn()
  logging.info('Step %s complete', step_name)
  if result_to_json:
    completed_steps[step_name] = result_to_json(val)
  else:
    completed_steps[step_name] = val

  with tmp_state_file.open('w', encoding='utf-8') as f:
    json.dump(completed_steps, f, indent=4)
  tmp_state_file.rename(state_file)
  return val


def prepare_uprev_from_json(obj: Any) -> Optional[Tuple[RustVersion, str]]:
  if not obj:
    return None
  version, ebuild_path = obj
  return RustVersion(*version), ebuild_path


def create_rust_uprev(rust_version: RustVersion,
                      maybe_template_version: Optional[RustVersion],
                      skip_compile: bool, run_step: Callable[[], T]) -> None:
  stage0_info = run_step(
      'parse stage0 file', lambda: parse_stage0_file(rust_version))
  template_version, template_ebuild = run_step(
      'prepare uprev',
      lambda: prepare_uprev(rust_version, maybe_template_version),
      result_from_json=prepare_uprev_from_json,
  )
  if template_ebuild is None:
    return

  run_step('copy patches', lambda: copy_patches(template_version, rust_version))
  ebuild_file = run_step(
      'create ebuild', lambda: create_ebuild(template_ebuild, rust_version))
  run_step('update ebuild', lambda: update_ebuild(ebuild_file, stage0_info))
  with tempfile.TemporaryDirectory(dir='/tmp') as tempdir:
    run_step('upload_to_localmirror', lambda: upload_to_localmirror(
        tempdir, rust_version, stage0_info))
  run_step('update manifest to add new version', lambda: update_manifest(
      ebuild_file))
  if not skip_compile:
    run_step('emerge rust', lambda: subprocess.check_call(
        ['sudo', 'emerge', 'dev-lang/rust']))
  run_step('insert version into rust packages', lambda: update_rust_packages(
      rust_version, add=True))
  run_step('upgrade virtual/rust', lambda: update_virtual_rust(
      template_version, rust_version))


def find_rust_versions_in_chroot() -> List[Tuple[RustVersion, str]]:
  return [(RustVersion.parse_from_ebuild(x), os.path.join(RUST_PATH, x))
          for x in os.listdir(RUST_PATH)
          if x.endswith('.ebuild')]


def find_oldest_rust_version_in_chroot() -> Tuple[RustVersion, str]:
  rust_versions = find_rust_versions_in_chroot()
  if len(rust_versions) <= 1:
    raise RuntimeError('Expect to find more than one Rust versions')
  return min(rust_versions)


def find_ebuild_for_rust_version(version: RustVersion) -> str:
  rust_ebuilds = [
      ebuild for x, ebuild in find_rust_versions_in_chroot() if x == version
  ]
  if not rust_ebuilds:
    raise ValueError(f'No Rust ebuilds found matching {version}')
  if len(rust_ebuilds) > 1:
    raise ValueError(f'Multiple Rust ebuilds found matching {version}: '
                     f'{rust_ebuilds}')
  return rust_ebuilds[0]


def remove_files(filename: str, path: str) -> None:
  subprocess.check_call(['git', 'rm', filename], cwd=path)


def remove_rust_uprev(rust_version: Optional[RustVersion],
                      run_step: Callable[[], T]) -> None:

  def find_desired_rust_version():
    if rust_version:
      return rust_version, find_ebuild_for_rust_version(rust_version)
    return find_oldest_rust_version_in_chroot()

  delete_version, delete_ebuild = run_step(
      'find rust version to delete',
      find_desired_rust_version,
      result_from_json=prepare_uprev_from_json,
  )
  run_step(
      'remove patches', lambda: remove_files(
          f'files/rust-{delete_version}-*.patch', RUST_PATH))
  run_step('remove ebuild', lambda: remove_files(delete_ebuild, RUST_PATH))
  ebuild_file = get_command_output(['equery', 'w', 'rust'])
  run_step('update manifest to delete old version', lambda: update_manifest(
      ebuild_file))
  run_step('remove version from rust packages', lambda: update_rust_packages(
      delete_version, add=False))
  run_step(
      'remove virtual/rust', lambda: remove_files(
          f'rust-{delete_version}.ebuild',
          os.path.join(RUST_PATH, '../../virtual/rust')))


def create_new_repo(rust_version: RustVersion) -> None:
  output = get_command_output(['git', 'status', '--porcelain'], cwd=RUST_PATH)
  if output:
    raise RuntimeError(
        f'{RUST_PATH} has uncommitted changes, please either discard them '
        'or commit them.')
  git.CreateBranch(RUST_PATH, f'rust-to-{rust_version}')


def build_cross_compiler() -> None:
  # Get target triples in ebuild
  rust_ebuild = get_command_output(['equery', 'w', 'rust'])
  with open(rust_ebuild, encoding='utf-8') as f:
    contents = f.read()

  target_triples_re = re.compile(r'RUSTC_TARGET_TRIPLES=\(([^)]+)\)')
  m = target_triples_re.search(contents)
  assert m, 'RUST_TARGET_TRIPLES not found in rust ebuild'
  target_triples = m.group(1).strip().split('\n')

  compiler_targets_to_install = [
      target.strip() for target in target_triples if 'cros-' in target
  ]
  for target in target_triples:
    if 'cros-' not in target:
      continue
    target = target.strip()

  # We also always need arm-none-eabi, though it's not mentioned in
  # RUSTC_TARGET_TRIPLES.
  compiler_targets_to_install.append('arm-none-eabi')

  logging.info('Emerging cross compilers %s', compiler_targets_to_install)
  subprocess.check_call(
      ['sudo', 'emerge', '-j', '-G'] +
      [f'cross-{target}/gcc' for target in compiler_targets_to_install])


def create_new_commit(rust_version: RustVersion) -> None:
  subprocess.check_call(['git', 'add', '-A'], cwd=RUST_PATH)
  messages = [
      f'[DO NOT SUBMIT] dev-lang/rust: upgrade to Rust {rust_version}',
      '',
      'This CL is created by rust_uprev tool automatically.'
      '',
      'BUG=None',
      'TEST=Use CQ to test the new Rust version',
  ]
  git.UploadChanges(RUST_PATH, f'rust-to-{rust_version}', messages)


def main() -> None:
  if not chroot.InChroot():
    raise RuntimeError('This script must be executed inside chroot')

  logging.basicConfig(level=logging.INFO)

  args = parse_commandline_args()

  state_file = pathlib.Path(args.state_file)
  tmp_state_file = state_file.with_suffix('.tmp')

  try:
    with state_file.open(encoding='utf-8') as f:
      completed_steps = json.load(f)
  except FileNotFoundError:
    completed_steps = {}

  def run_step(
      step_name: str,
      step_fn: Callable[[], T],
      result_from_json: Optional[Callable[[Any], T]] = None,
      result_to_json: Optional[Callable[[T], Any]] = None,
  ) -> T:
    return perform_step(state_file, tmp_state_file, completed_steps, step_name,
                        step_fn, result_from_json, result_to_json)

  if args.subparser_name == 'create':
    create_rust_uprev(args.rust_version, args.template, args.skip_compile,
                      run_step)
  elif args.subparser_name == 'remove':
    remove_rust_uprev(args.rust_version, run_step)
  else:
    # If you have added more subparser_name, please also add the handlers above
    assert args.subparser_name == 'roll'
    run_step('create new repo', lambda: create_new_repo(args.uprev))
    if not args.skip_cross_compiler:
      run_step('build cross compiler', build_cross_compiler)
    create_rust_uprev(args.uprev, args.template, args.skip_compile, run_step)
    remove_rust_uprev(args.remove, run_step)
    if not args.no_upload:
      run_step('create rust uprev CL', lambda: create_new_commit(args.uprev))


if __name__ == '__main__':
  sys.exit(main())
