Fs-verity keys
==============
All AOSP compatible devices ship with the Google-managed fs-verity certificate
(located at build/make/target/product/security/fsverity-release.x509.der). The
public key can verify the signature prebuilt of .dm.fsv\_sig in res/.

Modifying a .dm file requires to regenerate the signature with some debug key.
To use the debug key, you can run the following commands once per boot.

```
KEY_DIR=$ANDROID_BUILD_TOP/cts/hostsidetests/appsecurity/test-apps/ApkVerityTestApp/testdata

adb root
adb shell 'mini-keyctl padd asymmetric fsv-play .fs-verity' < $KEY_DIR/fsverity-debug.x509.der
```

Alternatively, copy the .der file to /{system, product}/etc/security/fsverity.
The key will be located upon reboot.

How to modify the signed .dm
============================
The easiet way is to re-sign and replace the signature in place. For example,

```
m fsverity

fsverity sign CtsDexMetadataSplitApp.dm CtsDexMetadataSplitApp.dm.fsv_sig \
  --key="$KEY_DIR/fsverity-debug-key.pem" \
  --cert="$KEY_DIR/fsverity-debug.x509.pem"
```
