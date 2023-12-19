#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for rust_uprev.py"""

# pylint: disable=cros-logging-import
import os
import shutil
import subprocess
import unittest
from unittest import mock

from llvm_tools import git

import rust_uprev


class RustVersionTest(unittest.TestCase):
  """Tests for RustVersion class"""

  def test_str(self):
    obj = rust_uprev.RustVersion(major=1, minor=2, patch=3)
    self.assertEqual(str(obj), '1.2.3')

  def test_parse_version_only(self):
    expected = rust_uprev.RustVersion(major=1, minor=2, patch=3)
    actual = rust_uprev.RustVersion.parse('1.2.3')
    self.assertEqual(expected, actual)

  def test_parse_ebuild_name(self):
    expected = rust_uprev.RustVersion(major=1, minor=2, patch=3)
    actual = rust_uprev.RustVersion.parse_from_ebuild('rust-1.2.3.ebuild')
    self.assertEqual(expected, actual)

    actual = rust_uprev.RustVersion.parse_from_ebuild('rust-1.2.3-r1.ebuild')
    self.assertEqual(expected, actual)

  def test_parse_fail(self):
    with self.assertRaises(AssertionError) as context:
      rust_uprev.RustVersion.parse('invalid-rust-1.2.3')
    self.assertEqual("failed to parse 'invalid-rust-1.2.3'",
                     str(context.exception))


class PrepareUprevTest(unittest.TestCase):
  """Tests for prepare_uprev step in rust_uprev"""

  def setUp(self):
    self.version_old = rust_uprev.RustVersion(1, 2, 3)
    self.version_new = rust_uprev.RustVersion(1, 3, 5)

  @mock.patch.object(
      rust_uprev,
      'find_ebuild_for_rust_version',
      return_value='/path/to/ebuild')
  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_success_with_template(self, mock_command, mock_find_ebuild):
    expected = (self.version_old, '/path/to/ebuild')
    actual = rust_uprev.prepare_uprev(
        rust_version=self.version_new, template=self.version_old)
    self.assertEqual(expected, actual)
    mock_command.assert_not_called()

  @mock.patch.object(
      rust_uprev,
      'find_ebuild_for_rust_version',
      return_value='/path/to/ebuild')
  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_return_none_with_template_larger_than_input(self, mock_command,
                                                       _mock_find_ebuild):
    ret = rust_uprev.prepare_uprev(
        rust_version=self.version_old, template=self.version_new)
    self.assertIsNone(ret)
    mock_command.assert_not_called()

  @mock.patch.object(os.path, 'exists')
  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_success_without_template(self, mock_command, mock_exists):
    rust_ebuild_path = f'/path/to/rust/rust-{self.version_old}-r3.ebuild'
    mock_command.return_value = rust_ebuild_path
    expected = (self.version_old, rust_ebuild_path)
    actual = rust_uprev.prepare_uprev(
        rust_version=self.version_new, template=None)
    self.assertEqual(expected, actual)
    mock_command.assert_called_once_with(['equery', 'w', 'rust'])
    mock_exists.assert_not_called()

  @mock.patch.object(os.path, 'exists')
  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_return_none_with_ebuild_larger_than_input(self, mock_command,
                                                     mock_exists):
    mock_command.return_value = f'/path/to/rust/rust-{self.version_new}.ebuild'
    ret = rust_uprev.prepare_uprev(rust_version=self.version_old, template=None)
    self.assertIsNone(ret)
    mock_exists.assert_not_called()

  def test_prepare_uprev_from_json(self):
    ebuild_path = '/path/to/the/ebuild'
    json_result = (list(self.version_new), ebuild_path)
    expected = (self.version_new, ebuild_path)
    actual = rust_uprev.prepare_uprev_from_json(json_result)
    self.assertEqual(expected, actual)


