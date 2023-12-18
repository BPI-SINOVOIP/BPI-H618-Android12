# external_updater

external updater is a tool to automatically updates libraries in external/ .

## Usage

Check updates for a library, or verify METADATA is valid:

```shell
tools/external_updater/updater.sh check ${LIBNAME}
```

Check updates for all libraries in external/:

```shell
tools/external_updater/updater.sh check --all
```

Update a library:

```shell
tools/external_updater/updater.sh update ${LIBNAME}
```

LIBNAME can be the path to a library under external/. E.g. kotlinc, or
python/cpython3.

## Configure

To use this tool, a METADATA file must present at the root of the 
repository. The full definition can be found
[here](https://android.googlesource.com/platform/tools/external_updater/+/refs/heads/master/metadata.proto).
Or see example [here](https://android.googlesource.com/platform/external/ImageMagick/+/refs/heads/master/METADATA)

The most important part in the file is a list of urls.
`external_updater` will go through all urls and uses the first
supported url.

### Git upstream

If type of a URL is set to GIT, the URL must be a git upstream
(the one you can use with `git clone`). And the version field must
be either a version tag, or SHA. The tool will find the latest
version tag or sha based on it.

When upgrade, the tool will simply run `git merge tag/sha`.

IMPORTANT: It is suggested to set up a `upstream-master` branch to
replicate upstream. Because most users don't have the privilege to
upload changes not authored by themselves. This can be done by
filing a bug to componentid:99104.

#### SHA

If the version is a SHA, the tool will always try to upgrade to the
top of upstream. As long as there is any new change upstream, local
library will be treated as stale.

#### Version tag

If the version is not a SHA, the tool will try to parse the version
to get a numbered version. Currently the supported version format is:

```markdown
<prefix><version_number><suffix>
```

version_number part can be numbers separated by `.` or `-` or `_`.

If you have project where this isn't working, file a bug so we can take a look.

#### Local changes

It is suggested to verify all local changes when upgrading. This can
be done easily in Gerrit, by comparing parent2 and the patchset.


### GitHub archive

If the url type is ARCHIVE, and the url is from GitHub, `external_updater`
can upgrade a library based on GitHub releases.

If you have the choice between archives and git tags, choose tags.
Because that makes it easier to manage local changes.

The tool will query GitHub to get the latest release from:

```url
https://github.com/user/proj/releases/latest
```

If the tag of latest release is not equal to version in METADATA file, a
new version is found. The tool will download the tarball and overwrite the
library with it.

If there are multiple archives in one GitHub release, the one most
[similar](https://en.wikipedia.org/wiki/Edit_distance) to previous
(from METADATA) will be used.

After upgrade, files not present in the new tarball will be removed. But we
explicitly keep files famous in Android tree.
See [here](https://android.googlesource.com/platform/tools/external_updater/+/refs/heads/master/update_package.sh).

If more files need to be reserved, a post_update.sh can be created to copy
these files over.
See [example](https://android.googlesource.com/platform/external/kotlinc/+/refs/heads/master/post_update.sh).

Local patches can be kept as patches/*.diff. They will be applied after
upgrade. [example](https://cs.corp.google.com/android/external/jsmn/patches/header.diff)

## Email notification

There is some support to automatically check updates for all external 
libraries every hour, send email and change. Currently this is done by 
running the following script on a desktop machine.

```shell
#!/bin/bash

cd /src/aosp
while true
do
        repo abandon tmp_auto_upgrade
        repo forall -c git checkout .
        repo forall -c git clean -xdf
        repo sync -c
        source build/envsetup.sh
        lunch aosp_arm-eng
        mmma tools/external_updater

        out/soong/host/linux-x86/bin/external_updater_notifier \
                --history ~/updater/history \
                --recipients=android_external_lib_updates@google.com \
                --generate_change \
                --all
        date
        echo "Sleeping..."
        sleep 3600
done
```
