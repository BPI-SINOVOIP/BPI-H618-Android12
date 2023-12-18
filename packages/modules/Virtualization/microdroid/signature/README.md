# Microdroid Payload

Payload disk is a composite disk referencing host APEXes and an APK so that microdroid
reads activates APEXes and executes a binary within the APK.

## Format

Payload disk has 1 + N(number of APEX/APK payloads) partitions.

The first partition is a Microdroid Signature partition which describes other partitions.
And APEXes and an APK are following as separate partitions.

For now, the order of partitions are important.

* partition 1: Microdroid Signature
* partition 2 ~ n: APEX payloads
* partition n + 1: APK payload

It's subject to change in the future, though.

### Microdroid Signature

Microdroid Signature contains the signatures of the payloads so that the payloads are
verified inside the Guest OS.

Microdroid Signature is composed of header and body.

| offset | size | description                                                    |
|--------|------|----------------------------------------------------------------|
| 0      | 4    | Header. unsigned int32: body length(L) in big endian           |
| 4      | L    | Body. A protobuf message. [schema](microdroid_signature.proto) |

### Payload Partitions

At the end of each payload partition the size of the original payload file (APEX or APK) is stored
in 4-byte big endian.

For example, the following code shows how to get the original size of host apex file
when the apex is read in microdroid as /dev/block/vdc2,

    int fd = open("/dev/block/vdc2", O_RDONLY | O_BINARY | O_CLOEXEC);
    uint32_t size;
    lseek(fd, -sizeof(size), SEEK_END);
    read(fd, &size, sizeof(size));
    size = betoh32(size);

## How to Create

### `mk_payload`

`mk_payload` creates a payload image.
```
$ cat payload_config.json
{
  "system_apexes": [
    "com.android.adbd",
  ],
  "apexes": [
    {
      "name": "com.my.hello",
      "path": "hello.apex"
    }
  ],
  "apk": {
    "name": "com.my.world",
    "path": "/path/to/world.apk"
  }
}
$ adb push payload_config.json hello.apex /data/local/tmp/
$ adb shell 'cd /data/local/tmp; /apex/com.android.virt/bin/mk_payload payload_config.json payload.img
$ adb shell ls /data/local/tmp/*.img
payload.img
payload-footer.img
payload-header.img
payload-signature.img
payload.img.0          # fillers
payload.img.1
...
```

In the future, [VirtManager](../../virtmanager) will handle this.