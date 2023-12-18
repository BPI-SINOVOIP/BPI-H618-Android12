# [Android Automotive](https://source.android.com/devices/automotive) Developer Tools

AADevT contains tools for AA device developers to improve their productivity.

* Fully unbundle from Android versions, and use the AOSP trunk based
development model.

* Bridge the app ecosystem for [Android Automotive OS](https://developer.android.com/training/cars)
because device developers are the key producers for the core apps.

* Leverage [Android app developer tools](https://developer.android.com/studio/intro)
that a few million developers use.

[TOC]

## Dev Tools
This contains tools designed specifically to simplify the AA device development
workflows.

* clone_proj.sh to clone a git project for the unbundled development workflows
in instead of the whole Android repo.

## Chnage Reports

### sysui_oem_diff.sh
sysui_oem_diff.sh generates a summary of code changes between 2 revisions.
Which gives you a rough idea of changes on files and Lines of Code.

* The report is especailly useful to discuss the pain points on sysui/notif
customization with AAOS team.
* For example, to generate the change report for Android 11 to 10 QPR3: [sysui_gcar_android10-qpr3-release_android11-release.txt](dev/resource/sysui_gcar_android10-qpr3-release_android11-release.txt)

```
$ ./sysui_oem_diff.sh ~/Android/android11-release remotes/aosp/android10-qpr3-release remotes/aosp/android11-release > sysui_gcar_android10-qpr3-release_android11-release.txt
```

## System Performance Tuning
AAOS system performance turning is hard. Here are the tools to make it a bit
easier for the device developers.

* time_to_init_disp.sh to measure an app's [Time the Initial Display](https://developer.android.com/topic/performance/vitals/launch-time#time-initial)

## [Android Virtual Device as a Development Platform](avd/README.md)
