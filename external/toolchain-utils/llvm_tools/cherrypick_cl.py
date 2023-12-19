#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=cros-logging-import

"""Adds a cherrypick to LLVM's PATCHES.json."""

from __future__ import print_function

import argparse
import json
import logging
import os
import shlex
import subprocess
import sys

import chroot
import get_llvm_hash
import git
import git_llvm_rev
import update_chromeos_llvm_hash


def add_cherrypick(patches_json_path: str, patches_dir: str,
                   relative_patches_dir: str, start_version: git_llvm_rev.Rev,
                   llvm_dir: str, rev: git_llvm_rev.Rev, sha: str,
                   package: str):
  with open(patches_json_path, encoding='utf-8') as f:
    patches_json = json.load(f)

  file_name = sha + '.patch'
  rel_patch_path = os.path.join(relative_patches_dir, file_name)

  for p in patches_json:
    rel_path = p['rel_patch_path']
    if rel_path == rel_patch_path:
      raise ValueError('Patch at %r already exists in PATCHES.json' % rel_path)
    if sha in rel_path:
      logging.warning(
          'Similarly-named patch already exists in PATCHES.json: %r', rel_path)

  with open(os.path.join(patches_dir, file_name), 'wb') as f:
    cmd = ['git', 'show', sha]
    # Only apply the part of the patch that belongs to this package, expect
    # LLVM. This is because some packages are built with LLVM ebuild on X86 but
    # not on the other architectures. e.g. compiler-rt. Therefore always apply
    # the entire patch to LLVM ebuild as a workaround.
    if package != 'llvm':
      cmd.append(package_to_project(package))
    subprocess.check_call(cmd, stdout=f, cwd=llvm_dir)

  commit_subject = subprocess.check_output(
      ['git', 'log', '-n1', '--format=%s', sha], cwd=llvm_dir, encoding='utf-8')

  patches_json.append({
      'comment': commit_subject.strip(),
      'rel_patch_path': rel_patch_path,
      'start_version': start_version.number,
      'end_version': rev.number,
  })

  temp_file = patches_json_path + '.tmp'
  with open(temp_file, 'w', encoding='utf-8') as f:
    json.dump(patches_json, f, indent=4, separators=(',', ': '))
  os.rename(temp_file, patches_json_path)


def parse_ebuild_for_assignment(ebuild_path: str, var_name: str) -> str:
  # '_pre' filters the LLVM 9.0 ebuild, which we never want to target, from
  # this list.
  candidates = [
      x for x in os.listdir(ebuild_path)
      if x.endswith('.ebuild') and '_pre' in x
  ]

  if not candidates:
    raise ValueError('No ebuilds found under %r' % ebuild_path)

  ebuild = os.path.join(ebuild_path, max(candidates))
  with open(ebuild, encoding='utf-8') as f:
    var_name_eq = var_name + '='
    for orig_line in f:
      if not orig_line.startswith(var_name_eq):
        continue

      # We shouldn't see much variety here, so do the simplest thing possible.
      line = orig_line[len(var_name_eq):]
      # Remove comments
      line = line.split('#')[0]
      # Remove quotes
      line = shlex.split(line)
      if len(line) != 1:
        raise ValueError('Expected exactly one quoted value in %r' % orig_line)
      return line[0].strip()

  raise ValueError('No %s= line found in %r' % (var_name, ebuild))


# Resolves a git ref (or similar) to a LLVM SHA.
def resolve_llvm_ref(llvm_dir: str, sha: str) -> str:
  return subprocess.check_output(
      ['git', 'rev-parse', sha],
      encoding='utf-8',
      cwd=llvm_dir,
  ).strip()


# Get the package name of an LLVM project
def project_to_package(project: str) -> str:
  if project == 'libunwind':
    return 'llvm-libunwind'
  return project


# Get the LLVM project name of a package
def package_to_project(package: str) -> str:
  if package == 'llvm-libunwind':
    return 'libunwind'
  return package


# Get the LLVM projects change in the specifed sha
def get_package_names(sha: str, llvm_dir: str) -> list:
  paths = subprocess.check_output(
      ['git', 'show', '--name-only', '--format=', sha],
      cwd=llvm_dir,
      encoding='utf-8').splitlines()
  # Some LLVM projects are built by LLVM ebuild on X86, so always apply the
  # patch to LLVM ebuild
  packages = {'llvm'}
  # Detect if there are more packages to apply the patch to
  for path in paths:
    package = project_to_package(path.split('/')[0])
    if package in ('compiler-rt', 'libcxx', 'libcxxabi', 'llvm-libunwind'):
      packages.add(package)
  packages = list(sorted(packages))
  return packages


