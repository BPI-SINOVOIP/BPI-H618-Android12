# Background

This document seeks to be a crash-course and cheat-sheet for running the NNAPI
fuzz tests.

The purpose of fuzz testing is to find crashes, assertions, memory violations,
or general undefined behavior in the code under test due to factors such as
unexpected inputs. For NNAPI fuzz testing, Android uses tests based on
`libFuzzer`, which are efficient at fuzzing because they use line coverage of
previous test cases to generate new random inputs. For example, `libFuzzer`
favors test cases that run on uncovered lines of code. This greatly reduces the
amount of time tests take to find problematic code.

Currently, there are two NNAPI fuzz test targets: `libneuralnetworks_fuzzer`
which tests at the NNAPI NDK layer (testing libneuralnetworks as a static
library) and `libneuralnetworks_driver_fuzzer` which tests an in-process driver
at the NNAPI HAL layer (the sample driver, unless the test is modified to do
otherwise). To simplify development of future tests, this directory also
defines an NNAPI fuzzing test harness and packages it in a blueprint default
`libneuralnetworks_fuzzer_defaults`.

Useful background reading and reference documents:
* libFuzzer overview: http://llvm.org/docs/LibFuzzer.html
* Android-specific libFuzzer documentation:
  https://source.android.com/devices/tech/debug/libfuzzer
* Android Security Testing (sanitizers, fuzzer, etc.):
  https://source.android.com/devices/tech/debug/fuzz-sanitize
* Sanitizer flags:
  https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
* Address Sanitizer flags:
  https://github.com/google/sanitizers/wiki/AddressSanitizerFlags
* libprotobuf-mutator:
  https://github.com/google/libprotobuf-mutator#libprotobuf-mutator

# Setting up the test

## Developing an NNAPI fuzz test

### Creating a new fuzz test using `libneuralnetworks_fuzzer_defaults`

To create a new fuzz test:
1. Create code that implements the function
   `void nnapiFuzzTest(const TestModel& testModel)` (examples: [1][1], [2][2])
2. Create a blueprint `cc_fuzz` target that includes
   `libneuralnetworks_fuzzer_defaults` as a default (examples: [1][3], [2][4])

### Modifying `libneuralnetworks_driver_fuzzer` to test custom driver

Alter the `libneuralnetworks_driver_fuzzer` code locally to test your own
driver. In the section `“TODO: INSERT CUSTOM DEVICE HERE”`, replace
`“new nn::sample_driver::SampleDriverFull(…);”` ([link][5]) with your own
driver.