class UpdateEbuildTest(unittest.TestCase):
  """Tests for update_ebuild step in rust_uprev"""
  ebuild_file_before = """
    STAGE0_DATE="2019-01-01"
    STAGE0_VERSION="any.random.(number)"
    STAGE0_VERSION_CARGO="0.0.0"
    """
  ebuild_file_after = """
    STAGE0_DATE="2020-01-01"
    STAGE0_VERSION="1.1.1"
    STAGE0_VERSION_CARGO="0.1.0"
    """

  def test_success(self):
    mock_open = mock.mock_open(read_data=self.ebuild_file_before)
    ebuild_file = '/path/to/rust/rust-1.3.5.ebuild'
    with mock.patch('builtins.open', mock_open):
      rust_uprev.update_ebuild(ebuild_file, ('2020-01-01', '1.1.1', '0.1.0'))
    mock_open.return_value.__enter__().write.assert_called_once_with(
        self.ebuild_file_after)

  def test_fail_when_ebuild_misses_a_variable(self):
    ebuild_file = 'STAGE0_DATE="2019-01-01"'
    mock_open = mock.mock_open(read_data=ebuild_file)
    ebuild_file = '/path/to/rust/rust-1.3.5.ebuild'
    with mock.patch('builtins.open', mock_open):
      with self.assertRaises(RuntimeError) as context:
        rust_uprev.update_ebuild(ebuild_file, ('2020-01-01', '1.1.1', '0.1.0'))
    self.assertEqual('STAGE0_VERSION not found in rust ebuild',
                     str(context.exception))


class UpdateManifestTest(unittest.TestCase):
  """Tests for update_manifest step in rust_uprev"""

  # pylint: disable=protected-access
  def _run_test_flip_mirror(self, before, after, add, expect_write):
    mock_open = mock.mock_open(read_data=f'RESTRICT="{before}"')
    with mock.patch('builtins.open', mock_open):
      rust_uprev.flip_mirror_in_ebuild('', add=add)
    if expect_write:
      mock_open.return_value.__enter__().write.assert_called_once_with(
          f'RESTRICT="{after}"')

  def test_add_mirror_in_ebuild(self):
    self._run_test_flip_mirror(
        before='variable1 variable2',
        after='variable1 variable2 mirror',
        add=True,
        expect_write=True)

  def test_remove_mirror_in_ebuild(self):
    self._run_test_flip_mirror(
        before='variable1 variable2 mirror',
        after='variable1 variable2',
        add=False,
        expect_write=True)

  def test_add_mirror_when_exists(self):
    self._run_test_flip_mirror(
        before='variable1 variable2 mirror',
        after='variable1 variable2 mirror',
        add=True,
        expect_write=False)

  def test_remove_mirror_when_not_exists(self):
    self._run_test_flip_mirror(
        before='variable1 variable2',
        after='variable1 variable2',
        add=False,
        expect_write=False)

  @mock.patch.object(rust_uprev, 'flip_mirror_in_ebuild')
  @mock.patch.object(rust_uprev, 'rust_ebuild_actions')
  def test_update_manifest(self, mock_run, mock_flip):
    ebuild_file = '/path/to/rust/rust-1.1.1.ebuild'
    rust_uprev.update_manifest(ebuild_file)
    mock_run.assert_called_once_with(['manifest'])
    mock_flip.assert_has_calls(
        [mock.call(ebuild_file, add=True),
         mock.call(ebuild_file, add=False)])


class UpdateRustPackagesTests(unittest.TestCase):
  """Tests for update_rust_packages step."""

  def setUp(self):
    self.old_version = rust_uprev.RustVersion(1, 1, 0)
    self.current_version = rust_uprev.RustVersion(1, 2, 3)
    self.new_version = rust_uprev.RustVersion(1, 3, 5)
    self.ebuild_file = os.path.join(rust_uprev.RUST_PATH,
                                    'rust-{self.new_version}.ebuild')

  def test_add_new_rust_packages(self):
    package_before = (f'dev-lang/rust-{self.old_version}\n'
                      f'dev-lang/rust-{self.current_version}')
    package_after = (f'dev-lang/rust-{self.old_version}\n'
                     f'dev-lang/rust-{self.current_version}\n'
                     f'dev-lang/rust-{self.new_version}')
    mock_open = mock.mock_open(read_data=package_before)
    with mock.patch('builtins.open', mock_open):
      rust_uprev.update_rust_packages(self.new_version, add=True)
    mock_open.return_value.__enter__().write.assert_called_once_with(
        package_after)

  def test_remove_old_rust_packages(self):
    package_before = (f'dev-lang/rust-{self.old_version}\n'
                      f'dev-lang/rust-{self.current_version}\n'
                      f'dev-lang/rust-{self.new_version}')
    package_after = (f'dev-lang/rust-{self.current_version}\n'
                     f'dev-lang/rust-{self.new_version}')
    mock_open = mock.mock_open(read_data=package_before)
    with mock.patch('builtins.open', mock_open):
      rust_uprev.update_rust_packages(self.old_version, add=False)
    mock_open.return_value.__enter__().write.assert_called_once_with(
        package_after)