def main():
  chroot.VerifyOutsideChroot()
  logging.basicConfig(
      format='%(asctime)s: %(levelname)s: %(filename)s:%(lineno)d: %(message)s',
      level=logging.INFO,
  )

  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      '--chroot_path',
      default=os.path.join(os.path.expanduser('~'), 'chromiumos'),
      help='the path to the chroot (default: %(default)s)')
  parser.add_argument(
      '--start_sha',
      default='llvm-next',
      help='LLVM SHA that the patch should start applying at. You can specify '
      '"llvm" or "llvm-next", as well. Defaults to %(default)s.')
  parser.add_argument(
      '--sha',
      required=True,
      action='append',
      help='The LLVM git SHA to cherry-pick.')
  parser.add_argument(
      '--create_cl',
      default=False,
      action='store_true',
      help='Automatically create a CL if specified')
  args = parser.parse_args()

  llvm_symlink = chroot.ConvertChrootPathsToAbsolutePaths(
      args.chroot_path,
      chroot.GetChrootEbuildPaths(args.chroot_path, ['sys-devel/llvm']))[0]
  llvm_symlink_dir = os.path.dirname(llvm_symlink)

  git_status = subprocess.check_output(['git', 'status', '-s'],
                                       cwd=llvm_symlink_dir,
                                       encoding='utf-8')
  if git_status:
    raise ValueError('Uncommited changes detected in %s' %
                     os.path.dirname(os.path.dirname(llvm_symlink_dir)))

  start_sha = args.start_sha
  if start_sha == 'llvm':
    start_sha = parse_ebuild_for_assignment(llvm_symlink_dir, 'LLVM_HASH')
  elif start_sha == 'llvm-next':
    start_sha = parse_ebuild_for_assignment(llvm_symlink_dir, 'LLVM_NEXT_HASH')
  logging.info('Base llvm hash == %s', start_sha)

  llvm_config = git_llvm_rev.LLVMConfig(
      remote='origin', dir=get_llvm_hash.GetAndUpdateLLVMProjectInLLVMTools())

  start_sha = resolve_llvm_ref(llvm_config.dir, start_sha)
  start_rev = git_llvm_rev.translate_sha_to_rev(llvm_config, start_sha)

  if args.create_cl:
    branch = 'cherry-pick'
    git.CreateBranch(llvm_symlink_dir, branch)
    symlinks_to_uprev = []
    commit_messages = [
        'llvm: cherry-pick CLs from upstream\n',
    ]

  for sha in args.sha:
    sha = resolve_llvm_ref(llvm_config.dir, sha)
    rev = git_llvm_rev.translate_sha_to_rev(llvm_config, sha)
    # Find out the llvm projects changed in this commit
    packages = get_package_names(sha, llvm_config.dir)
    # Find out the ebuild symlinks of the corresponding ChromeOS packages
    symlinks = chroot.GetChrootEbuildPaths(args.chroot_path, [
        'sys-devel/llvm' if package == 'llvm' else 'sys-libs/' + package
        for package in packages
    ])
    symlinks = chroot.ConvertChrootPathsToAbsolutePaths(args.chroot_path,
                                                        symlinks)

    # Create a patch and add its metadata for each package
    for package, symlink in zip(packages, symlinks):
      symlink_dir = os.path.dirname(symlink)
      patches_json_path = os.path.join(symlink_dir, 'files/PATCHES.json')
      relative_patches_dir = 'cherry' if package == 'llvm' else ''
      patches_dir = os.path.join(symlink_dir, 'files', relative_patches_dir)
      logging.info('Cherrypicking %s (%s) into %s', rev, sha, package)

      add_cherrypick(patches_json_path, patches_dir, relative_patches_dir,
                     start_rev, llvm_config.dir, rev, sha, package)
    if args.create_cl:
      symlinks_to_uprev.extend(symlinks)
      commit_messages.extend([
          '\n\nreviews.llvm.org/rG%s\n' % sha,
          subprocess.check_output(['git', 'log', '-n1', '--oneline', sha],
                                  cwd=llvm_config.dir,
                                  encoding='utf-8')
      ])

  logging.info('Complete.')

  if args.create_cl:
    symlinks_to_uprev = list(sorted(set(symlinks_to_uprev)))
    for symlink in symlinks_to_uprev:
      update_chromeos_llvm_hash.UprevEbuildSymlink(symlink)
    subprocess.check_output(['git', 'add', '--all'], cwd=symlink_dir)
    git.UploadChanges(llvm_symlink_dir, branch, commit_messages)
    git.DeleteBranch(llvm_symlink_dir, branch)


if __name__ == '__main__':
  sys.exit(main())
