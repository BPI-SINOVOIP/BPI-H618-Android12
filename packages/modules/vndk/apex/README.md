# Add a new VNDK APEX

In this document we add a new VNDK APEX for version 30. When you follow this doc with different versions,
change "30" to what you're adding. (eg. 31)

1. Add a new definition in `Android.bp`

```
apex_vndk {
    name: "com.android.vndk.v30",
    manifest: "apex_manifest.v30.json",
    key: "com.android.vndk.v30.key",
    certificate: ":com.android.vndk.v30.certificate",
    vndk_version: "30",
    system_ext_specific: true,
    file_contexts: ":com.android.vndk-file_contexts",
}

apex_key {
    name: "com.android.vndk.v30.key",
    public_key: "com.android.vndk.v30.pubkey",
    private_key: "com.android.vndk.v30.pem",
}

android_app_certificate {
    name: "com.android.vndk.v30.certificate",
    certificate: "com.android.vndk.v30",
}
```

2. Add `apex_manifest.v30.json`

```
{
  "name": "com.android.vndk.v30",
  "version": 1
}
```

3. Add keys/ceritificate

```
openssl genrsa -out com.android.vndk.v30.pem 4096
avbtool extract_public_key --key com.android.vndk.v30.pem --output com.android.vndk.v30.pubkey
openssl req -x509 -newkey rsa:4096 -nodes -days 999999 -keyout key.pem -out com.android.vndk.v30.x509.pem

# Enter following info via the interactive prompts
# Country Name: US
# State: California
# Locality Name: Mountain View
# Organization Name: Android
# Organization Unit Name: Android
# Common Name: com.android.vndk
# Email address: android@android.com

openssl pkcs8 -topk8 -inform PEM -outform DER -in key.pem -out com.android.vndk.v30.pk8 -nocrypt

rm key.pem
```

4. Verify

```
m com.android.vndk.v30
```