class UploadToLocalmirrorTests(unittest.TestCase):
  """Tests for upload_to_localmirror"""

  def setUp(self):
    self.tempdir = '/tmp/any/dir'
    self.new_version = rust_uprev.RustVersion(1, 3, 5)
    self.rust_url = 'https://static.rust-lang.org/dist'
    self.tarfile_name = f'rustc-{self.new_version}-src.tar.gz'
    self.rust_src = f'https://static.rust-lang.org/dist/{self.tarfile_name}'
    self.gsurl = f'gs://chromeos-localmirror/distfiles/{self.tarfile_name}'
    self.rust_file = os.path.join(self.tempdir, self.tarfile_name)
    self.sig_file = os.path.join(self.tempdir, 'rustc_sig.asc')

  @mock.patch.object(subprocess, 'call', return_value=1)
  @mock.patch.object(subprocess, 'check_call')
  @mock.patch.object(subprocess, 'check_output')
  @mock.patch.object(subprocess, 'run')
  def test_pass_without_retry(self, mock_run, mock_output, mock_call,
                              mock_raw_call):
    rust_uprev.upload_single_tarball(self.rust_url, self.tarfile_name,
                                     self.tempdir)
    mock_output.assert_called_once_with(
        ['gpg', '--verify', self.sig_file, self.rust_file],
        encoding='utf-8',
        stderr=subprocess.STDOUT)
    mock_raw_call.assert_has_calls([
        mock.call(['gsutil', 'ls', self.gsurl],
                  stdout=subprocess.DEVNULL,
                  stderr=subprocess.DEVNULL)
    ])
    mock_call.assert_has_calls([
        mock.call(['curl', '-f', '-o', self.rust_file, self.rust_src]),
        mock.call(['curl', '-f', '-o', self.sig_file, f'{self.rust_src}.asc']),
        mock.call([
            'gsutil', 'cp', '-n', '-a', 'public-read', self.rust_file,
            self.gsurl
        ])
    ])
    mock_run.assert_not_called()

  @mock.patch.object(subprocess, 'call')
  @mock.patch.object(subprocess, 'check_call')
  @mock.patch.object(subprocess, 'check_output')
  @mock.patch.object(subprocess, 'run')
  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_pass_with_retry(self, mock_output, mock_run, mock_check, mock_call,
                           mock_raw_call):
    mock_check.side_effect = subprocess.CalledProcessError(
        returncode=2, cmd=None, output="gpg: Can't check signature")
    mock_output.return_value = 'some_gpg_keys'
    rust_uprev.upload_single_tarball(self.rust_url, self.tarfile_name,
                                     self.tempdir)
    mock_check.assert_called_once_with(
        ['gpg', '--verify', self.sig_file, self.rust_file],
        encoding='utf-8',
        stderr=subprocess.STDOUT)
    mock_output.assert_called_once_with(
        ['curl', '-f', 'https://keybase.io/rust/pgp_keys.asc'])
    mock_run.assert_called_once_with(['gpg', '--import'],
                                     input='some_gpg_keys',
                                     encoding='utf-8',
                                     check=True)
    mock_raw_call.assert_has_calls([
        mock.call(['gsutil', 'ls', self.gsurl],
                  stdout=subprocess.DEVNULL,
                  stderr=subprocess.DEVNULL)
    ])
    mock_call.assert_has_calls([
        mock.call(['curl', '-f', '-o', self.rust_file, self.rust_src]),
        mock.call(['curl', '-f', '-o', self.sig_file, f'{self.rust_src}.asc']),
        mock.call(['gpg', '--verify', self.sig_file, self.rust_file]),
        mock.call([
            'gsutil', 'cp', '-n', '-a', 'public-read', self.rust_file,
            self.gsurl
        ])
    ])

  @mock.patch.object(rust_uprev, 'upload_single_tarball')
  def test_upload_to_mirror(self, mock_upload):
    stage0_info = '2020-01-01', '1.1.1', '0.1.0'
    rust_uprev.upload_to_localmirror(self.tempdir, self.new_version,
                                     stage0_info)
    mock_upload.assert_has_calls([
        mock.call(self.rust_url, f'rustc-{self.new_version}-src.tar.gz',
                  self.tempdir),
        mock.call(f'{self.rust_url}/{stage0_info[0]}',
                  f'rust-std-{stage0_info[1]}-x86_64-unknown-linux-gnu.tar.gz',
                  self.tempdir),
        mock.call(self.rust_url,
                  f'rustc-{stage0_info[1]}-x86_64-unknown-linux-gnu.tar.gz',
                  self.tempdir),
        mock.call(self.rust_url,
                  f'cargo-{stage0_info[2]}-x86_64-unknown-linux-gnu.tar.gz',
                  self.tempdir),
    ])


