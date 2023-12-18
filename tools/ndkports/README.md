# ndkports

A collection of Android build scripts for various third-party libraries and the
tooling to build them.

If you're an Android app developer looking to *consume* these libraries, this is
probably not what you want. This project builds AARs to be published to Maven.
You most likely want to use the AAR, not build it yourself.

Note: Gradle support for consuming these artifacts from an AAR is a work in
progress.

## Ports

Each third-party project is called a "port". Ports consist of a description of
where to fetch the source, apply any patches needed, build, install, and package
the library into an AAR.

A port is a subclass of the abstract Kotlin class `com.android.ndkports.Port`.
Projects define the name and version of the port, the URL to fetch source from,
a list of modules (libraries) to build, and the build steps.

See the [Port class] for documentation on the port API.

Individual port files are kept in `ports/$name/port.kts`. For example, the cURL
port is [ports/curl/port.kts](ports/curl/port.kts).

[Port class]: src/main/kotlin/com/android/ndkports/Port.kt

## Building a Port

ndkports requires an NDK to be used for building to be specified on the command
line as well as a list of packages to build. For example, to build cURL:

```bash
$ ./gradlew run --args='--ndk /path/to/android-ndk-r20 openssl curl'
Build output...
$ find  -name '*.aar'
./out/curl/curl.aar
./out/openssl/openssl.aar
```

Note that dependencies currently need to be already built or ordered explicitly.

To build all ports using Docker, use `scripts/build.sh`.
