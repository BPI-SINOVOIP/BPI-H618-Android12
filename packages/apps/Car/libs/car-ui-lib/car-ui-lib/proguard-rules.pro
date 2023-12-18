# required for replacing elements in PreferenceFragment
-keep class com.android.car.ui.preference.CarUiDropDownPreference {*;}
-keep class com.android.car.ui.preference.CarUiListPreference {*;}
-keep class com.android.car.ui.preference.CarUiMultiSelectListPreference {*;}
-keep class com.android.car.ui.preference.CarUiEditTextPreference {*;}
-keep class com.android.car.ui.preference.CarUiSwitchPreference {*;}
-keep class com.android.car.ui.preference.CarUiPreference {*;}
-keep class com.android.car.ui.preference.** extends com.android.car.ui.preference.CarUiPreference {*;}

# required for default scrollbar implementation.
-keep class com.android.car.ui.recyclerview.DefaultScrollBar {*;}

# required for MenuItem click listeners
-keepclasseswithmembers class * extends android.app.Activity {
  public void * (com.android.car.ui.toolbar.MenuItem);
}

# We dynamically link the oem apis, and proguard can't see them
# when running, so it errors out without -dontwarn
-dontwarn com.android.car.ui.sharedlibrary.oemapis.**

# Required because the static lib doesn't call most of the methods
# on adapters, but instead passes it to the shared lib, where they
# are called. Since proguard can't even see that those methods are
# overriding oem api interfaces (since the oem apis are dynmically
# linked and marked with -dontwarn), it thinks they're unused.
-keep class com.android.car.ui.**AdapterV* {*;}

# required for accessing oem apis
-keep class com.android.car.ui.sharedlibrarysupport.OemApiUtil {*;}

# Required for AppCompat instantiating our layout inflater factory,
# Otherwise it will be obfuscated and the reference to it in xml won't match
-keep class com.android.car.ui.CarUiLayoutInflaterFactory {*;}

# Required for reflection code in CarUiInstaller
-keep class com.android.car.ui.baselayout.Insets {*;}
-keep class com.android.car.ui.core.BaseLayoutController {*;}