class RustUprevOtherStagesTests(unittest.TestCase):
  """Tests for other steps in rust_uprev"""

  def setUp(self):
    self.old_version = rust_uprev.RustVersion(1, 1, 0)
    self.current_version = rust_uprev.RustVersion(1, 2, 3)
    self.new_version = rust_uprev.RustVersion(1, 3, 5)
    self.ebuild_file = os.path.join(rust_uprev.RUST_PATH,
                                    'rust-{self.new_version}.ebuild')

  @mock.patch.object(rust_uprev, 'get_command_output')
  def test_parse_stage0_file(self, mock_get):
    stage0_file = """
    unrelated stuff before
    date: 2020-01-01
    rustc: 1.1.1
    cargo: 0.1.0
    unrelated stuff after
    """
    mock_get.return_value = stage0_file
    expected = '2020-01-01', '1.1.1', '0.1.0'
    rust_version = rust_uprev.RustVersion(1, 2, 3)
    actual = rust_uprev.parse_stage0_file(rust_version)
    self.assertEqual(expected, actual)
    mock_get.assert_called_once_with([
        'curl', '-f', 'https://raw.githubusercontent.com/rust-lang/rust/'
        f'{rust_version}/src/stage0.txt'
    ])

  @mock.patch.object(shutil, 'copyfile')
  @mock.patch.object(os, 'listdir')
  @mock.patch.object(subprocess, 'check_call')
  def test_copy_patches(self, mock_call, mock_ls, mock_copy):
    mock_ls.return_value = [
        f'rust-{self.old_version}-patch-1.patch',
        f'rust-{self.old_version}-patch-2-old.patch',
        f'rust-{self.current_version}-patch-1.patch',
        f'rust-{self.current_version}-patch-2-new.patch'
    ]
    rust_uprev.copy_patches(self.current_version, self.new_version)
    mock_copy.assert_has_calls([
        mock.call(
            os.path.join(rust_uprev.RUST_PATH, 'files',
                         f'rust-{self.current_version}-patch-1.patch'),
            os.path.join(rust_uprev.RUST_PATH, 'files',
                         f'rust-{self.new_version}-patch-1.patch'),
        ),
        mock.call(
            os.path.join(rust_uprev.RUST_PATH, 'files',
                         f'rust-{self.current_version}-patch-2-new.patch'),
            os.path.join(rust_uprev.RUST_PATH, 'files',
                         f'rust-{self.new_version}-patch-2-new.patch'))
    ])
    mock_call.assert_called_once_with(
        ['git', 'add', f'files/rust-{self.new_version}-*.patch'],
        cwd=rust_uprev.RUST_PATH)

  @mock.patch.object(shutil, 'copyfile')
  @mock.patch.object(subprocess, 'check_call')
  def test_create_ebuild(self, mock_call, mock_copy):
    template_ebuild = f'/path/to/rust-{self.current_version}-r2.ebuild'
    rust_uprev.create_ebuild(template_ebuild, self.new_version)
    mock_copy.assert_called_once_with(
        template_ebuild,
        os.path.join(rust_uprev.RUST_PATH, f'rust-{self.new_version}.ebuild'))
    mock_call.assert_called_once_with(
        ['git', 'add', f'rust-{self.new_version}.ebuild'],
        cwd=rust_uprev.RUST_PATH)

  @mock.patch.object(os.path, 'exists', return_value=True)
  @mock.patch.object(shutil, 'copyfile')
  @mock.patch.object(subprocess, 'check_call')
  def test_update_virtual_rust(self, mock_call, mock_copy, mock_exists):
    virtual_rust_dir = os.path.join(rust_uprev.RUST_PATH, '../../virtual/rust')
    rust_uprev.update_virtual_rust(self.current_version, self.new_version)
    mock_call.assert_called_once_with(
        ['git', 'add', f'rust-{self.new_version}.ebuild'], cwd=virtual_rust_dir)
    mock_copy.assert_called_once_with(
        os.path.join(virtual_rust_dir, f'rust-{self.current_version}.ebuild'),
        os.path.join(virtual_rust_dir, f'rust-{self.new_version}.ebuild'))
    mock_exists.assert_called_once_with(virtual_rust_dir)

  @mock.patch.object(os, 'listdir')
  def test_find_oldest_rust_version_in_chroot_pass(self, mock_ls):
    oldest_version_name = f'rust-{self.old_version}.ebuild'
    mock_ls.return_value = [
        oldest_version_name, f'rust-{self.current_version}.ebuild',
        f'rust-{self.new_version}.ebuild'
    ]
    actual = rust_uprev.find_oldest_rust_version_in_chroot()
    expected = (self.old_version,
                os.path.join(rust_uprev.RUST_PATH, oldest_version_name))
    self.assertEqual(expected, actual)

  @mock.patch.object(os, 'listdir')
  def test_find_oldest_rust_version_in_chroot_fail_with_only_one_ebuild(
      self, mock_ls):
    mock_ls.return_value = [f'rust-{self.new_version}.ebuild']
    with self.assertRaises(RuntimeError) as context:
      rust_uprev.find_oldest_rust_version_in_chroot()
    self.assertEqual('Expect to find more than one Rust versions',
                     str(context.exception))

  @mock.patch.object(rust_uprev, 'get_command_output')
  @mock.patch.object(git, 'CreateBranch')
  def test_create_new_repo(self, mock_branch, mock_output):
    mock_output.return_value = ''
    rust_uprev.create_new_repo(self.new_version)
    mock_branch.assert_called_once_with(rust_uprev.RUST_PATH,
                                        f'rust-to-{self.new_version}')

  @mock.patch.object(rust_uprev, 'get_command_output')
  @mock.patch.object(subprocess, 'check_call')
  def test_build_cross_compiler(self, mock_call, mock_output):
    mock_output.return_value = f'rust-{self.new_version}.ebuild'
    cros_targets = [
        'x86_64-cros-linux-gnu',
        'armv7a-cros-linux-gnueabihf',
        'aarch64-cros-linux-gnu',
    ]
    all_triples = ['x86_64-pc-linux-gnu'] + cros_targets
    rust_ebuild = 'RUSTC_TARGET_TRIPLES=(' + '\n\t'.join(all_triples) + ')'
    mock_open = mock.mock_open(read_data=rust_ebuild)
    with mock.patch('builtins.open', mock_open):
      rust_uprev.build_cross_compiler()

    mock_call.assert_called_once_with(
        ['sudo', 'emerge', '-j', '-G'] +
        [f'cross-{x}/gcc' for x in cros_targets + ['arm-none-eabi']])


if __name__ == '__main__':
  unittest.main()
