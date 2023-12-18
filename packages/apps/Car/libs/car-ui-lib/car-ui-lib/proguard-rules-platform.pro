# This was added to fix issues with androidx CREATOR objects being removed.
# On google3 it is added automatically, so keep it in a separate file
# and don't use this file in google3.
-keepclassmembers class * implements android.os.Parcelable {
    static ** CREATOR;
}