This code employs an in-process driver (as opposed to retrieving it on the
device via `IDevice::getService(...))` for three reasons. First, the test runs
faster because it does not need to communicate with the driver via IPC because
the driver is created in the same process. Second, it ensures that the
`libFuzzer` can use the coverage from the driver to guide the test
appropriately, as everything is built as one unit. Finally, whenever a crash
occurs, only one stacktrace needs to be analyzed to debug the problem.

The current version of the test assumes a 1.3 driver and uses the methods
`IDevice::prepareModel_1_3` and `IDevice::executeSynchronously_1_3`
([link][6]). Change the test locally to test different methods or different
driver versions.

## Preparing a device

Because the test is self-contained, you should be able to just use a regular
device image without any modifications. The next section
[Building and uploading fuzz test](#building-and-uploading-fuzz-test) describes
how to build the test binary itself. If you need to have the entire image
fuzzed (for example, if you want to sanitize a shared library), you can build a
sanitized image with one of the following two sequences of commands depending
on your needs:

### You can build a pre-configured sanitized device image with:
```bash
$ . build/envsetup.sh
$ lunch <sanitized_target>  # e.g., <TARGET_PRODUCT>_hwasan-userdebug
$ mma -j
```

### Alternatively, you can build other (read: non-sanitized) targets with the following command:
```bash
$ . build/envsetup.sh
$ lunch <non-sanitized_target>  # e.g., <TARGET_PRODUCT>-userdebug
$ SANITIZE_TARGET=hwaddress mma -j
```

## Building and uploading fuzz test

For simplicity and clarity, the rest of the code here will use the following
environment variables:
```
$ FUZZER_NAME=libneuralnetworks_driver_fuzzer
$ FUZZER_TARGET_ARCH=$(get_build_var TARGET_ARCH)
$ FUZZER_TARGET_DIR=/data/fuzz/$FUZZER_TARGET_ARCH/$FUZZER_NAME
$ FUZZER_TARGET=$FUZZER_TARGET_DIR/$FUZZER_NAME
```

When using a sanitized lunch target, build the fuzz test with the following
command:
```bash
$ m $FUZZER_NAME -j
```

When building with a non-sanitized lunch target, build the fuzz test with the
following command:
```bash
$ SANITIZE_TARGET=hwaddress m $FUZZER_NAME -j
```

Note that the above commands use `hwaddress` sanitization, but other sanitizers
can be used in place of or in addition to `hwaddress`. More command options for
building with other sanitizers can be found [here][7], and they are explained
more in depth in the Android background reading [here][8].

Once the test is built, it can be pushed to the device via:
```bash
$ adb root
$ adb sync data
$ adb shell mkdir -p $FUZZER_TARGET_DIR/dump
```

The directory `$FUZZER_TARGET_DIR/` is now as follows:
* `$FUZZER_NAME` -- fuzz test binary
* `corpus/` -- directory for reference/example “good” test cases, used to speed
  up fuzz tests
* `dump/` -- sandbox directory used by the fuzz test; this can be ignored
* `crash-*` -- any future problematic test cases will be dumped to the directory

# Running the test

## Running the full fuzz test

The fuzz test can be launched with the following command, and will continue
running until the user terminates the process (e.g., ctrl+c) or until the test
crashes.

```bash
$ adb shell HWASAN_OPTIONS=handle_sigill=2:handle_sigfpe=2:handle_sigbus=2:handle_abort=2:handle_segv=2 $FUZZER_TARGET $FUZZER_TARGET_DIR/dump/ $FUZZER_TARGET_DIR/corpus/ -artifact_prefix=$FUZZER_TARGET_DIR/
```

(When using a non-hwasan build, you need to change the `HWASAN_OPTIONS`
variable to match whatever build you’re using, e.g., `ASAN_OPTIONS`.)

When something unexpected occurs (e.g., a crash or a very slow test case), the
test case that causes it will be dumped to a file in the directory specified by
“`-artifact_prefix`”. The generated file will appear as
`slow-unit-<unique_identifier>`, `crash-<unique_identifier>`,
`oom-<unique_identifier>`, or `timeout-<unique_identifier>`. Normally,
`libFuzzer` crash files will contain unreadable binary data; however,
`libneuralnetworks_driver_fuzzer`‘s output is formatted in a human readable way
because it uses `libprotobuf-mutator`, so it’s fine to inspect the file to get
more information on the test case that caused the problem. For more
information, refer to the [Fuzz test case format](#fuzz-test-case-format)
section below.

## Reproducing crash case

When a crash occurs, the crash test case can be re-run with the following
command:

```bash
$ adb shell HWASAN_OPTIONS=handle_sigill=2:handle_sigfpe=2:handle_sigbus=2:handle_abort=2:handle_segv=2 $FUZZER_TARGET $FUZZER_TARGET_DIR/<test_case_name>
```
(Note that the execution parameters for `HWASAN_OPTIONS` are the same as those
above.)

E.g., `<test_case_name>` could be:
* `minimized-from-15b1dae0d2872d8dccf4f35fbf4ecbecee697a49`
* `slow-unit-cad88bd58853b71b875ac048001b78f7a7501dc3`
* `crash-07cb8793bbc65ab010382c0f8d40087897826129`

# Finding minimal crash case

When a crash occurs, sometimes the offending test case is large and
complicated. `libFuzzer` has a way to minimize the crashing case to simplify
debugging with the following command:

```bash
$ adb shell HWASAN_OPTIONS=handle_sigill=2:handle_sigfpe=2:handle_sigbus=2:handle_abort=2:handle_segv=2 $FUZZER_TARGET $FUZZER_TARGET_DIR/<test_case_name> -artifact_prefix=$FUZZER_TARGET_DIR/ -minimize_crash=1 -max_total_time=60
```
(Note that the execution parameters for `HWASAN_OPTIONS` are the same as those
above.)

Note that the `<test_case_name>` must be some sort of crash for the
minimization to work. For example, minimization will not work on something like
`slow_unit-*` cases. Increasing the `max_total_time` value may yield a more
minimal test crash, but will take longer.

## Fuzz test case format

By itself, `libFuzzer` will generate a random collection of bytes as input to
the fuzz test. The test developer then needs to convert this random data to
some structured testing format (e.g., a syntactically correct NNAPI model).
Doing this conversion can be slow and difficult, and can lead to inefficient
mutations and tests. Additionally, whenever the fuzz test finds a crashing test
case, it will dump this test case as an unreadable binary chunk of data in a
file (e.g., `crash-*` files described above).

To help with both of these issues, the NNAPI fuzz tests additionally use a
library called [`libprotobuf-mutator`][9] to handle the conversions from the
random `libFuzzer` input to a protobuf format used for NNAPI fuzz testing. The
conversion from this protobuf format to a model format is much more
straightforward and efficient. As another useful utility, `libprotobuf-mutator`
provides the option to represent this data as human-readable text. This means
that whenever the fuzz test finds a crash, the resultant test case that is
dumped to a file will be in a human-readable format.

Here is one example of a crash case that was found:
```
model {
 operands {
   operand {
     type: TENSOR_INT32
     dimensions {
       dimension: 1
     }
     scale: 0
     zero_point: 0
     lifetime: TEMPORARY_VARIABLE
     channel_quant {
       scales {
       }
       channel_dim: 0
     }
     data {
       random_seed: 4
     }
   }
   operand {
     type: TENSOR_FLOAT32
     dimensions {
       dimension: 2
       dimension: 4
     }
     scale: 0
     zero_point: 0
     lifetime: TEMPORARY_VARIABLE
     channel_quant {
       scales {
       }
       channel_dim: 0
     }
     data {
       random_seed: 0
     }
   }
   operand {
     type: TENSOR_FLOAT32
     dimensions {
     }
     scale: 0
     zero_point: 0
     lifetime: SUBGRAPH_OUTPUT
     channel_quant {
       scales {
       }
       channel_dim: 27
     }
     data {
       random_seed: 0
     }
   }
 }
 operations {
   operation {
     type: EMBEDDING_LOOKUP
     inputs {
       index: 0
       index: 1
     }
     outputs {
       index: 2
     }
   }
 }
 input_indexes {
   index: 0
   index: 1
 }
 output_indexes {
   index: 2
 }
 is_relaxed: true
}
```

This format is largely based on the format defined in [NNAPI HAL][10]. The one
major exception is that the contents of an operand's data are replaced by data
generated from “random_seed” (except for `TEMPORARY_VARIABLE` and `NO_VALUE`
operands, in which cases there is no data, so "random_seed" is ignored). This
is done for a practical reason: `libFuzzer` (and by extension
`libprotobuf-mutator`) converge slower when the amount of randomly generated
input is large. For the fuzz tests, the contents of the operand data are not as
interesting as the structure of the graph itself, so the data was replaced by
a seed to a random number generator instead.

[1]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/android_fuzzing/DriverFuzzTest.cpp;l=307-324;drc=34aee872d5dc317ad8a32377e9114c0c606d8afe
[2]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/android_fuzzing/FuzzTest.cpp;l=130-151;drc=34aee872d5dc317ad8a32377e9114c0c606d8afe
[3]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/Android.bp;l=195-216;drc=60823f07172e6b5bbc06b2fac25a15ab91c80b25
[4]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/Android.bp;l=218-240;drc=60823f07172e6b5bbc06b2fac25a15ab91c80b25
[5]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/android_fuzzing/DriverFuzzTest.cpp;l=48-52;drc=34aee872d5dc317ad8a32377e9114c0c606d8afe
[6]: https://cs.android.com/android/platform/superproject/+/master:packages/modules/NeuralNetworks/runtime/test/android_fuzzing/DriverFuzzTest.cpp;l=291-292,302;drc=34aee872d5dc317ad8a32377e9114c0c606d8afe
[7]: https://cs.android.com/android/platform/superproject/+/master:build/soong/cc/sanitize.go;l=140-187;drc=b5b2aba43b5bb6305ea69d60f9bf580f711d7c96
[8]: https://source.android.com/devices/tech/debug/libfuzzer
[9]: https://cs.android.com/android/platform/superproject/+/master:external/libprotobuf-mutator/
[10]: https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/neuralnetworks/
