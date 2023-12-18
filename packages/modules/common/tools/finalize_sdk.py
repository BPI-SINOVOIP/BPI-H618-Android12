#!/usr/bin/python3

import argparse
import os
import re
import subprocess
import sys
import tempfile
import zipfile

from collections import defaultdict

# See go/fetch_artifact for details on this script.
FETCH_ARTIFACT = '/google/data/ro/projects/android/fetch_artifact'
BUILD_TARGET = 'mainline_modules-userdebug'
ARTIFACT_PATTERN = 'mainline-sdks/*-%d.zip'
COMMIT_TEMPLATE = """Finalize artifacts for extension SDK %d

Import from build id %s.

Generated with:
$ %s

Bug: %d
Test: presubmit"""

def fail(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    sys.exit(1)

def fetch_artifacts(target, build_id, artifact_path):
    tmpdir = tempfile.TemporaryDirectory().name
    os.mkdir(tmpdir)
    print('Fetching %s from %s ...' % (artifact_path, target))
    fetch_cmd = [FETCH_ARTIFACT]
    fetch_cmd.extend(['--bid', str(build_id)])
    fetch_cmd.extend(['--target', target])
    fetch_cmd.append(artifact_path)
    fetch_cmd.append(tmpdir)
    print("Running: " + ' '.join(fetch_cmd))
    try:
        subprocess.check_output(fetch_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        fail('FAIL: Unable to retrieve %s artifact for build ID %s' % (artifact_path, build_id))
    return tmpdir

def repo_for_sdk(filename):
    module = filename.split('-')[0]
    target_dir = ''
    if module == 'media': return 'prebuilts/module_sdk/Media'
    if module == 'tethering': return 'prebuilts/module_sdk/Connectivity'
    for dir in os.listdir('prebuilts/module_sdk/'):
        if module.lower() in dir.lower():
            if target_dir:
                fail('Multiple target dirs matched "%s": %s' % (module, (target_dir, dir)))
            target_dir = dir
    if not target_dir:
        fail('Could not find a target dir for %s' % filename)

    return 'prebuilts/module_sdk/%s' % target_dir

def dir_for_sdk(filename, version):
    base = str(version)
    if 'test-exports' in filename:
        return os.path.join(base, 'test-exports')
    if 'host-exports' in filename:
        return os.path.join(base, 'host-exports')
    return base

if not os.path.isdir('build/soong'):
    fail("This script must be run from the top of an Android source tree.")

parser = argparse.ArgumentParser(description=('Finalize an extension SDK with prebuilts'))
parser.add_argument('-f', '--finalize_sdk', type=int, required=True, help='The numbered SDK to finalize.')
parser.add_argument('-b', '--bug', type=int, required=True, help='The bug number to add to the commit message.')
parser.add_argument('-a', '--amend_last_commit', action="store_true", help='Amend current HEAD commits instead of making new commits.')
parser.add_argument('bid', help='Build server build ID')
args = parser.parse_args()

branch_name = 'finalize-%d' % args.finalize_sdk
cmdline = " ".join(filter(lambda x: x not in ['-a', '--amend_last_commit'], sys.argv))
commit_message = COMMIT_TEMPLATE % (args.finalize_sdk, args.bid, cmdline, args.bug)

tmpdir = fetch_artifacts(BUILD_TARGET, args.bid, ARTIFACT_PATTERN % args.finalize_sdk)

created_dirs = defaultdict(list)

for f in os.listdir(tmpdir):
    repo = repo_for_sdk(f)
    dir = dir_for_sdk(f, args.finalize_sdk)
    target_dir = os.path.join(repo, dir)
    if os.path.isfile(target_dir):
        print('Removing existing dir %s' % target_dir)
        shutil.rmtree(target_dir)
    with zipfile.ZipFile(os.path.join(tmpdir, f)) as zipFile:
        zipFile.extractall(target_dir)

    print('Created %s' % target_dir)
    created_dirs[repo].append(dir)

subprocess.check_output(['repo', 'start', branch_name] + list(created_dirs.keys()))
print('Running git commit')
for repo in created_dirs:
    git = ['git', '-C', repo]
    subprocess.check_output(git + ['add'] + created_dirs[repo])
    if args.amend_last_commit:
        change_id = '\n' + re.search(r'Change-Id: [^\\n]+', str(subprocess.check_output(git + ['log', '-1']))).group(0)
        subprocess.check_output(git + ['commit', '--amend', '-m', commit_message + change_id])
    else:
        subprocess.check_output(git + ['commit', '-m', commit_message])
