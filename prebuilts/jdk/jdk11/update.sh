#!/bin/bash -ev

# Updates prebuilts/jdk/jdk11 from build server.
# Usage:
#    $ cd <REPO>
#    $ repo start jdk11_update .
#    $ prebuilts/jdk/jdk11/update-jdk11.sh <BUILD_NUMBER>
#    $ repo upload --cbr .


BUILD_NUMBER="${1:?Specify build number}"
LINUX_ZIP=/tmp/$$.linux.zip
DARWIN_ZIP=/tmp/$$.darwin.zip

FETCH_ARTIFACT=/google/data/ro/projects/android/fetch_artifact
cd "$(dirname $0)"

rm -rf staging
mkdir -p staging

$FETCH_ARTIFACT --bid ${BUILD_NUMBER} --disable_progressbar --target linux_openjdk11 jdk.zip ${LINUX_ZIP}
$FETCH_ARTIFACT --bid ${BUILD_NUMBER} --disable_progressbar --target darwin_mac_openjdk11 jdk.zip ${DARWIN_ZIP}
$FETCH_ARTIFACT --bid ${BUILD_NUMBER} --disable_progressbar --target linux_openjdk11 manifest_${BUILD_NUMBER}.xml staging/manifest.xml

rm -rf staging/linux-x86
mkdir -p staging/linux-x86
(cd staging/linux-x86; unzip -q ${LINUX_ZIP})
touch staging/linux-x86/MODULE_LICENSE_GPL

rm -rf staging/darwin-x86
mkdir staging/darwin-x86
(cd staging/darwin-x86; unzip -q ${DARWIN_ZIP})
touch staging/darwin-x86/MODULE_LICENSE_GPL

rm -f ${LINUX_ZIP} ${DARWIN_ZIP}

version=$(staging/linux-x86/bin/java -version 2>&1 | grep "OpenJDK Runtime Environment" | sed -e 's/.*(\(.*\))/\1/')

# Commit to staging
git add staging
git commit -m "Add JDK ${version} to staging

Test: none"

git rm -rf linux-x86 darwin-x86 manifest.xml
git mv staging/* .
rmdir staging
git commit -m "Switch to JDK $version" --edit
