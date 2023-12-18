# AAOS SystemUI Sample RRO's

Each sample demonstrates the effects of overriding some configs in AAOS SystemUI.

## How to build and install RRO

```bash
# Enable RRO for the user 0
adb shell cmd overlay enable --user 0 com.android.systemui.rro
# Build all sample RRO's
mmma {path to the samples directory}
# Install one of the sample RRO's
adb install {path to the RRO apk}
# Restart SystemUI
adb shell pkill -TERM -f com.android.systemui
```