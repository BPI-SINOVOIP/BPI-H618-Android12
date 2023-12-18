/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tools.metalava

import org.junit.Test

class ApiLintTest : DriverTest() {

    @Test
    fun `Test names`() {
        // Make sure we only flag issues in new API
        check(
            apiLint = "", // enabled
            extraArguments = arrayOf(
                ARG_API_LINT_IGNORE_PREFIX,
                "android.icu.",
                ARG_API_LINT_IGNORE_PREFIX,
                "java.",
                ARG_HIDE, "MissingNullability"
            ),
            compatibilityMode = false,
            expectedIssues = """
                src/Dp.kt:3: warning: Acronyms should not be capitalized in method names: was `badCALL`, should this be `badCall`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/ALL_CAPS.java:3: warning: Acronyms should not be capitalized in class names: was `ALL_CAPS`, should this be `AllCaps`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/HTMLWriter.java:3: warning: Acronyms should not be capitalized in class names: was `HTMLWriter`, should this be `HtmlWriter`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/MyStringImpl.java:3: error: Don't expose your implementation details: `MyStringImpl` ends with `Impl` [EndsWithImpl] [See https://s.android.com/api-guidelines#dont-end-with-impl]
                src/android/pkg/badlyNamedClass.java:5: error: Class must start with uppercase char: badlyNamedClass [StartWithUpper] [See https://s.android.com/api-guidelines#style-conventions]
                src/android/pkg/badlyNamedClass.java:7: error: Method name must start with lowercase char: BadlyNamedMethod1 [StartWithLower] [See https://s.android.com/api-guidelines#style-conventions]
                src/android/pkg/badlyNamedClass.java:9: warning: Acronyms should not be capitalized in method names: was `fromHTMLToHTML`, should this be `fromHtmlToHtml`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/badlyNamedClass.java:10: warning: Acronyms should not be capitalized in method names: was `toXML`, should this be `toXml`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/badlyNamedClass.java:11: warning: Acronyms should not be capitalized in method names: was `getID`, should this be `getId`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg/badlyNamedClass.java:6: error: Constant field names must be named with only upper case characters: `android.pkg.badlyNamedClass#BadlyNamedField`, should be `BADLY_NAMED_FIELD`? [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class badlyNamedClass {
                        public static final int BadlyNamedField = 1;
                        public void BadlyNamedMethod1() { }

                        public void fromHTMLToHTML() { }
                        public void toXML() { }
                        @Nullable
                        public String getID() { return null; }
                        public void setZOrderOnTop() { } // OK
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class ALL_CAPS { // like android.os.Build.VERSION_CODES
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class HTMLWriter {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyStringImpl {
                    }
                    """
                ),
                java(
                    """
                    package android.icu;

                    import androidx.annotation.Nullable;

                    // Same as above android.pkg.badlyNamedClass but in a package
                    // that API lint is supposed to ignore (see ARG_API_LINT_IGNORE_PREFIX)
                    public class badlyNamedClass {
                        public static final int BadlyNamedField = 1;
                        public void BadlyNamedMethod1() { }

                        public void toXML() { }
                        @Nullable
                        public String getID() { return null; }
                        public void setZOrderOnTop() { }
                    }
                    """
                ),
                java(
                    """
                    package android.icu.sub;

                    import androidx.annotation.Nullable;

                    // Same as above android.pkg.badlyNamedClass but in a package
                    // that API lint is supposed to ignore (see ARG_API_LINT_IGNORE_PREFIX)
                    public class badlyNamedClass {
                        public static final int BadlyNamedField = 1;
                        public void BadlyNamedMethod1() { }

                        public void toXML() { }
                        @Nullable
                        public String getID() { return null; }
                        public void setZOrderOnTop() { }
                    }
                    """
                ),
                java(
                    """
                    package java;

                    import androidx.annotation.Nullable;

                    // Same as above android.pkg.badlyNamedClass but in a package
                    // that API lint is supposed to ignore (see ARG_API_LINT_IGNORE_PREFIX)
                    public class badlyNamedClass {
                        public static final int BadlyNamedField = 1;
                        public void BadlyNamedMethod1() { }

                        public void toXML() { }
                        @Nullable
                        public String getID() { return null; }
                        public void setZOrderOnTop() { }
                    }
                    """
                ),
                kotlin("""
                    inline class Dp(val value: Float)
                    fun greatCall(width: Dp)
                    fun badCALL(width: Dp)
                """),
                androidxNullableSource
            )
            /*
            expectedOutput = """
                9 new API lint issues were found.
                See tools/metalava/API-LINT.md for how to handle these.
            """
             */
        )
    }

    @Test
    fun `Test names against previous API`() {
        check(
            apiLint = """
                package android.pkg {
                  public class badlyNamedClass {
                    ctor public badlyNamedClass();
                    method public void BadlyNamedMethod1();
                    method public void fromHTMLToHTML();
                    method public String getID();
                    method public void toXML();
                    field public static final int BadlyNamedField = 1; // 0x1
                  }
                }
            """.trimIndent(),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/badlyNamedClass.java:8: warning: Acronyms should not be capitalized in method names: was `toXML2`, should this be `toXmL2`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg2/HTMLWriter.java:3: warning: Acronyms should not be capitalized in class names: was `HTMLWriter`, should this be `HtmlWriter`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                src/android/pkg2/HTMLWriter.java:4: warning: Acronyms should not be capitalized in method names: was `fromHTMLToHTML`, should this be `fromHtmlToHtml`? [AcronymName] [See https://s.android.com/api-guidelines#acronyms-in-method-name]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class badlyNamedClass {
                        public static final int BadlyNamedField = 1;

                        public void fromHTMLToHTML() { }
                        public void toXML() { }
                        public void toXML2() { }
                        public String getID() { return null; }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg2;

                    public class HTMLWriter {
                        public void fromHTMLToHTML() { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test constants`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/Constants.java:13: error: All constants must be defined at compile time: android.pkg.Constants#FOO [CompileTimeConstant]
                src/android/pkg/Constants.java:12: warning: If min/max could change in future, make them dynamic methods: android.pkg.Constants#MAX_FOO [MinMaxConstant] [See https://s.android.com/api-guidelines#min-max-constants]
                src/android/pkg/Constants.java:11: warning: If min/max could change in future, make them dynamic methods: android.pkg.Constants#MIN_FOO [MinMaxConstant] [See https://s.android.com/api-guidelines#min-max-constants]
                src/android/pkg/Constants.java:9: error: Constant field names must be named with only upper case characters: `android.pkg.Constants#myStrings`, should be `MY_STRINGS`? [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                src/android/pkg/Constants.java:7: error: Constant field names must be named with only upper case characters: `android.pkg.Constants#strings`, should be `STRINGS`? [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;

                    public class Constants {
                        private Constants() { }
                        @NonNull
                        public static final String[] strings = { "NONE", "WPA_PSK" };
                        @NonNull
                        public static final String[] myStrings = { "NONE", "WPA_PSK" };
                        public static final int MIN_FOO = 1;
                        public static final int MAX_FOO = 10;
                        @NonNull
                        public static final String FOO = System.getProperty("foo");
                    }
                    """
                ),
                androidxNonNullSource,
                androidxNullableSource
            )
        )
    }

    @Test
    fun `No enums`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyEnum.java:3: error: Enums are discouraged in Android APIs [Enum] [See https://s.android.com/api-guidelines#avoid-enum]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public enum MyEnum {
                       FOO, BAR
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test callbacks`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyCallback.java:9: error: Callback method names must follow the on<Something> style: bar [CallbackMethodName] [See https://s.android.com/api-guidelines#callback-method-naming]
                src/android/pkg/MyCallbacks.java:3: error: Callback class names should be singular: MyCallbacks [SingularCallback] [See https://s.android.com/api-guidelines#callback-class-singular]
                src/android/pkg/MyInterfaceCallback.java:3: error: Callbacks must be abstract class instead of interface to enable extension in future API levels: MyInterfaceCallback [CallbackInterface] [See https://s.android.com/api-guidelines#callback-abstract-instead-of-interface]
                src/android/pkg/MyObserver.java:3: warning: Class should be named MyCallback [CallbackName] [See https://s.android.com/api-guidelines#observer-should-be-callback]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyCallbacks {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public final class RemoteCallback {
                        public void sendResult();
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyObserver {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public interface MyInterfaceCallback {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyCallback {
                        public void onFoo() {
                        }
                        public void onAnimationStart() {
                        }
                        public void onN1GoodMethod() { }
                        public void bar() {
                        }
                        public static void staticMethod() {
                        }
                        public final void finalMethod() {
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test listeners`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClassListener.java:3: error: Listeners should be an interface, or otherwise renamed Callback: MyClassListener [ListenerInterface] [See https://s.android.com/api-guidelines#callbacks-listener]
                src/android/pkg/MyListener.java:6: error: Listener method names must follow the on<Something> style: bar [CallbackMethodName] [See https://s.android.com/api-guidelines#callback-method-naming]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyClassListener {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public interface OnFooBarListener {
                        void bar();
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public interface OnFooBarListener {
                        void onFooBar(); // OK
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public interface MyInterfaceListener {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public interface MyListener {
                        public void onFoo() {
                        }
                        public void bar() {
                        }
                        public static void staticMethod() {
                        }
                        public static void finalMethod() {
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test actions`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/accounts/Actions.java:7: error: Intent action constant name must be ACTION_FOO: ACCOUNT_ADDED [IntentName] [See https://s.android.com/api-guidelines#use-standard-prefixes-for-constants]
                src/android/accounts/Actions.java:6: error: Inconsistent action value; expected `android.accounts.action.ACCOUNT_OPENED`, was `android.accounts.ACCOUNT_OPENED` [ActionValue]
                src/android/accounts/Actions.java:8: error: Intent action constant name must be ACTION_FOO: SOMETHING [IntentName] [See https://s.android.com/api-guidelines#use-standard-prefixes-for-constants]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.accounts;

                    public class Actions {
                        private Actions() { }
                        public static final String ACTION_ACCOUNT_REMOVED = "android.accounts.action.ACCOUNT_REMOVED";
                        public static final String ACTION_ACCOUNT_OPENED = "android.accounts.ACCOUNT_OPENED";
                        public static final String ACCOUNT_ADDED = "android.accounts.action.ACCOUNT_ADDED";
                        public static final String SOMETHING = "android.accounts.action.ACCOUNT_MOVED";
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test extras`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/accounts/Extras.java:5: error: Inconsistent extra value; expected `android.accounts.extra.AUTOMATIC_RULE_ID`, was `android.app.extra.AUTOMATIC_RULE_ID` [ActionValue]
                src/android/accounts/Extras.java:7: error: Intent extra constant name must be EXTRA_FOO: RULE_ID [IntentName] [See https://s.android.com/api-guidelines#use-standard-prefixes-for-constants]
                src/android/accounts/Extras.java:6: error: Intent extra constant name must be EXTRA_FOO: SOMETHING_EXTRA [IntentName] [See https://s.android.com/api-guidelines#use-standard-prefixes-for-constants]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.accounts;

                    public class Extras {
                        private Extras() { }
                        public static final String EXTRA_AUTOMATIC_RULE_ID = "android.app.extra.AUTOMATIC_RULE_ID";
                        public static final String SOMETHING_EXTRA = "something.here";
                        public static final String RULE_ID = "android.app.extra.AUTOMATIC_RULE_ID";
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Test equals and hashCode`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MissingEquals.java:4: error: Must override both equals and hashCode; missing one in android.pkg.MissingEquals [EqualsAndHashCode] [See https://s.android.com/api-guidelines#equals-and-hashcode]
                src/android/pkg/MissingHashCode.java:7: error: Must override both equals and hashCode; missing one in android.pkg.MissingHashCode [EqualsAndHashCode] [See https://s.android.com/api-guidelines#equals-and-hashcode]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    @SuppressWarnings("EqualsWhichDoesntCheckParameterClass")
                    public class Ok {
                        public boolean equals(@Nullable Object other) { return true; }
                        public int hashCode() { return 0; }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MissingEquals {
                        public int hashCode() { return 0; }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    @SuppressWarnings("EqualsWhichDoesntCheckParameterClass")
                    public class MissingHashCode {
                        public boolean equals(@Nullable Object other) { return true; }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class UnrelatedEquals {
                        @SuppressWarnings("EqualsWhichDoesntCheckParameterClass")
                        public static boolean equals(@Nullable Object other) { return true; } // static
                        public boolean equals(int other) { return false; } // wrong parameter type
                        public boolean equals(@Nullable Object other, int bar) { return false; } // wrong signature
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Test Parcelable`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MissingCreator.java:5: error: Parcelable requires a `CREATOR` field; missing in android.pkg.MissingCreator [ParcelCreator] [See https://s.android.com/api-guidelines#parcelable-creator]
                src/android/pkg/MissingDescribeContents.java:5: error: Parcelable requires `public int describeContents()`; missing in android.pkg.MissingDescribeContents [ParcelCreator] [See https://s.android.com/api-guidelines#parcelable-creator]
                src/android/pkg/MissingWriteToParcel.java:5: error: Parcelable requires `void writeToParcel(Parcel, int)`; missing in android.pkg.MissingWriteToParcel [ParcelCreator] [See https://s.android.com/api-guidelines#parcelable-creator]
                src/android/pkg/NonFinalParcelable.java:5: error: Parcelable classes must be final: android.pkg.NonFinalParcelable is not final [ParcelNotFinal] [See https://s.android.com/api-guidelines#parcelable-final]
                src/android/pkg/ParcelableConstructor.java:6: error: Parcelable inflation is exposed through CREATOR, not raw constructors, in android.pkg.ParcelableConstructor [ParcelConstructor] [See https://s.android.com/api-guidelines#parcelable-creator]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public final class ParcelableConstructor implements android.os.Parcelable {
                        public ParcelableConstructor(@Nullable android.os.Parcel p) { }
                        public int describeContents() { return 0; }
                        public void writeToParcel(@Nullable android.os.Parcel p, int f) { }
                        @Nullable
                        public static final android.os.Parcelable.Creator<android.app.WallpaperColors> CREATOR = null;
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class NonFinalParcelable implements android.os.Parcelable {
                        public NonFinalParcelable() { }
                        public int describeContents() { return 0; }
                        public void writeToParcel(@Nullable android.os.Parcel p, int f) { }
                        @Nullable
                        public static final android.os.Parcelable.Creator<android.app.WallpaperColors> CREATOR = null;
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public final class MissingCreator implements android.os.Parcelable {
                        public MissingCreator() { }
                        public int describeContents() { return 0; }
                        public void writeToParcel(@Nullable android.os.Parcel p, int f) { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public final class MissingDescribeContents implements android.os.Parcelable {
                        public MissingDescribeContents() { }
                        public void writeToParcel(@Nullable android.os.Parcel p, int f) { }
                        @Nullable
                        public static final android.os.Parcelable.Creator<android.app.WallpaperColors> CREATOR = null;
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public final class MissingWriteToParcel implements android.os.Parcelable {
                        public MissingWriteToParcel() { }
                        public int describeContents() { return 0; }
                        @Nullable
                        public static final android.os.Parcelable.Creator<android.app.WallpaperColors> CREATOR = null;
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `No protected methods or fields are allowed`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:6: error: Protected methods not allowed; must be public: method android.pkg.MyClass.wrong()} [ProtectedMember] [See https://s.android.com/api-guidelines#avoid-protected]
                src/android/pkg/MyClass.java:8: error: Protected fields not allowed; must be public: field android.pkg.MyClass.wrong} [ProtectedMember] [See https://s.android.com/api-guidelines#avoid-protected]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyClass implements AutoCloseable {
                        public void ok() { }
                        protected void finalize() { } // OK
                        protected void wrong() { }
                        public final int ok = 42;
                        protected final int wrong = 5;
                        private int ok2 = 2;
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Fields must be final and properly named`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:11: error: Non-static field ALSO_BAD_CONSTANT must be named using fooBar style [StartWithLower] [See https://s.android.com/api-guidelines#style-conventions]
                src/android/pkg/MyClass.java:11: error: Constant ALSO_BAD_CONSTANT must be marked static final [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                src/android/pkg/MyClass.java:7: error: Non-static field AlsoBadName must be named using fooBar style [StartWithLower] [See https://s.android.com/api-guidelines#style-conventions]
                src/android/pkg/MyClass.java:10: error: Bare field BAD_CONSTANT must be marked final, or moved behind accessors if mutable [MutableBareField] [See https://s.android.com/api-guidelines#mutable-bare-field]
                src/android/pkg/MyClass.java:10: error: Constant BAD_CONSTANT must be marked static final [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                src/android/pkg/MyClass.java:5: error: Bare field badMutable must be marked final, or moved behind accessors if mutable [MutableBareField] [See https://s.android.com/api-guidelines#mutable-bare-field]
                src/android/pkg/MyClass.java:9: error: Bare field badStaticMutable must be marked final, or moved behind accessors if mutable [MutableBareField] [See https://s.android.com/api-guidelines#mutable-bare-field]
                src/android/pkg/MyClass.java:6: error: Internal field mBadName must not be exposed [InternalField] [See https://s.android.com/api-guidelines#internal-fields]
                src/android/pkg/MyClass.java:8: error: Constant field names must be named with only upper case characters: `android.pkg.MyClass#sBadStaticName`, should be `S_BAD_STATIC_NAME`? [AllUpper] [See https://s.android.com/api-guidelines#constant-naming]
                src/android/pkg/MyClass.java:8: error: Internal field sBadStaticName must not be exposed [InternalField] [See https://s.android.com/api-guidelines#internal-fields]
                src/android/pkg/MyClass.java:15: error: Internal field mBad must not be exposed [InternalField] [See https://s.android.com/api-guidelines#internal-fields]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass {
                        private int mOk;
                        public int badMutable;
                        public final int mBadName;
                        public final int AlsoBadName;
                        public static final int sBadStaticName;
                        public static int badStaticMutable;
                        public static int BAD_CONSTANT;
                        public final int ALSO_BAD_CONSTANT;

                        public static class LayoutParams {
                            public int ok;
                            public int mBad;
                        }
                    }
                    """
                ),
                kotlin(
                    """
                    package android.pkg

                    class KotlinClass(val ok: Int) {
                        companion object {
                            const val OkConstant = 1
                            const val OK_CONSTANT = 2
                            @JvmField
                            val OkSingleton = KotlinClass(3)
                            @JvmField
                            val OK_SINGLETON = KotlinClass(4)
                        }

                        object OkObject
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Only android_net_Uri allowed`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:7: error: Use android.net.Uri instead of java.net.URL (method android.pkg.MyClass.bad1()) [AndroidUri] [See https://s.android.com/api-guidelines#android-uri]
                src/android/pkg/MyClass.java:8: error: Use android.net.Uri instead of java.net.URI (parameter param in android.pkg.MyClass.bad2(java.util.List<java.net.URI> param)) [AndroidUri] [See https://s.android.com/api-guidelines#android-uri]
                src/android/pkg/MyClass.java:9: error: Use android.net.Uri instead of android.net.URL (parameter param in android.pkg.MyClass.bad3(android.net.URL param)) [AndroidUri] [See https://s.android.com/api-guidelines#android-uri]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import java.util.List;import java.util.concurrent.CompletableFuture;import java.util.concurrent.Future;
                    import androidx.annotation.NonNull;

                    public final class MyClass {
                        public @NonNull java.net.URL bad1() { throw new RuntimeException(); }
                        public void bad2(@NonNull List<java.net.URI> param) { }
                        public void bad3(@NonNull android.net.URL param) { }
                    }
                    """
                ),
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `CompletableFuture and plain Future not allowed`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:9: error: Use ListenableFuture (library), or a combination of Consumer<T>, Executor, and CancellationSignal (platform) instead of java.util.concurrent.CompletableFuture (method android.pkg.MyClass.bad1()) [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:10: error: Use ListenableFuture (library), or a combination of Consumer<T>, Executor, and CancellationSignal (platform) instead of java.util.concurrent.CompletableFuture (parameter param in android.pkg.MyClass.bad2(java.util.concurrent.CompletableFuture<java.lang.String> param)) [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:11: error: Use ListenableFuture (library), or a combination of Consumer<T>, Executor, and CancellationSignal (platform) instead of java.util.concurrent.Future (method android.pkg.MyClass.bad3()) [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:12: error: Use ListenableFuture (library), or a combination of Consumer<T>, Executor, and CancellationSignal (platform) instead of java.util.concurrent.Future (parameter param in android.pkg.MyClass.bad4(java.util.concurrent.Future<java.lang.String> param)) [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:21: error: BadCompletableFuture should not extend `java.util.concurrent.CompletableFuture`. In AndroidX, use (but do not extend) ListenableFuture. In platform, use a combination of Consumer<T>, Executor, and CancellationSignal`. [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:17: error: BadFuture should not extend `java.util.concurrent.Future`. In AndroidX, use (but do not extend) ListenableFuture. In platform, use a combination of Consumer<T>, Executor, and CancellationSignal`. [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                src/android/pkg/MyClass.java:19: error: BadFutureClass should not implement `java.util.concurrent.Future`. In AndroidX, use (but do not extend) ListenableFuture. In platform, use a combination of Consumer<T>, Executor, and CancellationSignal`. [BadFuture] [See https://s.android.com/api-guidelines#bad-future]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import java.util.concurrent.CompletableFuture;
                    import java.util.concurrent.Future;
                    import androidx.annotation.Nullable;
                    import com.google.common.util.concurrent.ListenableFuture;

                    public final class MyClass {
                        public @Nullable CompletableFuture<String> bad1() { return null; }
                        public void bad2(@Nullable CompletableFuture<String> param) { }
                        public @Nullable Future<String> bad3() { return null; }
                        public void bad4(@Nullable Future<String> param) { }

                        public @Nullable ListenableFuture<String> ok1() { return null; }
                        public void ok2(@Nullable ListenableFuture<String> param) { }

                        public interface BadFuture<T> extends Future<T> {
                        }
                        public static abstract class BadFutureClass<T> implements Future<T> {
                        }
                        public class BadCompletableFuture<T> extends CompletableFuture<T> {
                        }
                    }
                    """
                ),
                java(
                    """
                    package com.google.common.util.concurrent;
                    public class ListenableFuture<T> {
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Typedef must be hidden`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:15: error: Don't expose @IntDef: SomeInt must be hidden. [PublicTypedef] [See https://s.android.com/api-guidelines#no-public-typedefs]
                src/android/pkg/MyClass.java:20: error: Don't expose @LongDef: SomeLong must be hidden. [PublicTypedef] [See https://s.android.com/api-guidelines#no-public-typedefs]
                src/android/pkg/MyClass.java:10: error: Don't expose @StringDef: SomeString must be hidden. [PublicTypedef] [See https://s.android.com/api-guidelines#no-public-typedefs]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public final class MyClass {
                            private MyClass() {}

                            public static final String SOME_STRING = "abc";
                            public static final int SOME_INT = 1;
                            public static final long SOME_LONG = 1L;

                            @android.annotation.StringDef(value = {
                                    SOME_STRING
                            })
                            @Retention(RetentionPolicy.SOURCE)
                            public @interface SomeString {}
                            @android.annotation.IntDef(value = {
                                    SOME_INT
                            })
                            @Retention(RetentionPolicy.SOURCE)
                            public @interface SomeInt {}
                            @android.annotation.LongDef(value = {
                                    SOME_LONG
                            })
                            @Retention(RetentionPolicy.SOURCE)
                            public @interface SomeLong {}
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Ensure registration methods are matched`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/RegistrationInterface.java:6: error: Found registerOverriddenUnpairedCallback but not unregisterOverriddenUnpairedCallback in android.pkg.RegistrationInterface [PairedRegistration] [See https://s.android.com/api-guidelines#callbacks-symmetry]
                src/android/pkg/RegistrationMethods.java:8: error: Found registerUnpairedCallback but not unregisterUnpairedCallback in android.pkg.RegistrationMethods [PairedRegistration] [See https://s.android.com/api-guidelines#callbacks-symmetry]
                src/android/pkg/RegistrationMethods.java:12: error: Found unregisterMismatchedCallback but not registerMismatchedCallback in android.pkg.RegistrationMethods [PairedRegistration] [See https://s.android.com/api-guidelines#callbacks-symmetry]
                src/android/pkg/RegistrationMethods.java:13: error: Callback methods should be named register/unregister; was addCallback [RegistrationName] [See https://s.android.com/api-guidelines#callbacks-accessors]
                src/android/pkg/RegistrationMethods.java:18: error: Found addUnpairedListener but not removeUnpairedListener in android.pkg.RegistrationMethods [PairedRegistration] [See https://s.android.com/api-guidelines#callbacks-symmetry]
                src/android/pkg/RegistrationMethods.java:19: error: Found removeMismatchedListener but not addMismatchedListener in android.pkg.RegistrationMethods [PairedRegistration] [See https://s.android.com/api-guidelines#callbacks-symmetry]
                src/android/pkg/RegistrationMethods.java:20: error: Listener methods should be named add/remove; was registerWrongListener [RegistrationName] [See https://s.android.com/api-guidelines#callbacks-accessors]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class RegistrationMethods implements RegistrationInterface {
                        public void registerOkCallback(@Nullable Runnable r) { } // OK
                        public void unregisterOkCallback(@Nullable Runnable r) { } // OK
                        public void registerUnpairedCallback(@Nullable Runnable r) { }
                        // OK here because it is override
                        @Override
                        public void registerOverriddenUnpairedCallback(@Nullable Runnable r) { }
                        public void unregisterMismatchedCallback(@Nullable Runnable r) { }
                        public void addCallback(@Nullable Runnable r) { }

                        public void addOkListener(@Nullable Runnable r) { } // OK
                        public void removeOkListener(@Nullable Runnable r) { } // OK

                        public void addUnpairedListener(@Nullable Runnable r) { }
                        public void removeMismatchedListener(@Nullable Runnable r) { }
                        public void registerWrongListener(@Nullable Runnable r) { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public interface RegistrationInterface {
                        void registerOverriddenUnpairedCallback(@Nullable Runnable r) { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Api methods should not be synchronized in their signature`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/CheckSynchronization.java:12: error: Internal locks must not be exposed: method android.pkg.CheckSynchronization.errorMethod1(Runnable) [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization.java:14: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization.errorMethod2() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization.java:18: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization.errorMethod2() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization.java:23: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization.errorMethod3() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization2.kt:5: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization2.errorMethod1() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization2.kt:8: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization2.errorMethod2() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization2.kt:13: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization2.errorMethod3() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization2.kt:16: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization2.errorMethod4() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                src/android/pkg/CheckSynchronization2.kt:18: error: Internal locks must not be exposed (synchronizing on this or class is still externally observable): method android.pkg.CheckSynchronization2.errorMethod5() [VisiblySynchronized] [See https://s.android.com/api-guidelines#avoid-synchronized]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class CheckSynchronization {
                        public void okMethod1(@Nullable Runnable r) { }
                        private static final Object LOCK = new Object();
                        public void okMethod2() {
                            synchronized(LOCK) {
                            }
                        }
                        public synchronized void errorMethod1(@Nullable Runnable r) { } // ERROR
                        public void errorMethod2() {
                            synchronized(this) {
                            }
                        }
                        public void errorMethod2() {
                            synchronized(CheckSynchronization.class) {
                            }
                        }
                        public void errorMethod3() {
                            if (true) {
                                synchronized(CheckSynchronization.class) {
                                }
                            }
                        }
                    }
                    """
                ),
                kotlin(
                    """
                    package android.pkg

                    class CheckSynchronization2 {
                        fun errorMethod1() {
                            synchronized(this) { println("hello") }
                        }
                        fun errorMethod2() {
                            synchronized(CheckSynchronization2::class.java) { println("hello") }
                        }
                        fun errorMethod3() {
                            @Suppress("ConstantConditionIf")
                            if (true) {
                                synchronized(CheckSynchronization2::class.java) { println("hello") }
                            }
                        }
                        fun errorMethod4() = synchronized(this) { println("hello") }
                        fun errorMethod5() {
                            synchronized(CheckSynchronization2::class) { println("hello") }
                        }
                        fun okMethod() {
                            val lock = Object()
                            synchronized(lock) { println("hello") }
                        }
                    }
                    """
                ),
                androidxNullableSource,
                nullableSource
            )
        )
    }

    @Test
    fun `Check intent builder names`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/IntentBuilderNames.java:8: warning: Methods creating an Intent should be named `create<Foo>Intent()`, was `makeMyIntent` [IntentBuilderName] [See https://s.android.com/api-guidelines#intent-builder-createintent]
                src/android/pkg/IntentBuilderNames.java:10: warning: Methods creating an Intent should be named `create<Foo>Intent()`, was `createIntentNow` [IntentBuilderName] [See https://s.android.com/api-guidelines#intent-builder-createintent]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.content.Intent;
                    import androidx.annotation.Nullable;

                    public class IntentBuilderNames {
                        @Nullable
                        public Intent createEnrollIntent() { return null; } // OK
                        @Nullable
                        public Intent makeMyIntent() { return null; } // WARN
                        @Nullable
                        public Intent createIntentNow() { return null; } // WARN
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check helper classes`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass1.java:3: error: Inconsistent class name; should be `<Foo>Activity`, was `MyClass1` [ContextNameSuffix] [See https://s.android.com/api-guidelines#classes-subclass-naming]
                src/android/pkg/MyClass1.java:6: warning: Methods implemented by developers should follow the on<Something> style, was `badlyNamedAbstractMethod` [OnNameExpected] [See https://s.android.com/api-guidelines#callback-method-naming]
                src/android/pkg/MyClass1.java:7: warning: If implemented by developer, should follow the on<Something> style; otherwise consider marking final [OnNameExpected] [See https://s.android.com/api-guidelines#callback-method-naming]
                src/android/pkg/MyClass1.java:3: error: MyClass1 should not extend `Activity`. Activity subclasses are impossible to compose. Expose a composable API instead. [ForbiddenSuperClass]
                src/android/pkg/MyClass2.java:3: error: Inconsistent class name; should be `<Foo>Provider`, was `MyClass2` [ContextNameSuffix] [See https://s.android.com/api-guidelines#classes-subclass-naming]
                src/android/pkg/MyClass3.java:3: error: Inconsistent class name; should be `<Foo>Service`, was `MyClass3` [ContextNameSuffix] [See https://s.android.com/api-guidelines#classes-subclass-naming]
                src/android/pkg/MyClass4.java:3: error: Inconsistent class name; should be `<Foo>Receiver`, was `MyClass4` [ContextNameSuffix] [See https://s.android.com/api-guidelines#classes-subclass-naming]
                src/android/pkg/MyOkActivity.java:3: error: MyOkActivity should not extend `Activity`. Activity subclasses are impossible to compose. Expose a composable API instead. [ForbiddenSuperClass]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass1 extends android.app.Activity {
                        public void onOk() { }
                        public final void ok() { }
                        public abstract void badlyNamedAbstractMethod();
                        public void badlyNamedMethod() { }
                        public static void staticOk() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyClass2 extends android.content.ContentProvider {
                        public static final String PROVIDER_INTERFACE = "android.pkg.MyClass2";
                        public final void ok();
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyClass3 extends android.app.Service {
                        public static final String SERVICE_INTERFACE = "android.pkg.MyClass3";
                        public final void ok();
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyClass4 extends android.content.BroadcastReceiver {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyOkActivity extends android.app.Activity {
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check builders`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/Bad.java:12: warning: Builder must be final: android.pkg.Bad.BadBuilder [StaticFinalBuilder] [See https://s.android.com/api-guidelines#builders-static-inner]
                src/android/pkg/Bad.java:12: warning: Builder must be static: android.pkg.Bad.BadBuilder [StaticFinalBuilder] [See https://s.android.com/api-guidelines#builders-static-inner]
                src/android/pkg/Bad.java:13: warning: Builder constructor arguments must be mandatory (i.e. not @Nullable): parameter badParameter in android.pkg.Bad.BadBuilder(String badParameter) [OptionalBuilderConstructorArgument] [See https://s.android.com/api-guidelines#builders-nonnull-constructors]
                src/android/pkg/Bad.java:36: warning: Builder methods names should use setFoo() / addFoo() / clearFoo() style: method android.pkg.Bad.BadBuilder.withBadSetterStyle(boolean) [BuilderSetStyle] [See https://s.android.com/api-guidelines#builder-method-naming]
                src/android/pkg/Bad.java:39: warning: Builder setter must be @NonNull: method android.pkg.Bad.BadBuilder.setReturnsNullable(boolean) [SetterReturnsThis]
                src/android/pkg/Bad.java:42: warning: Getter should be on the built object, not the builder: method android.pkg.Bad.BadBuilder.getOnBuilder() [GetterOnBuilder] [See https://s.android.com/api-guidelines#getter-on-builder]
                src/android/pkg/Bad.java:44: warning: Methods must return the builder object (return type android.pkg.Bad.BadBuilder instead of void): method android.pkg.Bad.BadBuilder.setNotReturningBuilder(boolean) [SetterReturnsThis]
                src/android/pkg/Bad.java:18: warning: android.pkg.Bad does not declare a `getWithoutMatchingGetters()` method matching method android.pkg.Bad.BadBuilder.addWithoutMatchingGetter(String) [MissingGetterMatchingBuilder] [See https://s.android.com/api-guidelines#builders-symmetric-setters]
                src/android/pkg/Bad.java:21: warning: android.pkg.Bad does not declare a `isWithoutMatchingGetter()` method matching method android.pkg.Bad.BadBuilder.setWithoutMatchingGetter(boolean) [MissingGetterMatchingBuilder] [See https://s.android.com/api-guidelines#builders-symmetric-setters]
                src/android/pkg/Bad.java:24: warning: android.pkg.Bad does not declare a `getPluralWithoutMatchingGetters()` method matching method android.pkg.Bad.BadBuilder.addPluralWithoutMatchingGetter(Collection<String>) [MissingGetterMatchingBuilder] [See https://s.android.com/api-guidelines#builders-symmetric-setters]
                src/android/pkg/Bad.java:30: warning: android.pkg.Bad does not declare a getter method matching method android.pkg.Bad.BadBuilder.addPluralWithoutMatchingGetters(Collection<String>) (expected one of: [getPluralWithoutMatchingGetters(), getPluralWithoutMatchingGetterses()]) [MissingGetterMatchingBuilder] [See https://s.android.com/api-guidelines#builders-symmetric-setters]
                src/android/pkg/Bad.java:44: warning: android.pkg.Bad does not declare a `isNotReturningBuilder()` method matching method android.pkg.Bad.BadBuilder.setNotReturningBuilder(boolean) [MissingGetterMatchingBuilder] [See https://s.android.com/api-guidelines#builders-symmetric-setters]
                src/android/pkg/Bad.java:55: warning: Methods must return the builder object (return type android.pkg.Bad.BadGenericBuilder<T> instead of T): method android.pkg.Bad.BadGenericBuilder.setBoolean(boolean) [SetterReturnsThis]
                src/android/pkg/Bad.java:50: warning: android.pkg.Bad.NoBuildMethodBuilder does not declare a `build()` method, but builder classes are expected to [MissingBuildMethod] [See https://s.android.com/api-guidelines#builder-must-declare-build]
                src/android/pkg/TopLevelBuilder.java:3: warning: Builder should be defined as inner class: android.pkg.TopLevelBuilder [TopLevelBuilder] [See https://s.android.com/api-guidelines#builders-static-inner]
                src/android/pkg/TopLevelBuilder.java:3: warning: android.pkg.TopLevelBuilder does not declare a `build()` method, but builder classes are expected to [MissingBuildMethod] [See https://s.android.com/api-guidelines#builder-must-declare-build]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public final class TopLevelBuilder {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    public class Ok {

                        public int getInt();
                        @NonNull
                        public List<String> getStrings();
                        @NonNull
                        public List<String> getProperties();
                        @NonNull
                        public List<String> getRays();
                        @NonNull
                        public List<String> getBuses();
                        @NonNull
                        public List<String> getTaxes();
                        @NonNull
                        public List<String> getMessages();
                        public boolean isBoolean();
                        public boolean hasBoolean2();
                        public boolean shouldBoolean3();

                        public static final class OkBuilder {
                            public OkBuilder(@NonNull String goodParameter, int goodParameter2) {}

                            @NonNull
                            public Ok build() { return null; }

                            @NonNull
                            public OkBuilder setInt(int value) { return this; }

                            @NonNull
                            public OkBuilder addString(@NonNull String value) { return this; }

                            @NonNull
                            public OkBuilder addProperty(@NonNull String value) { return this; }

                            @NonNull
                            public OkBuilder addRay(@NonNull String value) { return this; }

                            @NonNull
                            public OkBuilder addBus(@NonNull String value) { return this; }

                            @NonNull
                            public OkBuilder addTax(@NonNull String value) { return this; }

                            @NonNull
                            public OkBuilder addMessages(@NonNull Collection<String> value) {
                                return this;
                            }

                            @NonNull
                            public OkBuilder clearStrings() { return this; }

                            @NonNull
                            public OkBuilder setBoolean(boolean v) { return this; }

                            @NonNull
                            public OkBuilder setHasBoolean2(boolean v) { return this; }

                            @NonNull
                            public OkBuilder setShouldBoolean3(boolean v) { return this; }

                            @NonNull
                            public OkBuilder clear() { return this; }

                            @NonNull
                            public OkBuilder clearAll() { return this; }
                        }

                        public static final class GenericBuilder<B extends GenericBuilder> {
                            @NonNull
                            public B setBoolean(boolean value) { return this; }

                            @NonNull
                            public Ok build() { return null; }
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class SubOk extends Ok {

                        public static final class Builder {
                            public Builder() {}

                            @NonNull
                            public SubOk build() { return null; }

                            @NonNull
                            public Builder setInt(int value) { return this; }
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    public class Bad {

                        public boolean isBoolean();
                        public boolean getWithoutMatchingGetter();
                        public boolean isReturnsNullable();

                        public class BadBuilder {
                            public BadBuilder(@Nullable String badParameter) {}

                            @NonNull
                            public Bad build() { return null; }

                            @NonNull
                            public BadBuilder addWithoutMatchingGetter(@NonNull String value) { return this; }

                            @NonNull
                            public BadBuilder setWithoutMatchingGetter(boolean v) { return this; }

                            @NonNull
                            public BadBuilder addPluralWithoutMatchingGetter(
                                @NonNull Collection<String> value) {
                                return this;
                            }

                            @NonNull
                            public BadBuilder addPluralWithoutMatchingGetters(
                                @NonNull Collection<String> value) {
                                return this;
                            }

                            @NonNull
                            public BadBuilder withBadSetterStyle(boolean v) { return this; }

                            @Nullable
                            public BadBuilder setReturnsNullable(boolean v) { return this; }

                            public boolean getOnBuilder() { return true; }

                            public void setNotReturningBuilder(boolean v) { return this; }

                            @NonNull
                            public BadBuilder () { return this; }
                        }

                        public static final class NoBuildMethodBuilder {
                            public NoBuildMethodBuilder() {}
                        }

                        public static final class BadGenericBuilder<T extends Bad> {
                            @NonNull
                            public T setBoolean(boolean value) { return this; }

                            @NonNull
                            public Bad build() { return null; }
                        }
                    }
                    """
                ),
                androidxNonNullSource,
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check suppress works on inherited methods`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                warning: Should avoid odd sized primitives; use `int` instead of `short` in method android.pkg.Ok.Public.shouldFail(PublicT) [NoByteOrShort] [See https://s.android.com/api-guidelines#avoid-short-byte]
                """,
            sourceFiles = arrayOf(
                java(
                """
                package android.pkg;

                import androidx.annotation.NonNull;

                public class Ok {

                    static final class Hidden<HiddenT> {
                        @SuppressWarnings("NoByteOrShort")
                        public short suppressed(HiddenT t) { return null; }

                        public short shouldFail(HiddenT t) { return null; }
                    }

                    public static final class Public<PublicT> extends Hidden<PublicT> {
                    }
                }
                """
                ),
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `Raw AIDL`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass1.java:3: error: Raw AIDL interfaces must not be exposed: MyClass1 extends Binder [RawAidl] [See https://s.android.com/api-guidelines#no-public-binder]
                src/android/pkg/MyClass2.java:3: error: Raw AIDL interfaces must not be exposed: MyClass2 implements IInterface [RawAidl] [See https://s.android.com/api-guidelines#no-public-binder]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyClass1 extends android.os.Binder {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public abstract class MyClass2 implements android.os.IInterface {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;
                    // Ensure that we don't flag transitively implementing IInterface
                    public class MyClass3 extends MyClass1 implements MyClass2 {
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Internal packages`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/com/android/pkg/MyClass.java:3: error: Internal classes must not be exposed [InternalClasses]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package com.android.pkg;

                    public class MyClass {
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check package layering`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/content/MyClass1.java:8: warning: Field type `android.view.View` violates package layering: nothing in `package android.content` should depend on `package android.view` [PackageLayering]
                src/android/content/MyClass1.java:8: warning: Method return type `android.view.View` violates package layering: nothing in `package android.content` should depend on `package android.view` [PackageLayering]
                src/android/content/MyClass1.java:8: warning: Method parameter type `android.view.View` violates package layering: nothing in `package android.content` should depend on `package android.view` [PackageLayering]
                src/android/content/MyClass1.java:8: warning: Method parameter type `android.view.View` violates package layering: nothing in `package android.content` should depend on `package android.view` [PackageLayering]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.content;

                    import android.graphics.drawable.Drawable;
                    import android.graphics.Bitmap;
                    import android.view.View;
                    import androidx.annotation.Nullable;

                    public class MyClass1 {
                        @Nullable
                        public final View view = null;
                        @Nullable
                        public final Drawable drawable = null;
                        @Nullable
                        public final Bitmap bitmap = null;
                        @Nullable
                        public View ok(@Nullable View view, @Nullable Drawable drawable) { return null; }
                        @Nullable
                        public Bitmap wrong(@Nullable View view, @Nullable Bitmap bitmap) { return null; }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check boolean getter and setter naming patterns`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                    src/android/pkg/MyClass.java:20: error: Symmetric method for `isVisibleBad` must be named `setVisibleBad`; was `setIsVisibleBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:24: error: Symmetric method for `hasTransientStateBad` must be named `setHasTransientStateBad`; was `setTransientStateBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:28: error: Symmetric method for `setHasTransientStateAlsoBad` must be named `hasTransientStateAlsoBad`; was `isHasTransientStateAlsoBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:31: error: Symmetric method for `setCanRecordBad` must be named `canRecordBad`; was `isCanRecordBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:34: error: Symmetric method for `setShouldFitWidthBad` must be named `shouldFitWidthBad`; was `isShouldFitWidthBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:37: error: Symmetric method for `setWiFiRoamingSettingEnabledBad` must be named `isWiFiRoamingSettingEnabledBad`; was `getWiFiRoamingSettingEnabledBad` [GetterSetterNames]
                    src/android/pkg/MyClass.java:40: error: Symmetric method for `setEnabledBad` must be named `isEnabledBad`; was `getEnabledBad` [GetterSetterNames]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass {
                        // Correct
                        public void setVisible(boolean visible) {}
                        public boolean isVisible() { return false; }

                        public void setHasTransientState(boolean hasTransientState) {}
                        public boolean hasTransientState() { return false; }

                        public void setCanRecord(boolean canRecord) {}
                        public boolean canRecord() { return false; }

                        public void setShouldFitWidth(boolean shouldFitWidth) {}
                        public boolean shouldFitWidth() { return false; }

                        public void setWiFiRoamingSettingEnabled(boolean enabled) {}
                        public boolean isWiFiRoamingSettingEnabled() { return false; }

                        // Bad
                        public void setIsVisibleBad(boolean visible) {}
                        public boolean isVisibleBad() { return false; }

                        public void setTransientStateBad(boolean hasTransientState) {}
                        public boolean hasTransientStateBad() { return false; }

                        public void setHasTransientStateAlsoBad(boolean hasTransientState) {}
                        public boolean isHasTransientStateAlsoBad() { return false; }

                        public void setCanRecordBad(boolean canRecord) {}
                        public boolean isCanRecordBad() { return false; }

                        public void setShouldFitWidthBad(boolean shouldFitWidth) {}
                        public boolean isShouldFitWidthBad() { return false; }

                        public void setWiFiRoamingSettingEnabledBad(boolean enabled) {}
                        public boolean getWiFiRoamingSettingEnabledBad() { return false; }

                        public void setEnabledBad(boolean enabled) {}
                        public boolean getEnabledBad() { return false; }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check banned collections`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:6: error: Parameter type is concrete collection (`java.util.HashMap`); must be higher-level interface [ConcreteCollection] [See https://s.android.com/api-guidelines#classes-collections]
                src/android/pkg/MyClass.java:9: error: Return type is concrete collection (`java.util.Vector`); must be higher-level interface [ConcreteCollection] [See https://s.android.com/api-guidelines#classes-collections]
                src/android/pkg/MyClass.java:10: error: Parameter type is concrete collection (`java.util.LinkedList`); must be higher-level interface [ConcreteCollection] [See https://s.android.com/api-guidelines#classes-collections]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;

                    public class MyClass {
                        public MyClass(@NonNull java.util.HashMap<String,String> map1,
                                @NonNull java.util.Map<String,String> map2) {
                        }
                        @NonNull
                        public java.util.Vector<String> getList(@NonNull java.util.LinkedList<String> list) {
                            throw new RuntimeException();
                        }
                    }
                    """
                ),
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `Check nullable collections`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyCallback.java:4: warning: Type of parameter list in android.pkg.MyCallback.onFoo(java.util.List<java.lang.String> list) is a nullable collection (`java.util.List`); must be non-null [NullableCollection] [See https://s.android.com/api-guidelines#methods-prefer-non-null-collections]
                src/android/pkg/MyClass.java:8: warning: Return type of method android.pkg.MyClass.getList(java.util.List<java.lang.String>) is a nullable collection (`java.util.List`); must be non-null [NullableCollection] [See https://s.android.com/api-guidelines#methods-prefer-non-null-collections]
                src/android/pkg/MyClass.java:12: warning: Type of field android.pkg.MyClass.STRINGS is a nullable collection (`java.lang.String[]`); must be non-null [NullableCollection] [See https://s.android.com/api-guidelines#methods-prefer-non-null-collections]
                src/android/pkg/MySubClass.java:12: warning: Return type of method android.pkg.MySubClass.getOtherList(java.util.List<java.lang.String>) is a nullable collection (`java.util.List`); must be non-null [NullableCollection] [See https://s.android.com/api-guidelines#methods-prefer-non-null-collections]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class MyClass {
                        public MyClass() { }

                        @Nullable
                        public java.util.List<String> getList(@Nullable java.util.List<String> list) {
                            return null;
                        }
                        @Nullable
                        public static final String[] STRINGS = null;

                        /** @deprecated don't use this. */
                        @Deprecated
                        @Nullable
                        public String[] ignoredBecauseDeprecated(@Nullable String[] ignored) {
                            return null;
                        }

                        protected MyClass() {
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    /** @hide */
                    public interface MyHiddenInterface {
                        @Nullable
                        java.util.List<String> getOtherList(@Nullable java.util.List<String> list);
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class MySubClass extends MyClass implements MyHiddenInterface {
                        @Nullable
                        public java.util.List<String> getList(@Nullable java.util.List<String> list) {
                            // Ignored because it has the same nullability as its super method
                            return null;
                        }

                        @Override
                        @Nullable
                        public java.util.List<String> getOtherList(@Nullable java.util.List<String> list) {
                            // Reported because the super method is hidden.
                            return null;
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyCallback {
                        public void onFoo(@Nullable java.util.List<String> list) {
                        }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check non-overlapping flags`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/accounts/OverlappingFlags.java:19: warning: Found overlapping flag constant values: `TEST1_FLAG_SECOND` with value 3 (0x3) and overlapping flag value 1 (0x1) from `TEST1_FLAG_FIRST` [OverlappingConstants] [See https://s.android.com/api-guidelines#overlapping-constants]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.accounts;

                    public class OverlappingFlags {
                        private OverlappingFlags() { }
                        public static final int DRAG_FLAG_GLOBAL_PREFIX_URI_PERMISSION = 128; // 0x80
                        public static final int DRAG_FLAG_GLOBAL_URI_READ = 1; // 0x1
                        public static final int DRAG_FLAG_GLOBAL_URI_WRITE = 2; // 0x2
                        public static final int DRAG_FLAG_OPAQUE = 512; // 0x200
                        public static final int SYSTEM_UI_FLAG_FULLSCREEN = 4; // 0x4
                        public static final int SYSTEM_UI_FLAG_HIDE_NAVIGATION = 2; // 0x2
                        public static final int SYSTEM_UI_FLAG_IMMERSIVE = 2048; // 0x800
                        public static final int SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 4096; // 0x1000
                        public static final int SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 1024; // 0x400
                        public static final int SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 512; // 0x200
                        public static final int SYSTEM_UI_FLAG_LAYOUT_STABLE = 256; // 0x100
                        public static final int SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR = 16; // 0x10

                        public static final int TEST1_FLAG_FIRST = 1;
                        public static final int TEST1_FLAG_SECOND = 3;
                        public static final int TEST2_FLAG_FIRST = 5;
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check exception related issues`() {
        check(
            extraArguments = arrayOf(
                ARG_API_LINT,
                // Conflicting advice:
                ARG_HIDE, "BannedThrow"
            ),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:6: error: Methods must not throw generic exceptions (`java.lang.Exception`) [GenericException] [See https://s.android.com/api-guidelines#appropriate-exception]
                src/android/pkg/MyClass.java:7: error: Methods must not throw generic exceptions (`java.lang.Throwable`) [GenericException] [See https://s.android.com/api-guidelines#appropriate-exception]
                src/android/pkg/MyClass.java:8: error: Methods must not throw generic exceptions (`java.lang.Error`) [GenericException] [See https://s.android.com/api-guidelines#appropriate-exception]
                src/android/pkg/MyClass.java:9: warning: Methods taking no arguments should throw `IllegalStateException` instead of `java.lang.IllegalArgumentException` [IllegalStateException] [See https://s.android.com/api-guidelines#appropriate-exception]
                src/android/pkg/MyClass.java:10: warning: Methods taking no arguments should throw `IllegalStateException` instead of `java.lang.NullPointerException` [IllegalStateException] [See https://s.android.com/api-guidelines#appropriate-exception]
                src/android/pkg/MyClass.java:11: error: Methods calling system APIs should rethrow `RemoteException` as `RuntimeException` (but do not list it in the throws clause) [RethrowRemoteException] [See https://s.android.com/api-guidelines#appropriate-exception]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.os.RemoteException;

                    @SuppressWarnings("RedundantThrows")
                    public class MyClass {
                        public void method1() throws Exception { }
                        public void method2() throws Throwable { }
                        public void method3() throws Error { }
                        public void method4() throws IllegalArgumentException { }
                        public void method4() throws NullPointerException { }
                        public void method5() throws RemoteException { }
                        public void ok(int p) throws NullPointerException { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check no mentions of Google in APIs`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:4: error: Must never reference Google (`MyGoogleService`) [MentionsGoogle] [See https://s.android.com/api-guidelines#mentions-google]
                src/android/pkg/MyClass.java:5: error: Must never reference Google (`callGoogle`) [MentionsGoogle] [See https://s.android.com/api-guidelines#mentions-google]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass {
                        public static class MyGoogleService {
                            public void callGoogle() { }
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check no usages of heavy BitSet`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:8: error: Type must not be heavy BitSet (method android.pkg.MyClass.reverse(java.util.BitSet)) [HeavyBitSet] [See https://s.android.com/api-guidelines#avoid-bitset]
                src/android/pkg/MyClass.java:9: error: Type must not be heavy BitSet (parameter bitset in android.pkg.MyClass.reverse(java.util.BitSet bitset)) [HeavyBitSet] [See https://s.android.com/api-guidelines#avoid-bitset]
                src/android/pkg/MyClass.java:6: error: Type must not be heavy BitSet (field android.pkg.MyClass.bitset) [HeavyBitSet] [See https://s.android.com/api-guidelines#avoid-bitset]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import androidx.annotation.Nullable;
                    import java.util.BitSet;

                    public class MyClass {
                        @Nullable
                        public final BitSet bitset;
                        @Nullable
                        public BitSet reverse(@Nullable BitSet bitset) { return null; }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check Manager related issues`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyFirstManager.java:6: error: Managers must always be obtained from Context; no direct constructors [ManagerConstructor]
                src/android/pkg/MyFirstManager.java:8: error: Managers must always be obtained from Context (`get`) [ManagerLookup]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class MyFirstManager {
                        public MyFirstManager() {
                        }
                        @Nullable
                        public MyFirstManager get() { return null; }
                        @Nullable
                        public MySecondManager ok() { return null; }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MySecondManager {
                        private MySecondManager() {
                        }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check boxed types`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/test/pkg/KotlinClass.kt:4: error: Must avoid boxed primitives (`java.lang.Double`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/KotlinClass.kt:6: error: Must avoid boxed primitives (`java.lang.Boolean`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/MyClass.java:9: error: Must avoid boxed primitives (`java.lang.Long`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/MyClass.java:11: error: Must avoid boxed primitives (`java.lang.Short`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/MyClass.java:12: error: Must avoid boxed primitives (`java.lang.Double`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/MyClass.java:13: error: Must avoid boxed primitives (`java.lang.Boolean`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                src/test/pkg/MyClass.java:6: error: Must avoid boxed primitives (`java.lang.Integer`) [AutoBoxing] [See https://s.android.com/api-guidelines#auto-boxing]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    import androidx.annotation.Nullable;

                    public class MyClass {
                        @Nullable
                        public final Integer integer1;
                        public final int integer2;
                        public MyClass(@Nullable Long l) {
                        }
                        @Nullable
                        public Short getDouble(@Nullable Double l) { return null; }
                        @Nullable
                        public Boolean getBoolean() { return null; }
                    }
                    """
                ),
                kotlin("""
                    package test.pkg
                    class KotlinClass {
                        fun getIntegerOk(): Double { TODO() }
                        fun getIntegerBad(): Double? { TODO() }
                        fun getBooleanOk(): Boolean { TODO() }
                        fun getBooleanBad(): Boolean? { TODO() }
                    }
                """),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check static utilities`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyUtils1.java:3: error: Fully-static utility classes must not have constructor [StaticUtils]
                src/android/pkg/MyUtils2.java:3: error: Fully-static utility classes must not have constructor [StaticUtils]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyUtils1 {
                        // implicit constructor
                        public static void foo() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyUtils2 {
                        public MyUtils2() { }
                        public static void foo() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyUtils3 {
                        private MyUtils3() { }
                        public static void foo() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyUtils4 {
                        // OK: instance method
                        public void foo() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyUtils5 {
                        // OK: instance field
                        public final int foo = 42;
                        public static void foo() { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check context first`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:11: error: Context is distinct, so it must be the first argument (method `wrong`) [ContextFirst]
                src/android/pkg/MyClass.java:12: error: ContentResolver is distinct, so it must be the first argument (method `wrong`) [ContextFirst]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.content.Context;
                    import android.content.ContentResolver;
                    import androidx.annotation.Nullable;

                    public class MyClass {
                        public MyClass(@Nullable Context context1, @Nullable Context context2) {
                        }
                        public void ok(@Nullable ContentResolver resolver, int i) { }
                        public void ok(@Nullable Context context, int i) { }
                        public void wrong(int i, @Nullable Context context) { }
                        public void wrong(int i, @Nullable ContentResolver resolver) { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check listener last`() {
        check(
            extraArguments = arrayOf(ARG_API_LINT, ARG_HIDE, "ExecutorRegistration"),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:7: warning: Listeners should always be at end of argument list (method `MyClass`) [ListenerLast] [See https://s.android.com/api-guidelines#placement-of-sam-parameters]
                src/android/pkg/MyClass.java:10: warning: Listeners should always be at end of argument list (method `wrong`) [ListenerLast] [See https://s.android.com/api-guidelines#placement-of-sam-parameters]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.pkg.MyCallback;
                    import android.content.Context;
                    import androidx.annotation.Nullable;

                    public class MyClass {
                        public MyClass(@Nullable MyCallback listener, int i) {
                        }
                        public void ok(@Nullable Context context, int i, @Nullable MyCallback listener) { }
                        public void wrong(@Nullable MyCallback listener, int i) { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    @SuppressWarnings("WeakerAccess")
                    public abstract class MyCallback {
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check overloaded arguments`() {
        // TODO: This check is not yet hooked up
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass {
                        private MyClass() {
                        }

                        public void name1() { }
                        public void name1(int i) { }
                        public void name1(int i, int j) { }
                        public void name1(int i, int j, int k) { }
                        public void name1(int i, int j, int k, float f) { }

                        public void name2(int i) { }
                        public void name2(int i, int j) { }
                        public void name2(int i, float j, float k) { }
                        public void name2(int i, int j, int k, float f) { }
                        public void name2(int i, float f, int j) { }

                        public void name3() { }
                        public void name3(int i) { }
                        public void name3(int i, int j) { }
                        public void name3(int i, float j, int k) { }

                        public void name4(int i, int j, float f, long z) { }
                        public void name4(double d, int i, int j, float f) { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check Callback Handlers`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:16: warning: Registration methods should have overload that accepts delivery Executor: `registerWrongCallback` [ExecutorRegistration] [See https://s.android.com/api-guidelines#callbacks-listener]
                src/android/pkg/MyClass.java:6: warning: Registration methods should have overload that accepts delivery Executor: `MyClass` [ExecutorRegistration] [See https://s.android.com/api-guidelines#callbacks-listener]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class MyClass {
                        public MyClass(@Nullable MyCallback callback) {
                        }

                        public void registerStreamEventCallback(@Nullable MyCallback callback);
                        public void unregisterStreamEventCallback(@Nullable MyCallback callback);
                        public void registerStreamEventCallback(@Nullable java.util.concurrent.Executor executor,
                                @Nullable MyCallback callback);
                        public void unregisterStreamEventCallback(@Nullable java.util.concurrent.Executor executor,
                                @Nullable MyCallback callback);

                        public void registerWrongCallback(@Nullable MyCallback callback);
                        public void unregisterWrongCallback(@Nullable MyCallback callback);
                    }
                    """
                ),
                java(
                    """
                    package android.graphics;

                    import android.pkg.MyCallback;
                    import androidx.annotation.Nullable;

                    public class MyUiClass {
                        public void registerWrongCallback(@Nullable MyCallback callback);
                        public void unregisterWrongCallback(@Nullable MyCallback callback);
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    @SuppressWarnings("WeakerAccess")
                    public abstract class MyCallback {
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check resource names`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/R.java:11: error: Expected resource name in `android.R.id` to be in the `fooBarBaz` style, was `wrong_style` [ResourceValueFieldName]
                src/android/R.java:17: error: Expected config name to be in the `config_fooBarBaz` style, was `config_wrong_config_style` [ConfigFieldName]
                src/android/R.java:20: error: Expected resource name in `android.R.layout` to be in the `foo_bar_baz` style, was `wrongNameStyle` [ResourceFieldName]
                src/android/R.java:31: error: Expected resource name in `android.R.style` to be in the `FooBar_Baz` style, was `wrong_style_name` [ResourceStyleFieldName]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android;

                    public final class R {
                        public static final class id {
                            public static final int text = 7000;
                            public static final int config_fooBar = 7001;
                            public static final int layout_fooBar = 7002;
                            public static final int state_foo = 7003;
                            public static final int foo = 7004;
                            public static final int fooBar = 7005;
                            public static final int wrong_style = 7006;
                        }
                        public static final class layout {
                            public static final int text = 7000;
                            public static final int config_fooBar = 7001;
                            public static final int config_foo = 7002;
                            public static final int config_wrong_config_style = 7003;

                            public static final int ok_name_style = 7004;
                            public static final int wrongNameStyle = 7005;
                        }
                        public static final class style {
                            public static final int TextAppearance_Compat_Notification = 0x7f0c00ec;
                            public static final int TextAppearance_Compat_Notification_Info = 0x7f0c00ed;
                            public static final int TextAppearance_Compat_Notification_Line2 = 0x7f0c00ef;
                            public static final int TextAppearance_Compat_Notification_Time = 0x7f0c00f2;
                            public static final int TextAppearance_Compat_Notification_Title = 0x7f0c00f4;
                            public static final int Widget_Compat_NotificationActionContainer = 0x7f0c015d;
                            public static final int Widget_Compat_NotificationActionText = 0x7f0c015e;

                            public static final int wrong_style_name = 7000;
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check files`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/CheckFiles.java:13: warning: Methods accepting `File` should also accept `FileDescriptor` or streams: method android.pkg.CheckFiles.error(int,java.io.File) [StreamFiles]
                src/android/pkg/CheckFiles.java:9: warning: Methods accepting `File` should also accept `FileDescriptor` or streams: constructor android.pkg.CheckFiles(android.content.Context,java.io.File) [StreamFiles]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.content.Context;
                    import android.content.ContentResolver;
                    import androidx.annotation.Nullable;
                    import java.io.File;
                    import java.io.InputStream;

                    public class CheckFiles {
                        public CheckFiles(@Nullable Context context, @Nullable File file) {
                        }
                        public void ok(int i, @Nullable File file) { }
                        public void ok(int i, @Nullable InputStream stream) { }
                        public void error(int i, @Nullable File file) { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check parcelable lists`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/CheckFiles.java:13: warning: Methods accepting `File` should also accept `FileDescriptor` or streams: method android.pkg.CheckFiles.error(int,java.io.File) [StreamFiles]
                src/android/pkg/CheckFiles.java:9: warning: Methods accepting `File` should also accept `FileDescriptor` or streams: constructor android.pkg.CheckFiles(android.content.Context,java.io.File) [StreamFiles]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.content.Context;
                    import android.content.ContentResolver;
                    import androidx.annotation.Nullable;
                    import java.io.File;
                    import java.io.InputStream;

                    public class CheckFiles {
                        public CheckFiles(@Nullable Context context, @Nullable File file) {
                        }
                        public void ok(int i, @Nullable File file) { }
                        public void ok(int i, @Nullable InputStream stream) { }
                        public void error(int i, @Nullable File file) { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check abstract inner`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyManager.java:9: warning: Abstract inner classes should be static to improve testability: class android.pkg.MyManager.MyInnerManager [AbstractInner]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.content.Context;
                    import android.content.ContentResolver;
                    import java.io.File;
                    import java.io.InputStream;

                    public abstract class MyManager {
                         private MyManager() {}
                         public abstract class MyInnerManager {
                             private MyInnerManager() {}
                         }
                         public abstract static class MyOkManager {
                             private MyOkManager() {}
                         }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check for banned runtime exceptions`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:7: error: Methods must not mention RuntimeException subclasses in throws clauses (was `java.lang.SecurityException`) [BannedThrow]
                src/android/pkg/MyClass.java:6: error: Methods must not mention RuntimeException subclasses in throws clauses (was `java.lang.ClassCastException`) [BannedThrow]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass {
                         private MyClass() throws NullPointerException {} // OK, private
                         @SuppressWarnings("RedundantThrows") public MyClass(int i) throws java.io.IOException {} // OK, not runtime exception
                         public MyClass(double l) throws ClassCastException {} // error
                         public void foo() throws SecurityException {} // error
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check for extending errors`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyClass.java:3: error: Trouble must be reported through an `Exception`, not an `Error` (`MyClass` extends `Error`) [ExtendsError]
                src/android/pkg/MySomething.java:3: error: Exceptions must be named `FooException`, was `MySomething` [ExceptionName]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MyClass extends Error {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MySomething extends RuntimeException {
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check units and method names`() {
        check(
            extraArguments = arrayOf(ARG_API_LINT, ARG_HIDE, "NoByteOrShort"),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/UnitNameTest.java:7: error: Expected method name units to be `Hours`, was `Hr` in `getErrorHr` [MethodNameUnits] [See https://s.android.com/api-guidelines#unit-names]
                src/android/pkg/UnitNameTest.java:8: error: Expected method name units to be `Nanos`, was `Ns` in `getErrorNs` [MethodNameUnits] [See https://s.android.com/api-guidelines#unit-names]
                src/android/pkg/UnitNameTest.java:9: error: Expected method name units to be `Bytes`, was `Byte` in `getErrorByte` [MethodNameUnits] [See https://s.android.com/api-guidelines#unit-names]
                src/android/pkg/UnitNameTest.java:14: error: Fractions must use floats, was `int` in `getErrorFraction` [FractionFloat]
                src/android/pkg/UnitNameTest.java:15: error: Fractions must use floats, was `int` in `setErrorFraction` [FractionFloat]
                src/android/pkg/UnitNameTest.java:19: error: Percentage must use ints, was `float` in `getErrorPercentage` [PercentageInt]
                src/android/pkg/UnitNameTest.java:20: error: Percentage must use ints, was `float` in `setErrorPercentage` [PercentageInt]
                src/android/pkg/UnitNameTest.java:22: error: Expected method name units to be `Bytes`, was `Byte` in `readSingleByte` [MethodNameUnits] [See https://s.android.com/api-guidelines#unit-names]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                androidxNonNullSource,
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;

                    public class UnitNameTest {
                        public int okay() { return 0; }
                        public int getErrorHr() { return 0; }
                        public int getErrorNs() { return 0; }
                        public short getErrorByte() { return (short)0; }

                        public float getOkFraction() { return 0f; }
                        public void setOkFraction(float f) { }
                        public void setOkFraction(int n, int d) { }
                        public int getErrorFraction() { return 0; }
                        public void setErrorFraction(int i) { }

                        public int getOkPercentage() { return 0f; }
                        public void setOkPercentage(int i) { }
                        public float getErrorPercentage() { return 0f; }
                        public void setErrorPercentage(float f) { }

                        public int readSingleByte() { return 0; }

                        public static final class UnitNameTestBuilder {
                            public UnitNameTestBuilder() {}

                            @NonNull
                            public UnitNameTest build() { return null; }

                            @NonNull
                            public UnitNameTestBuilder setOkFraction(float f) { return this; }

                            @NonNull
                            public UnitNameTestBuilder setOkPercentage(int i) { return this; }
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check closeable`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyErrorClass1.java:3: warning: Classes that release resources (close()) should implement AutoClosable and CloseGuard: class android.pkg.MyErrorClass1 [NotCloseable]
                src/android/pkg/MyErrorClass2.java:3: warning: Classes that release resources (finalize(), shutdown()) should implement AutoClosable and CloseGuard: class android.pkg.MyErrorClass2 [NotCloseable]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyOkClass1 implements java.io.Closeable {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    // Ok: indirectly implementing AutoCloseable
                    public abstract class MyOkClass2 implements MyInterface {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public class MyInterface extends AutoCloseable {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass1 {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass2 {
                        public void finalize() {}
                        public void shutdown() {}
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check closeable for minSdkVersion 19`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyErrorClass1.java:3: warning: Classes that release resources (close()) should implement AutoClosable and CloseGuard: class android.pkg.MyErrorClass1 [NotCloseable]
                src/android/pkg/MyErrorClass2.java:3: warning: Classes that release resources (finalize(), shutdown()) should implement AutoClosable and CloseGuard: class android.pkg.MyErrorClass2 [NotCloseable]
            """,
            manifest = """<?xml version="1.0" encoding="UTF-8"?>
                <manifest xmlns:android="http://schemas.android.com/apk/res/android">
                    <uses-sdk android:minSdkVersion="19" />
                </manifest>
            """.trimIndent(),
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass1 {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass2 {
                        public void finalize() {}
                        public void shutdown() {}
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Do not check closeable for minSdkVersion less than 19`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = "",
            manifest = """<?xml version="1.0" encoding="UTF-8"?>
                <manifest xmlns:android="http://schemas.android.com/apk/res/android">
                    <uses-sdk android:minSdkVersion="18" />
                </manifest>
            """.trimIndent(),
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass1 {
                        public void close() {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    public abstract class MyErrorClass2 {
                        public void finalize() {}
                        public void shutdown() {}
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check ICU types for minSdkVersion 24`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyErrorClass1.java:7: warning: Type `java.util.TimeZone` should be replaced with richer ICU type `android.icu.util.TimeZone` [UseIcu]
            """,
            manifest = """<?xml version="1.0" encoding="UTF-8"?>
                <manifest xmlns:android="http://schemas.android.com/apk/res/android">
                    <uses-sdk android:minSdkVersion="24" />
                </manifest>
            """.trimIndent(),
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.annotation.NonNull;
                    import java.util.TimeZone;

                    public abstract class MyErrorClass1 {
                        @NonNull
                        public TimeZone getDefaultTimeZone() {
                            return TimeZone.getDefault();
                        }
                    }
                    """
                ),
                nonNullSource
            )
        )
    }

    @Test
    fun `Do not check ICU types for minSdkVersion less than 24`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = "",
            manifest = """<?xml version="1.0" encoding="UTF-8"?>
                <manifest xmlns:android="http://schemas.android.com/apk/res/android">
                    <uses-sdk android:minSdkVersion="23" />
                </manifest>
            """.trimIndent(),
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.annotation.NonNull;
                    import java.util.TimeZone;

                    public abstract class MyErrorClass1 {
                        @NonNull
                        public TimeZone getDefaultTimeZone() {
                            return TimeZone.getDefault();
                        }
                    }
                    """
                ),
                nonNullSource
            )
        )
    }

    @Test
    fun `Check Kotlin keywords`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/KotlinKeywordTest.java:7: error: Avoid method names that are Kotlin hard keywords ("fun"); see https://android.github.io/kotlin-guides/interop.html#no-hard-keywords [KotlinKeyword]
                src/android/pkg/KotlinKeywordTest.java:8: error: Avoid field names that are Kotlin hard keywords ("as"); see https://android.github.io/kotlin-guides/interop.html#no-hard-keywords [KotlinKeyword]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class KotlinKeywordTest {
                        public void okay();
                        public final int okay = 0;

                        public void fun() {} // error
                        public final int as = 0; // error
                    }
                    """
                ),
                androidxNonNullSource,
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check Kotlin operators`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/KotlinOperatorTest.java:6: info: Method can be invoked with an indexing operator from Kotlin: `get` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                src/android/pkg/KotlinOperatorTest.java:7: info: Method can be invoked with an indexing operator from Kotlin: `set` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                src/android/pkg/KotlinOperatorTest.java:8: info: Method can be invoked with function call syntax from Kotlin: `invoke` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                src/android/pkg/KotlinOperatorTest.java:9: info: Method can be invoked as a binary operator from Kotlin: `plus` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                src/android/pkg/KotlinOperatorTest.java:9: error: Only one of `plus` and `plusAssign` methods should be present for Kotlin [UniqueKotlinOperator]
                src/android/pkg/KotlinOperatorTest.java:10: info: Method can be invoked as a compound assignment operator from Kotlin: `plusAssign` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class KotlinOperatorTest {
                        public int get(int i) { return i + 2; }
                        public void set(int i, int j, int k) { }
                        public void invoke(int i, int j, int k) { }
                        public int plus(@Nullable JavaClass other) { return 0; }
                        public void plusAssign(@Nullable JavaClass other) { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Return collections instead of arrays`() {
        check(
            extraArguments = arrayOf(ARG_API_LINT, ARG_HIDE, "AutoBoxing"),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/ArrayTest.java:11: warning: Method should return Collection<Object> (or subclass) instead of raw array; was `java.lang.Object[]` [ArrayReturn] [See https://s.android.com/api-guidelines#methods-prefer-collection-over-array]
                src/android/pkg/ArrayTest.java:13: warning: Method parameter should be Collection<Number> (or subclass) instead of raw array; was `java.lang.Number[]` [ArrayReturn] [See https://s.android.com/api-guidelines#methods-prefer-collection-over-array]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.NonNull;

                    public class ArrayTest {
                        @NonNull
                        public int[] ok1() { throw new RuntimeException(); }
                        @NonNull
                        public String[] ok2() { throw new RuntimeException(); }
                        public void ok3(@Nullable int[] i) { }
                        @NonNull
                        public Object[] error1() { throw new RuntimeException(); }
                        public void error2(@NonNull Number[] i) { }
                        public void ok(@NonNull Number... args) { }
                    }
                    """
                ),
                kotlin("""
                    package test.pkg
                    fun okMethod(vararg values: Integer, foo: Float, bar: Float)
                    """),
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `Check user handle names`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MyManager.java:7: warning: When a method overload is needed to target a specific UserHandle, callers should be directed to use Context.createPackageContextAsUser() and re-obtain the relevant Manager, and no new API should be added [UserHandle]
                src/android/pkg/UserHandleTest.java:8: warning: Method taking UserHandle should be named `doFooAsUser` or `queryFooForUser`, was `error` [UserHandleName]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    import android.os.UserHandle;
                    import androidx.annotation.Nullable;

                    public class UserHandleTest {
                        public void doFooAsUser(int i, @Nullable UserHandle handle) {} //OK
                        public void doFooForUser(int i, @Nullable UserHandle handle) {} //OK
                        public void error(int i, @Nullable UserHandle handle) {}
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;
                    import android.os.UserHandle;
                    import androidx.annotation.Nullable;

                    public class MyManager {
                        private MyManager() { }
                        public void error(int i, @Nullable UserHandle handle) {}
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check parameters`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/FooOptions.java:3: warning: Classes holding a set of parameters should be called `FooParams`, was `FooOptions` [UserHandleName]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class FooOptions {
                    }
                    """
                ),
                java(
                    """
                    package android.app;

                    public class ActivityOptions {
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check service names`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/content/Context.java:11: error: Inconsistent service constant name; expected `SOMETHING_SERVICE`, was `OTHER_MANAGER` [ServiceName]
                src/android/content/Context.java:12: error: Inconsistent service constant name; expected `OTHER_SERVICE`, was `OTHER_MANAGER_SERVICE` [ServiceName]
                src/android/content/Context.java:9: error: Inconsistent service value; expected `other`, was `something` (Note: Do not change the name of already released services, which will break tools using `adb shell dumpsys`. Instead add `@SuppressLint("ServiceName"))` [ServiceName]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.content;

                    public class Context {
                        private Context() { }
                        // Good
                        public static final String FOO_BAR_SERVICE = "foo_bar";
                        // Unrelated
                        public static final int NON_STRING_SERVICE = 42;
                        // Bad
                        public static final String OTHER_SERVICE = "something";
                        public static final String OTHER_MANAGER = "something";
                        public static final String OTHER_MANAGER_SERVICE = "other_manager";
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    // Unrelated
                    public class ServiceNameTest {
                        private ServiceNameTest() { }
                        public static final String FOO_BAR_SERVICE = "foo_bar";
                        public static final String OTHER_SERVICE = "something";
                        public static final int NON_STRING_SERVICE = 42;
                        public static final String BIND_SOME_SERVICE = "android.permission.BIND_SOME_SERVICE";
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check method name tense`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MethodNameTest.java:6: warning: Unexpected tense; probably meant `enabled`, was `fooEnable` [MethodNameTense]
                src/android/pkg/MethodNameTest.java:7: warning: Unexpected tense; probably meant `enabled`, was `mustEnable` [MethodNameTense]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public class MethodNameTest {
                        private MethodNameTest() { }
                        public void enable() { } // ok, not Enable
                        public void fooEnable() { } // warn
                        public boolean mustEnable() { return true; } // warn
                        public boolean isEnabled() { return true; }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check no clone`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/CloneTest.java:7: error: Provide an explicit copy constructor instead of implementing `clone()` [NoClone] [See https://s.android.com/api-guidelines#avoid-clone]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public class CloneTest {
                        public void clone(int i) { } // ok
                        @Nullable
                        public CloneTest clone() { return super.clone(); } // error
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check ICU types`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/IcuTest.java:6: warning: Type `java.util.TimeZone` should be replaced with richer ICU type `android.icu.util.TimeZone` [UseIcu]
                src/android/pkg/IcuTest.java:7: warning: Type `java.text.BreakIterator` should be replaced with richer ICU type `android.icu.text.BreakIterator` [UseIcu]
                src/android/pkg/IcuTest.java:8: warning: Type `java.text.Collator` should be replaced with richer ICU type `android.icu.text.Collator` [UseIcu]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public abstract class IcuTest {
                        public IcuTest(@Nullable java.util.TimeZone timeZone) { }
                        @Nullable
                        public abstract java.text.BreakIterator foo(@Nullable java.text.Collator collator);
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check using parcel file descriptors`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/PdfTest.java:6: error: Must use ParcelFileDescriptor instead of FileDescriptor in parameter fd in android.pkg.PdfTest.error1(java.io.FileDescriptor fd) [UseParcelFileDescriptor] [See https://s.android.com/api-guidelines#prefer-parcelfiledescriptor]
                src/android/pkg/PdfTest.java:7: error: Must use ParcelFileDescriptor instead of FileDescriptor in method android.pkg.PdfTest.getFileDescriptor() [UseParcelFileDescriptor] [See https://s.android.com/api-guidelines#prefer-parcelfiledescriptor]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public abstract class PdfTest {
                        public void error1(@Nullable java.io.FileDescriptor fd) { }
                        public int getFileDescriptor() { return -1; }
                        public void ok(@Nullable android.os.ParcelFileDescriptor fd) { }
                    }
                    """
                ),
                java(
                    """
                    package android.system;

                    public class Os {
                        public void ok(@Nullable java.io.FileDescriptor fd) { }
                        public int getFileDescriptor() { return -1; }
                    }
                    """
                ),
                java(
                    """
                    package android.yada;

                    import android.annotation.NonNull;

                    public class YadaService extends android.app.Service {
                        @Override
                        public final void dump(@NonNull java.io.FileDescriptor fd, @NonNull java.io.PrintWriter pw, @NonNull String[] args) {
                        }
                    }
                    """
                ),
                androidxNullableSource,
                nonNullSource
            )
        )
    }

    @Test
    fun `Check using bytes and shorts`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/ByteTest.java:4: warning: Should avoid odd sized primitives; use `int` instead of `byte` in parameter b in android.pkg.ByteTest.error1(byte b) [NoByteOrShort] [See https://s.android.com/api-guidelines#avoid-short-byte]
                src/android/pkg/ByteTest.java:5: warning: Should avoid odd sized primitives; use `int` instead of `short` in parameter s in android.pkg.ByteTest.error2(short s) [NoByteOrShort] [See https://s.android.com/api-guidelines#avoid-short-byte]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    public abstract class ByteTest {
                        public void error1(byte b) { }
                        public void error2(short s) { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Check singleton constructors`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/MySingleton.java:8: error: Singleton classes should use `getInstance()` methods: `MySingleton` [SingletonConstructor] [See https://s.android.com/api-guidelines#singleton-class]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public abstract class MySingleton {
                        @Nullable
                        public static MySingleton getMyInstance() { return null; }
                        public MySingleton() { }
                        public void foo() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    import androidx.annotation.Nullable;

                    public abstract class MySingleton2 {
                        @Nullable
                        public static MySingleton2 getMyInstance() { return null; }
                        private MySingleton2() { } // OK, private
                        public void foo() { }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Check forbidden super-classes`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/FirstActivity.java:2: error: FirstActivity should not extend `Activity`. Activity subclasses are impossible to compose. Expose a composable API instead. [ForbiddenSuperClass]
                src/android/pkg/IndirectActivity.java:2: error: IndirectActivity should not extend `Activity`. Activity subclasses are impossible to compose. Expose a composable API instead. [ForbiddenSuperClass]
                src/android/pkg/MyTask.java:2: error: MyTask should not extend `AsyncTask`. AsyncTask is an implementation detail. Expose a listener or, in androidx, a `ListenableFuture` API instead [ForbiddenSuperClass]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.pkg;
                    public abstract class FirstActivity extends android.app.Activity {
                        private FirstActivity() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;
                    public abstract class IndirectActivity extends android.app.ListActivity {
                        private IndirectActivity() { }
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;
                    public abstract class MyTask extends android.os.AsyncTask<String,String,String> {
                        private MyTask() { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `KotlinOperator check only applies when not using operator modifier`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/A.kt:3: info: Note that adding the `operator` keyword would allow calling this method using operator syntax [KotlinOperator]
                src/android/pkg/Bar.kt:4: info: Note that adding the `operator` keyword would allow calling this method using operator syntax [KotlinOperator]
                src/android/pkg/Foo.java:7: info: Method can be invoked as a binary operator from Kotlin: `div` (this is usually desirable; just make sure it makes sense for this type of object) [KotlinOperator]
                """,
            sourceFiles = arrayOf(
                java(
                    """
                        package android.pkg;

                        import androidx.annotation.Nullable;

                        public class Foo {
                            private Foo() { }
                            @Nullable
                            public Foo div(int value) { }
                        }
                    """
                ),
                kotlin(
                    """
                        package android.pkg
                        class Bar {
                            operator fun div(value: Int): Bar { TODO() }
                            fun plus(value: Int): Bar { TODO() }
                        }
                    """
                ),
                kotlin(
                    """
                        package android.pkg
                        class FontFamily(val fonts: List<String>) : List<String> by fonts
                    """
                ),
                kotlin(
                    """
                        package android.pkg
                        class B: A() {
                            override fun get(i: Int): A {
                                return A()
                            }
                        }
                    """
                ),
                kotlin(
                    """
                        package android.pkg
                        open class A {
                            open fun get(i: Int): A {
                                return A()
                            }
                        }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `Test fields, parameters and returns require nullability`() {
        check(
            apiLint = "", // enabled
            extraArguments = arrayOf(ARG_API_LINT, ARG_HIDE, "AllUpper,StaticUtils,Enum"),
            compatibilityMode = false,
            expectedIssues = """
                src/android/pkg/Foo.java:11: error: Missing nullability on parameter `name` in method `Foo` [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                src/android/pkg/Foo.java:12: error: Missing nullability on parameter `value` in method `setBadValue` [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                src/android/pkg/Foo.java:13: error: Missing nullability on method `getBadValue` return [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                src/android/pkg/Foo.java:20: error: Missing nullability on parameter `duration` in method `methodMissingParamAnnotations` [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                src/android/pkg/Foo.java:7: error: Missing nullability on field `badField` in class `class android.pkg.Foo` [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                        package android.pkg;

                        import androidx.annotation.NonNull;
                        import androidx.annotation.Nullable;

                        public class Foo<T> {
                            public final Foo badField;
                            @Nullable
                            public final Foo goodField;

                            public Foo(String name, int number) { }
                            public void setBadValue(Foo value) { }
                            public Foo getBadValue(int number) { throw UnsupportedOperationExceptions(); }
                            public void setGoodValue(@Nullable Foo value) { }
                            public void setGoodIgnoredGenericValue(T value) { }
                            @NonNull
                            public Foo getGoodValue(int number) { throw UnsupportedOperationExceptions(); }

                            @NonNull
                            public Foo methodMissingParamAnnotations(java.time.Duration duration) {
                                throw UnsupportedOperationException();
                            }
                        }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public enum Foo {
                        A, B;
                    }
                    """
                ),
                kotlin(
                    """
                    package test.pkg
                    enum class Language {
                        KOTLIN,
                        JAVA
                    }
                    """
                ),
                kotlin(
                    """
                    package android.pkg

                    object Bar

                    class FooBar {
                        companion object
                    }

                    class FooBarNamed {
                        companion object Named
                    }
                    """
                ),
                androidxNullableSource,
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `Test equals, toString, non-null constants, enums and annotation members don't require nullability`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = "",
            sourceFiles = arrayOf(
                java(
                    """
                        package android.pkg;

                        import android.annotation.SuppressLint;

                        public class Foo<T> {
                            public static final String FOO_CONSTANT = "test";

                            public boolean equals(Object other) {
                                return other == this;
                            }

                            public int hashCode() {
                                return 0;
                            }

                            public String toString() {
                                return "Foo";
                            }

                            @SuppressLint("Enum")
                            public enum FooEnum {
                                FOO, BAR
                            }

                            public @interface FooAnnotation {
                                String value() default "";
                            }
                        }
                    """
                ),
                androidxNullableSource,
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `Nullability check for generic methods referencing parent type parameter`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = """
                src/test/pkg/MyClass.java:13: error: Missing nullability on method `method4` return [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
                src/test/pkg/MyClass.java:14: error: Missing nullability on parameter `input` in method `method4` [MissingNullability] [See https://s.android.com/api-guidelines#annotations]
            """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    public class MyClass extends HiddenParent<String> {
                        public void method1() { }

                        @Nullable
                        @Override
                        public String method3(@NonNull String input) { return null; }

                        @Override
                        public String method4(String input) { return null; }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    class HiddenParent<T> {
                        public T method2(T t) { }
                        public T method3(T t) { }
                        public T method4(T t) { }
                    }
                    """
                ),
                androidxNullableSource,
                androidxNonNullSource
            )
        )
    }

    @Test
    fun `No new setting keys`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            extraArguments = arrayOf(
                ARG_ERROR,
                "NoSettingsProvider"
            ),
            expectedIssues = """
                src/android/provider/Settings.java:9: error: New setting keys are not allowed (Field: BAD1); use getters/setters in relevant manager class [NoSettingsProvider] [See https://s.android.com/api-guidelines#no-settings-provider]
                src/android/provider/Settings.java:11: error: Bare field okay2 must be marked final, or moved behind accessors if mutable [MutableBareField] [See https://s.android.com/api-guidelines#mutable-bare-field]
                src/android/provider/Settings.java:17: error: New setting keys are not allowed (Field: BAD1); use getters/setters in relevant manager class [NoSettingsProvider] [See https://s.android.com/api-guidelines#no-settings-provider]
                src/android/provider/Settings.java:21: error: New setting keys are not allowed (Field: BAD1); use getters/setters in relevant manager class [NoSettingsProvider] [See https://s.android.com/api-guidelines#no-settings-provider]
            """,
            expectedFail = DefaultLintErrorMessage,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.provider;

                    import androidx.annotation.Nullable;

                    public class Settings {
                        private Settings() { }
                        public static class Global {
                            private Global() { }
                            public static final String BAD1 = "";
                            public final String okay1 = "";
                            @Nullable
                            public static String okay2 = "";
                            public static final int OKAY3 = 0;
                        }
                        public static class Secure {
                            private Secure() { }
                            public static final String BAD1 = "";
                        }
                        public static class System {
                            private System() { }
                            public static final String BAD1 = "";
                        }
                        public static class Other {
                            private Other() { }
                            public static final String OKAY1 = "";
                        }
                    }
                    """
                ),
                androidxNullableSource
            )
        )
    }

    @Test
    fun `No issues for ignored packages`() {
        check(
            apiLint = """
                package java.math {
                  public class BigInteger {
                    ctor public BigInteger();
                  }
                }
            """.trimIndent(),
            compatibilityMode = false,
            extraArguments = arrayOf(
                ARG_API_LINT_IGNORE_PREFIX,
                "java."
            ),
            expectedIssues = "",
            sourceFiles = arrayOf(
                java(
                    """
                    package java.math;

                    public class BigInteger {
                        public byte newMethod() {
                            return 0;
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `vararg use in annotations`() {
        check(
            apiLint = "", // enabled
            compatibilityMode = false,
            expectedIssues = "",
            sourceFiles = arrayOf(
                kotlin(
                    """
                        package test.pkg

                        import kotlin.reflect.KClass

                        annotation class MyAnnotation(
                            vararg val markerClass: KClass<out Annotation>
                        )
                    """
                )
            )
        )
    }

    @Test
    fun `Inherited interface constants`() {
        check(
            compatibilityMode = false,
            expectedIssues = "",
            expectedFail = "",
            apiLint = """
                package javax.microedition.khronos.egl {
                    public interface EGL {
                    }
                    public interface EGL10 extends javax.microedition.khronos.egl.EGL {
                        field public static final int EGL_SUCCESS = 0;
                    }
                    public interface EGL11 extends javax.microedition.khronos.egl.EGL10 {
                        field public static final int EGL_CONTEXT_LOST = 1;
                    }
                    public interface EGLDisplay {
                    }
                }
                """,
            sourceFiles = arrayOf(
                java(
                    """
                        package javax.microedition.khronos.egl;

                        public interface EGL {
                        }
                    """
                ),
                java(
                    """
                        package javax.microedition.khronos.egl;

                        public interface EGL10 extends EGL {
                            EGLDisplay EGL_SUCCESS = new EGLImpl();
                        }
                    """
                ),
                java(
                    """
                        package javax.microedition.khronos.egl;

                        public interface EGL11 extends EGL10 {
                            int EGL_CONTEXT_LOST = 1;
                        }
                    """
                ),
                java(
                    """
                        package javax.microedition.khronos.egl;

                        public abstract class EGLDisplay {
                        }
                    """
                )
            )
        )
    }

    @Test
    fun `Inherited interface constants inherited through parents into children`() {
        check(
            compatibilityMode = false,
            expectedIssues = "",
            expectedFail = "",
            apiLint = """
                package android.provider {
                  public static final class Settings.Global extends android.provider.Settings.NameValueTable {
                  }
                  public static class Settings.NameValueTable implements android.provider.BaseColumns {
                  }
                  public interface BaseColumns {
                      field public static final String _ID = "_id";
                  }
                }
                """,
            sourceFiles = arrayOf(
                java(
                    """
                        package android.provider;

                        public class Settings {
                            private Settings() { }
                            public static final class Global extends NameValueTable {
                            }
                            public static final class NameValueTable implements BaseColumns {
                            }
                        }
                    """
                ),
                java(
                    """
                        package android.provider;

                        public interface BaseColumns {
                            public static final String _ID = "_id";
                        }
                    """
                )
            ),
            extraArguments = arrayOf("--error", "NoSettingsProvider")
        )
    }

    @Test
    fun `No warnings about nullability on private constructor getters`() {
        check(
            compatibilityMode = false,
            expectedIssues = "",
            apiLint = "",
            sourceFiles = arrayOf(
                kotlin(
                    """
                        package test.pkg
                        class MyClass private constructor(
                            val myParameter: Set<Int>
                        )
                    """
                )
            )
        )
    }
}
