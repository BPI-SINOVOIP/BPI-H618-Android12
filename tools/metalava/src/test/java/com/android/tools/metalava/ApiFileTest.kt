/*
 * Copyright (C) 2017 The Android Open Source Project
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

@file:Suppress("ALL")

package com.android.tools.metalava

import com.android.tools.lint.checks.infrastructure.TestFiles.base64gzip
import org.junit.Test

class ApiFileTest : DriverTest() {
/*
   Conditions to test:
   - test all the error scenarios found in the notStrippable case!
   - split up test into many individual test cases
   - try referencing a class from an annotation!
   - test having a throws list where some exceptions are hidden but extend
     public exceptions: do we map over to the referenced ones?

   - test type reference from all the possible places -- in type signatures - interfaces,
     extends, throws, type bounds, etc.
   - method which overrides @hide method: should appear in subclass (test chain
     of two nested too)
   - BluetoothGattCharacteristic.java#describeContents: Was marked @hide,
     but is unhidden because it extends a public interface method
   - package javadoc (also make sure merging both!, e.g. try having @hide in each)
   - StopWatchMap -- inner class with @hide marks allh top levels!
   - Test field inlining: should I include fields from an interface, if that
     inteface was implemented by the parent class (and therefore appears there too?)
     What if the superclass is abstract?
   - Exposing package private classes. Test that I only do this for package private
     classes, NOT Those marked @hide (is that, having @hide on a used type, illegal?)
   - Test error handling (invalid @hide combinations))
   - Consider what happens if we promote a package private class (because it's
     extended by a public class), and then we restore its public members; the
     override logic there isn't quite right. We've duplicated the significant-override
     code to not skip private members, but that could change semantics. This isn't
     ideal; instead we should now mark this class as public, and re-run the analysis
     again (with the new hidden state for this class).
   - compilation unit sorting - top level classes out of order
   - Massive classes such as android.R.java? Maybe do synthetic test.
   - HttpResponseCache implemented a public OkHttp interface, but the sole implementation
     method was marked @hide, so the method doesn't show up. Is that some other rule --
     that we skip interfaces if their implementation methods are marked @hide?
   - Test recursive package filtering.
 */

    @Test
    fun `Basic class signature extraction`() {
        // Basic class; also checks that default constructor is made explicit
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Foo {
                    }
                    """
                )
            ),
            api = """
                    package test.pkg {
                      public class Foo {
                        ctor public Foo();
                      }
                    }
                    """
        )
    }

    @Test
    fun `Parameter Names in Java`() {
        // Java code which explicitly specifies parameter names
        check(
            compatibilityMode = false, // parameter names only in v2
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.ParameterName;

                    public class Foo {
                        public void foo(int javaParameter1, @ParameterName("publicParameterName") int javaParameter2) {
                        }
                    }
                    """
                ),
                supportParameterName
            ),
            api = """
                    package test.pkg {
                      public class Foo {
                        ctor public Foo();
                        method public void foo(int, int publicParameterName);
                      }
                    }
                 """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `Default Values Names in Java`() {
        // Java code which explicitly specifies parameter names
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.DefaultValue;

                    public class Foo {
                        public void foo(
                            @DefaultValue("null") String prefix,
                            @DefaultValue("\"Hello World\"") String greeting,
                            @DefaultValue("42") int meaning) {
                        }
                    }
                    """
                ),
                supportDefaultValue
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public class Foo {
                    ctor public Foo();
                    method public void foo(String! = null, String! = "Hello World", int = 42);
                  }
                }
                 """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `Default Values and Names in Kotlin`() {
        // Kotlin code which explicitly specifies parameter names
        check(
            format = FileFormat.V3,
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    import some.other.pkg.Constants.Misc.SIZE
                    import android.graphics.Bitmap
                    import android.view.View

                    class Foo {
                        fun method1(myInt: Int = 42,
                            myInt2: Int? = null,
                            myByte: Int = 2 * 21,
                            str: String = "hello " + "world",
                            vararg args: String) { }

                        fun method2(myInt: Int, myInt2: Int = (2*int) * SIZE) { }

                        fun method3(str: String, myInt: Int, myInt2: Int = double(int) + str.length) { }

                        fun emptyLambda(sizeOf: () -> Unit = {  }) {}

                        fun View.drawToBitmap(config: Bitmap.Config = Bitmap.Config.ARGB_8888): Bitmap? = null

                        companion object {
                            fun double(myInt: Int) = 2 * myInt
                            fun print(foo: Foo = Foo()) { println(foo) }
                        }
                    }
                    """
                ),
                java(
                    """
                    package some.other.pkg;
                    public class Constants {
                        public static class Misc {
                            public static final int SIZE = 5;
                        }
                    }
                    """
                )
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class Foo {
                    ctor public Foo();
                    method public android.graphics.Bitmap? drawToBitmap(android.view.View, android.graphics.Bitmap.Config config = android.graphics.Bitmap.Config.ARGB_8888);
                    method public void emptyLambda(kotlin.jvm.functions.Function0<kotlin.Unit> sizeOf = {});
                    method public void method1(int myInt = 42, Integer? myInt2 = null, int myByte = 42, String str = "hello world", java.lang.String... args);
                    method public void method2(int myInt, int myInt2 = (2 * int) * some.other.pkg.Constants.Misc.SIZE);
                    method public void method3(String str, int myInt, int myInt2 = double(int) + str.length);
                    field public static final test.pkg.Foo.Companion Companion;
                  }
                  public static final class Foo.Companion {
                    method public int double(int myInt);
                    method public void print(test.pkg.Foo foo = test.pkg.Foo());
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation", ARG_HIDE_PACKAGE, "some.other.pkg"),
            includeSignatureVersion = true
        )
    }

    @Test
    fun `Default Values in Kotlin for expressions`() {
        // Testing trickier default values; regression test for problem
        // observed in androidx.core.util with LruCache
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package androidx.core.util

                    import android.util.LruCache

                    inline fun <K : Any, V : Any> lruCache(
                        maxSize: Int,
                        crossinline sizeOf: (key: K, value: V) -> Int = { _, _ -> 1 },
                        @Suppress("USELESS_CAST") // https://youtrack.jetbrains.com/issue/KT-21946
                        crossinline create: (key: K) -> V? = { null as V? },
                        crossinline onEntryRemoved: (evicted: Boolean, key: K, oldValue: V, newValue: V?) -> Unit =
                            { _, _, _, _ -> }
                    ): LruCache<K, V> {
                        return object : LruCache<K, V>(maxSize) {
                            override fun sizeOf(key: K, value: V) = sizeOf(key, value)
                            override fun create(key: K) = create(key)
                            override fun entryRemoved(evicted: Boolean, key: K, oldValue: V, newValue: V?) {
                                onEntryRemoved(evicted, key, oldValue, newValue)
                            }
                        }
                    }
                    """
                ),
                java(
                    """
                    package androidx.collection;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    import java.util.LinkedHashMap;
                    import java.util.Locale;
                    import java.util.Map;

                    public class LruCache<K, V> {
                        @Nullable
                        protected V create(@NonNull K key) {
                            return null;
                        }

                        protected int sizeOf(@NonNull K key, @NonNull V value) {
                            return 1;
                        }

                        protected void entryRemoved(boolean evicted, @NonNull K key, @NonNull V oldValue,
                                @Nullable V newValue) {
                        }
                    }
                    """
                ),
                androidxNullableSource,
                androidxNonNullSource
            ),
            api = """
                // Signature format: 3.0
                package androidx.core.util {
                  public final class TestKt {
                    method public static inline <K, V> android.util.LruCache<K,V> lruCache(int maxSize, kotlin.jvm.functions.Function2<? super K,? super V,java.lang.Integer> sizeOf = { _, _ -> return 1 }, kotlin.jvm.functions.Function1<? super K,? extends V> create = { it -> return null as V }, kotlin.jvm.functions.Function4<? super java.lang.Boolean,? super K,? super V,? super V,kotlin.Unit> onEntryRemoved = { _, _, _, _ ->  });
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation", ARG_HIDE_PACKAGE, "androidx.collection"),
            includeSignatureVersion = true
        )
    }

    @Test
    fun `Basic Kotlin class`() {
        check(
            format = FileFormat.V1,
            extraArguments = arrayOf("--parameter-names=true"),
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    class Kotlin(val property1: String = "Default Value", arg2: Int) : Parent() {
                        override fun method() = "Hello World"
                        fun otherMethod(ok: Boolean, times: Int) {
                        }

                        var property2: String? = null

                        private var someField = 42
                        @JvmField
                        var someField2 = 42

                        internal var myHiddenVar = false
                        internal fun myHiddenMethod() { }
                        internal data class myHiddenClass(): Unit

                        companion object {
                            const val MY_CONST = 42
                        }
                    }

                    //@get:RequiresApi(26)
                    inline val @receiver:String Long.isSrgb get() = true
                    inline val /*@receiver:ColorInt*/ Int.red get() = 0
                    inline operator fun String.component1() = ""

                    open class Parent {
                        open fun method(): String? = null
                        open fun method2(value: Boolean, value: Boolean?): String? = null
                        open fun method3(value: Int?, value2: Int): Int = null
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class Kotlin extends test.pkg.Parent {
                    ctor public Kotlin(java.lang.String property1, int arg2);
                    method public java.lang.String getProperty1();
                    method public java.lang.String getProperty2();
                    method public void otherMethod(boolean ok, int times);
                    method public void setProperty2(java.lang.String property2);
                    property public final java.lang.String property1;
                    property public final java.lang.String property2;
                    field public static final test.pkg.Kotlin.Companion Companion;
                    field public static final int MY_CONST = 42; // 0x2a
                    field public int someField2;
                  }
                  public static final class Kotlin.Companion {
                  }
                  public final class KotlinKt {
                    method public static inline operator java.lang.String component1(java.lang.String);
                    method public static inline int getRed(int);
                    method public static inline boolean isSrgb(long);
                  }
                  public class Parent {
                    ctor public Parent();
                    method public java.lang.String method();
                    method public java.lang.String method2(boolean value, java.lang.Boolean value);
                    method public int method3(java.lang.Integer value, int value2);
                  }
                }
                """
        )
    }

    @Test
    fun `Kotlin Reified Methods`() {
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    public class Context {
                        @SuppressWarnings("unchecked")
                        public final <T> T getSystemService(Class<T> serviceClass) {
                            return null;
                        }
                    }
                    """
                ),
                kotlin(
                    """
                    package test.pkg

                    inline fun <reified T> Context.systemService1() = getSystemService(T::class.java)
                    inline fun Context.systemService2() = getSystemService(String::class.java)
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Context {
                    ctor public Context();
                    method public final <T> T getSystemService(java.lang.Class<T>);
                  }
                  public final class _java_Kt {
                    method public static inline <reified T> T systemService1(test.pkg.Context);
                    method public static inline java.lang.String systemService2(test.pkg.Context);
                  }
                }
                """
        )
    }

    @Test
    fun `Kotlin Reified Methods 2`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    @file:Suppress("NOTHING_TO_INLINE", "RedundantVisibilityModifier", "unused")

                    package test.pkg

                    inline fun <T> a(t: T) { }
                    inline fun <reified T> b(t: T) { }
                    private inline fun <reified T> c(t: T) { } // hide
                    internal inline fun <reified T> d(t: T) { } // hide
                    public inline fun <reified T> e(t: T) { }
                    inline fun <reified T> T.f(t: T) { }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class TestKt {
                    method public static inline <T> void a(@Nullable T t);
                    method public static inline <reified T> void b(@Nullable T t);
                    method public static inline <reified T> void e(@Nullable T t);
                    method public static inline <reified T> void f(@Nullable T, @Nullable T t);
                  }
                }
                """
        )
    }

    @Test
    fun `Suspend functions`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    suspend inline fun hello(foo: Int) { }
                    suspend fun helloTwoContinuations(myContinuation: kotlin.coroutines.Continuation<Any>) { }
                    internal suspend fun internalHello() { }
                    private suspend fun privateHello() { }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class TestKt {
                    method @Nullable public static suspend inline Object hello(int foo, @NonNull kotlin.coroutines.Continuation<? super kotlin.Unit> p);
                    method @Nullable public static suspend Object helloTwoContinuations(@NonNull kotlin.coroutines.Continuation<java.lang.Object> myContinuation, @NonNull kotlin.coroutines.Continuation<? super kotlin.Unit> p);
                  }
                }
                """
        )
    }

    @Test
    fun `Var properties with private setters`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    class MyClass {
                        // This property should have no public setter
                        var readOnlyVar = false
                            internal set
                        // This property should have no public setter
                        public var readOnlyVarWithPublicModifer = false
                            internal set
                    }
                    """
                )
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class MyClass {
                    ctor public MyClass();
                    method public boolean getReadOnlyVar();
                    method public boolean getReadOnlyVarWithPublicModifer();
                    property public final boolean readOnlyVar;
                    property public final boolean readOnlyVarWithPublicModifer;
                  }
                }
                """
        )
    }

    @Test
    fun `Kotlin Generics`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    class Bar
                    class Type<in T> {
                        fun foo(param: Type<Bar>) {
                        }
                    }
                    """
                )
            ),
            compatibilityMode = false,
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class Bar {
                    ctor public Bar();
                  }
                  public final class Type<T> {
                    ctor public Type();
                    method public void foo(test.pkg.Type<? super test.pkg.Bar> param);
                  }
                }
                """
        )
    }

    @Test
    fun `Nullness in reified signatures`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    "src/test/pkg/test.kt",
                    """
                    package test.pkg

                    import androidx.annotation.UiThread
                    import test.pkg2.NavArgs
                    import test.pkg2.NavArgsLazy
                    import test.pkg2.Fragment
                    import test.pkg2.Bundle

                    @UiThread
                    inline fun <reified Args : NavArgs> Fragment.navArgs() = NavArgsLazy(Args::class) {
                        throw IllegalStateException("Fragment $this has null arguments")
                    }
                    """
                ),
                kotlin(
                    """
                    package test.pkg2

                    import kotlin.reflect.KClass

                    interface NavArgs
                    class Fragment
                    class Bundle
                    class NavArgsLazy<Args : NavArgs>(
                        private val navArgsClass: KClass<Args>,
                        private val argumentProducer: () -> Bundle
                    )
                    """
                ),
                uiThreadSource
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class TestKt {
                    method @UiThread public static inline <reified Args extends test.pkg2.NavArgs> test.pkg2.NavArgsLazy<Args>! navArgs(test.pkg2.Fragment);
                  }
                }
                """,
//            Actual expected API is below. However, due to KT-39209 the nullability information is
//              missing
//            api = """
//                // Signature format: 3.0
//                package test.pkg {
//                  public final class TestKt {
//                    method @UiThread public static inline <reified Args extends test.pkg2.NavArgs> test.pkg2.NavArgsLazy<Args> navArgs(test.pkg2.Fragment);
//                  }
//                }
//                """,
            format = FileFormat.V3,
            extraArguments = arrayOf(
                ARG_HIDE_PACKAGE, "androidx.annotation",
                ARG_HIDE_PACKAGE, "test.pkg2",
                ARG_HIDE, "ReferencesHidden",
                ARG_HIDE, "UnavailableSymbol",
                ARG_HIDE, "HiddenTypeParameter"
            )
        )
    }

    @Test
    fun `Nullness in varargs`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package androidx.collection;

                    import java.util.Collection;
                    import java.util.HashMap;
                    import java.util.Map;

                    public class ArrayMap<K, V> extends HashMap<K, V> implements Map<K, V> {
                        public ArrayMap() {
                        }
                    }
                    """
                ),
                java(
                    """
                    package androidx.core.app;

                    import java.util.ArrayList;
                    import java.util.List;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    public class ActivityOptionsCompat {
                        private ActivityOptionsCompat() {
                        }
                        @NonNull
                        public static List<String> javaListOf(String... sharedElements) {
                            return new ArrayList<String>();
                        }
                        @Nullable
                        public static List<String> javaListOfNullable(String... sharedElements) {
                            return null;
                        }

                    }
                    """
                ),
                kotlin(
                    "src/main/java/androidx/collection/ArrayMap.kt",
                    """
                    package androidx.collection

                    inline fun <K, V> arrayMapOf(): ArrayMap<K, V> = ArrayMap()

                    fun <K, V> arrayMapOf(vararg pairs: Pair<K, V>): ArrayMap<K, V> {
                        val map = ArrayMap<K, V>(pairs.size)
                        for (pair in pairs) {
                            map[pair.first] = pair.second
                        }
                        return map
                    }
                    fun <K, V> arrayMapOfNullable(vararg pairs: Pair<K, V>?): ArrayMap<K, V>? {
                        return null
                    }
                    """
                ),
                androidxNonNullSource,
                androidxNullableSource
            ),
            api = """
                // Signature format: 3.0
                package androidx.collection {
                  public class ArrayMap<K, V> extends java.util.HashMap<K,V> implements java.util.Map<K,V> {
                    ctor public ArrayMap();
                  }
                  public final class ArrayMapKt {
                    method public static inline <K, V> androidx.collection.ArrayMap<K,V> arrayMapOf();
                    method public static <K, V> androidx.collection.ArrayMap<K,V> arrayMapOf(kotlin.Pair<? extends K,? extends V>... pairs);
                    method public static <K, V> androidx.collection.ArrayMap<K,V>? arrayMapOfNullable(kotlin.Pair<? extends K,? extends V>?... pairs);
                  }
                }
                package androidx.core.app {
                  public class ActivityOptionsCompat {
                    method public static java.util.List<java.lang.String!> javaListOf(java.lang.String!...);
                    method public static java.util.List<java.lang.String!>? javaListOfNullable(java.lang.String!...);
                  }
                }
                """,
            format = FileFormat.V3,
            extraArguments = arrayOf(
                ARG_HIDE_PACKAGE, "androidx.annotation",
                ARG_HIDE, "ReferencesHidden",
                ARG_HIDE, "UnavailableSymbol",
                ARG_HIDE, "HiddenTypeParameter"
            )
        )
    }

    @Test
    fun `Propagate Platform types in Kotlin`() {
        check(
            compatibilityMode = false,
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    // Nullable Pair in Kotlin
                    package androidx.util

                    class NullableKotlinPair<out F, out S>(val first: F?, val second: S?)
                    """
                ),
                kotlin(
                    """
                    // Non-nullable Pair in Kotlin
                    package androidx.util
                    class NonNullableKotlinPair<out F: Any, out S: Any>(val first: F, val second: S)
                    """
                ),
                java(
                    """
                    // Platform nullability Pair in Java
                    package androidx.util;

                    @SuppressWarnings("WeakerAccess")
                    public class PlatformJavaPair<F, S> {
                        public final F first;
                        public final S second;

                        public PlatformJavaPair(F first, S second) {
                            this.first = first;
                            this.second = second;
                        }
                    }
                """
                ),
                java(
                    """
                    // Platform nullability Pair in Java
                    package androidx.util;
                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    @SuppressWarnings("WeakerAccess")
                    public class NullableJavaPair<F, S> {
                        public final @Nullable F first;
                        public final @Nullable S second;

                        public NullableJavaPair(@Nullable F first, @Nullable S second) {
                            this.first = first;
                            this.second = second;
                        }
                    }
                    """
                ),
                java(
                    """
                    // Platform nullability Pair in Java
                    package androidx.util;

                    import androidx.annotation.NonNull;

                    @SuppressWarnings("WeakerAccess")
                    public class NonNullableJavaPair<F, S> {
                        public final @NonNull F first;
                        public final @NonNull S second;

                        public NonNullableJavaPair(@NonNull F first, @NonNull S second) {
                            this.first = first;
                            this.second = second;
                        }
                    }
                    """
                ),
                kotlin(
                    """
                    package androidx.util

                    @Suppress("HasPlatformType") // Intentionally propagating platform type with unknown nullability.
                    inline operator fun <F, S> PlatformJavaPair<F, S>.component1() = first
                    """
                ),
                androidxNonNullSource,
                androidxNullableSource
            ),
            api = """
                // Signature format: 3.0
                package androidx.util {
                  public class NonNullableJavaPair<F, S> {
                    ctor public NonNullableJavaPair(F, S);
                    field public final F first;
                    field public final S second;
                  }
                  public final class NonNullableKotlinPair<F, S> {
                    ctor public NonNullableKotlinPair(F first, S second);
                    method public F getFirst();
                    method public S getSecond();
                    property public final F first;
                    property public final S second;
                  }
                  public class NullableJavaPair<F, S> {
                    ctor public NullableJavaPair(F?, S?);
                    field public final F? first;
                    field public final S? second;
                  }
                  public final class NullableKotlinPair<F, S> {
                    ctor public NullableKotlinPair(F? first, S? second);
                    method public F? getFirst();
                    method public S? getSecond();
                    property public final F? first;
                    property public final S? second;
                  }
                  public class PlatformJavaPair<F, S> {
                    ctor public PlatformJavaPair(F!, S!);
                    field public final F! first;
                    field public final S! second;
                  }
                  public final class TestKt {
                    method public static inline operator <F, S> F! component1(androidx.util.PlatformJavaPair<F,S>);
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `Known nullness`() {
        // Don't emit platform types for some unannotated elements that we know the
        // nullness for: annotation type members, equals-parameters, initialized constants, etc.
        check(
            compatibilityMode = false,
            outputKotlinStyleNulls = true,
            sourceFiles = arrayOf(
                java(
                    """
                    // Platform nullability Pair in Java
                    package test;

                    import androidx.annotation.NonNull;

                    public class MyClass {
                        public static final String MY_CONSTANT1 = "constant"; // Not nullable
                        public final String MY_CONSTANT2 = "constant"; // Not nullable
                        public String MY_CONSTANT3 = "constant"; // Unknown

                        /** @deprecated */
                        @Deprecated
                        @Override
                        public boolean equals(
                            Object parameter  // nullable
                        ) {
                            return super.equals(parameter);
                        }

                        /** @deprecated */
                        @Deprecated
                        @Override // Not nullable
                        public String toString() {
                            return super.toString();
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    import static java.lang.annotation.ElementType.*;
                    import java.lang.annotation.*;
                    public @interface MyAnnotation {
                        String[] value(); // Not nullable
                    }
                    """
                ).indented(),
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
                ).indented(),
                kotlin(
                    """
                    package test.pkg
                    class Issue {
                        fun setAndroidSpecific(value: Boolean): Issue { return this }
                        companion object {
                            @JvmStatic
                            fun create(
                                id: String,
                                briefDescription: String,
                                explanation: String
                            ): Issue {
                                return Issue()
                            }
                        }
                    }
                    """
                ).indented(),
                kotlin(
                    """
                    package test.pkg
                    object MySingleton {
                    }
                    """
                ).indented(),
                java(
                    """
                    package test.pkg;
                    public class WrongCallDetector {
                        public static final Issue ISSUE =
                                Issue.create(
                                                "WrongCall",
                                                "Using wrong draw/layout method",
                                                "Custom views typically need to call `measure()`)
                                        .setAndroidSpecific(true));
                    }
                    """
                ).indented(),
                androidxNonNullSource,
                androidxNullableSource
            ),
            api = """
                // Signature format: 3.0
                package test {
                  public class MyClass {
                    ctor public MyClass();
                    method @Deprecated public boolean equals(Object?);
                    method @Deprecated public String toString();
                    field public static final String MY_CONSTANT1 = "constant";
                    field public final String MY_CONSTANT2 = "constant";
                    field public String! MY_CONSTANT3;
                  }
                }
                package test.pkg {
                  public enum Foo {
                    enum_constant public static final test.pkg.Foo A;
                    enum_constant public static final test.pkg.Foo B;
                  }
                  public final class Issue {
                    ctor public Issue();
                    method public static test.pkg.Issue create(String id, String briefDescription, String explanation);
                    method public test.pkg.Issue setAndroidSpecific(boolean value);
                    field public static final test.pkg.Issue.Companion Companion;
                  }
                  public static final class Issue.Companion {
                    method public test.pkg.Issue create(String id, String briefDescription, String explanation);
                  }
                  public enum Language {
                    enum_constant public static final test.pkg.Language JAVA;
                    enum_constant public static final test.pkg.Language KOTLIN;
                  }
                  @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS) public @interface MyAnnotation {
                    method public abstract String[] value();
                  }
                  public final class MySingleton {
                    field public static final test.pkg.MySingleton INSTANCE;
                  }
                  public class WrongCallDetector {
                    ctor public WrongCallDetector();
                    field public static final test.pkg.Issue ISSUE;
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun JvmOverloads() {
        // Regression test for https://github.com/android/android-ktx/issues/366
        check(
            format = FileFormat.V3,
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    """
                        package androidx.content

                        import android.annotation.SuppressLint
                        import android.content.SharedPreferences

                        @SuppressLint("ApplySharedPref")
                        @JvmOverloads
                        inline fun SharedPreferences.edit(
                            commit: Boolean = false,
                            action: SharedPreferences.Editor.() -> Unit
                        ) {
                            val editor = edit()
                            action(editor)
                            if (commit) {
                                editor.commit()
                            } else {
                                editor.apply()
                            }
                        }

                        @JvmOverloads
                        fun String.blahblahblah(firstArg: String = "hello", secondArg: Int = 42, thirdArg: String = "world") {
                        }
                    """
                )
            ),
            api = """
                // Signature format: 3.0
                package androidx.content {
                  public final class TestKt {
                    method public static void blahblahblah(String, String firstArg = "hello", int secondArg = 42, String thirdArg = "world");
                    method public static void blahblahblah(String, String firstArg = "hello", int secondArg = 42);
                    method public static void blahblahblah(String, String firstArg = "hello");
                    method public static void blahblahblah(String);
                    method public static inline void edit(android.content.SharedPreferences, boolean commit = false, kotlin.jvm.functions.Function1<? super android.content.SharedPreferences.Editor,kotlin.Unit> action);
                    method public static inline void edit(android.content.SharedPreferences, kotlin.jvm.functions.Function1<? super android.content.SharedPreferences.Editor,kotlin.Unit> action);
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `Test JvmStatic`() {
        check(
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    class SimpleClass {
                        companion object {
                            @JvmStatic
                            fun jvmStaticMethod() {}
                            fun nonJvmStaticMethod() {}
                        }
                    }
                """
                )
            ),
            format = FileFormat.V3,
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class SimpleClass {
                    ctor public SimpleClass();
                    method public static void jvmStaticMethod();
                    field public static final test.pkg.SimpleClass.Companion Companion;
                  }
                  public static final class SimpleClass.Companion {
                    method public void jvmStaticMethod();
                    method public void nonJvmStaticMethod();
                  }
                }
            """
        )
    }

    @Test
    fun `Test JvmField`() {
        check(
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    class SimpleClass {
                        @JvmField
                        var jvmField = -1

                        var nonJvmField = -2
                    }
                """
                )
            ),
            format = FileFormat.V3,
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class SimpleClass {
                    ctor public SimpleClass();
                    method public int getNonJvmField();
                    method public void setNonJvmField(int nonJvmField);
                    property public final int nonJvmField;
                    field public int jvmField;
                  }
                }
            """
        )
    }

    @Test
    fun `Test JvmName`() {
        check(
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    class SimpleClass {
                        @get:JvmName("myPropertyJvmGetter")
                        var myProperty = -1

                        var anotherProperty = -1
                    }
                """
                )
            ),
            format = FileFormat.V3,
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class SimpleClass {
                    ctor public SimpleClass();
                    method public int getAnotherProperty();
                    method public int myPropertyJvmGetter();
                    method public void setAnotherProperty(int anotherProperty);
                    method public void setMyProperty(int myProperty);
                    property public final int anotherProperty;
                    property public final int myProperty;
                  }
                }
            """
        )
    }

    @Test
    fun `Test RequiresOptIn and OptIn`() {
        check(
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    @RequiresOptIn
                    @Retention(AnnotationRetention.BINARY)
                    @Target(AnnotationTarget.CLASS, AnnotationTarget.FUNCTION)
                    annotation class ExperimentalBar

                    @ExperimentalBar
                    class FancyBar

                    @OptIn(FancyBar::class) // @OptIn should not be tracked as it is not API
                    class SimpleClass {
                        fun methodUsingFancyBar() {
                            val fancyBar = FancyBar()
                        }
                    }
                """
                )
            ),
            format = FileFormat.V3,
            api = """
                // Signature format: 3.0
                package test.pkg {
                  @kotlin.RequiresOptIn @kotlin.annotation.Retention(kotlin.annotation.AnnotationRetention) @kotlin.annotation.Target(allowedTargets={kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget}) public @interface ExperimentalBar {
                  }
                  @test.pkg.ExperimentalBar public final class FancyBar {
                    ctor public FancyBar();
                  }
                  public final class SimpleClass {
                    ctor public SimpleClass();
                    method public void methodUsingFancyBar();
                  }
                }
            """
        )
    }

    @Test
    fun `Test Experimental and UseExperimental`() {
        check(
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    @Experimental
                    @Retention(AnnotationRetention.BINARY)
                    @Target(AnnotationTarget.CLASS, AnnotationTarget.FUNCTION)
                    annotation class ExperimentalBar

                    @ExperimentalBar
                    class FancyBar

                    @UseExperimental(FancyBar::class) // @UseExperimental should not be tracked as it is not API
                    class SimpleClass {
                        fun methodUsingFancyBar() {
                            val fancyBar = FancyBar()
                        }
                    }

                    @androidx.annotation.experimental.UseExperimental(FancyBar::class) // @UseExperimental should not be tracked as it is not API
                    class AnotherSimpleClass {
                        fun methodUsingFancyBar() {
                            val fancyBar = FancyBar()
                        }
                    }
                """
                ),
                kotlin("""
                    package androidx.annotation.experimental

                    import kotlin.annotation.Retention
                    import kotlin.annotation.Target
                    import kotlin.reflect.KClass

                    @Retention(AnnotationRetention.BINARY)
                    @Target(
                        AnnotationTarget.CLASS,
                        AnnotationTarget.PROPERTY,
                        AnnotationTarget.LOCAL_VARIABLE,
                        AnnotationTarget.VALUE_PARAMETER,
                        AnnotationTarget.CONSTRUCTOR,
                        AnnotationTarget.FUNCTION,
                        AnnotationTarget.PROPERTY_GETTER,
                        AnnotationTarget.PROPERTY_SETTER,
                        AnnotationTarget.FILE,
                        AnnotationTarget.TYPEALIAS
                    )
                    annotation class UseExperimental(
                        /**
                         * Defines the experimental API(s) whose usage this annotation allows.
                         */
                        vararg val markerClass: KClass<out Annotation>
                    )
                """)
            ),
            format = FileFormat.V3,
            api = """
                // Signature format: 3.0
                package androidx.annotation.experimental {
                  @kotlin.annotation.Retention(kotlin.annotation.AnnotationRetention) @kotlin.annotation.Target(allowedTargets={kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget}) public @interface UseExperimental {
                    method public abstract kotlin.reflect.KClass<? extends java.lang.annotation.Annotation>[] markerClass();
                    property public abstract kotlin.reflect.KClass<? extends java.lang.annotation.Annotation>![] markerClass;
                  }
                }
                package test.pkg {
                  public final class AnotherSimpleClass {
                    ctor public AnotherSimpleClass();
                    method public void methodUsingFancyBar();
                  }
                  @kotlin.Experimental @kotlin.annotation.Retention(kotlin.annotation.AnnotationRetention) @kotlin.annotation.Target(allowedTargets={kotlin.annotation.AnnotationTarget, kotlin.annotation.AnnotationTarget}) public @interface ExperimentalBar {
                  }
                  @test.pkg.ExperimentalBar public final class FancyBar {
                    ctor public FancyBar();
                  }
                  public final class SimpleClass {
                    ctor public SimpleClass();
                    method public void methodUsingFancyBar();
                  }
                }
            """
        )
    }

    @Test
    fun `Extract class with generics`() {
        // Basic interface with generics; makes sure <T extends Object> is written as just <T>
        // Also include some more complex generics expressions to make sure they're serialized
        // correctly (in particular, using fully qualified names instead of what appears in
        // the source code.)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public interface MyInterface<T extends Object>
                            extends MyBaseInterface {
                    }
                    """
                ), java(
                    """
                    package a.b.c;
                    @SuppressWarnings("ALL")
                    public interface MyStream<T, S extends MyStream<T, S>> extends test.pkg.AutoCloseable {
                    }
                    """
                ), java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public interface MyInterface2<T extends Number>
                            extends MyBaseInterface {
                        class TtsSpan<C extends MyInterface<?>> { }
                        abstract class Range<T extends Comparable<? super T>> {
                            protected String myString;
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface MyBaseInterface {
                        void fun(int a, String b);
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface MyOtherInterface extends MyBaseInterface, AutoCloseable {
                        void fun(int a, String b);
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface AutoCloseable {
                    }
                    """
                )
            ),
            api = """
                    package a.b.c {
                      public abstract interface MyStream<T, S extends a.b.c.MyStream<T, S>> implements test.pkg.AutoCloseable {
                      }
                    }
                    package test.pkg {
                      public abstract interface AutoCloseable {
                      }
                      public abstract interface MyBaseInterface {
                        method public abstract void fun(int, java.lang.String);
                      }
                      public abstract interface MyInterface<T> implements test.pkg.MyBaseInterface {
                      }
                      public abstract interface MyInterface2<T extends java.lang.Number> implements test.pkg.MyBaseInterface {
                      }
                      public static abstract class MyInterface2.Range<T extends java.lang.Comparable<? super T>> {
                        ctor public MyInterface2.Range();
                        field protected java.lang.String myString;
                      }
                      public static class MyInterface2.TtsSpan<C extends test.pkg.MyInterface<?>> {
                        ctor public MyInterface2.TtsSpan();
                      }
                      public abstract interface MyOtherInterface implements test.pkg.AutoCloseable test.pkg.MyBaseInterface {
                      }
                    }
                """,
            extraArguments = arrayOf(ARG_HIDE, "KotlinKeyword")
        )
    }

    @Test
    fun `Basic class without default constructor, has constructors with args`() {
        // Class without private constructors (shouldn't insert default constructor)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Foo {
                        public Foo(int i) {

                        }
                        public Foo(int i, int j) {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Foo {
                    ctor public Foo(int);
                    ctor public Foo(int, int);
                  }
                }
                """
        )
    }

    @Test
    fun `Basic class without default constructor, has private constructor`() {
        // Class without private constructors; no default constructor should be inserted
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class Foo {
                        private Foo() {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Foo {
                  }
                }
                """
        )
    }

    @Test
    fun `Interface class extraction`() {
        // Interface: makes sure the right modifiers etc are shown (and that "package private" methods
        // in the interface are taken to be public etc)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public interface Foo {
                        void foo();
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public abstract interface Foo {
                    method public abstract void foo();
                  }
                }
                """
        )
    }

    @Test
    fun `Enum class extraction`() {
        // Interface: makes sure the right modifiers etc are shown (and that "package private" methods
        // in the interface are taken to be public etc)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public enum Foo {
                        A, B;
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class Foo extends java.lang.Enum {
                    method public static test.pkg.Foo valueOf(java.lang.String) throws java.lang.IllegalArgumentException;
                    method public static final test.pkg.Foo[] values();
                    enum_constant public static final test.pkg.Foo A;
                    enum_constant public static final test.pkg.Foo B;
                  }
                }
                """
        )
    }

    @Test
    fun `Enum class, non-compat mode`() {
        // Interface: makes sure the right modifiers etc are shown (and that "package private" methods
        // in the interface are taken to be public etc)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public enum Foo {
                        A, B;
                    }
                    """
                )
            ),
            compatibilityMode = false,
            api = """
                package test.pkg {
                  public enum Foo {
                    enum_constant public static final test.pkg.Foo A;
                    enum_constant public static final test.pkg.Foo B;
                  }
                }
                """
        )
    }

    @Test
    fun `Annotation class extraction`() {
        // Interface: makes sure the right modifiers etc are shown (and that "package private" methods
        // in the interface are taken to be public etc)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public @interface Foo {
                        String value();
                    }
                    """
                ),
                java(
                    """
                    package android.annotation;
                    import static java.lang.annotation.ElementType.*;
                    import java.lang.annotation.*;
                    @Target({TYPE, FIELD, METHOD, PARAMETER, CONSTRUCTOR, LOCAL_VARIABLE})
                    @Retention(RetentionPolicy.CLASS)
                    @SuppressWarnings("ALL")
                    public @interface SuppressLint {
                        String[] value();
                    }
                """
                )
            ),
            api = """
                package android.annotation {
                  public abstract class SuppressLint implements java.lang.annotation.Annotation {
                  }
                }
                package test.pkg {
                  public abstract class Foo implements java.lang.annotation.Annotation {
                  }
                }
                """
        )
    }

    @Test
    fun `Do not include inherited public methods from private parents in compat mode`() {
        // Real life example: StringBuilder.setLength, in compat mode
        check(
            compatibilityMode = true,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class MyStringBuilder extends AbstractMyStringBuilder {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    class AbstractMyStringBuilder {
                        public void setLength(int length) {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyStringBuilder {
                    ctor public MyStringBuilder();
                  }
                }
                """
        )
    }

    @Test
    fun `Include inherited public methods from private parents`() {
        // In non-compat mode, include public methods from hidden parents too.
        // Real life example: StringBuilder.setLength
        // This is just like the above test, but with compat mode disabled.
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class MyStringBuilder extends AbstractMyStringBuilder {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    class AbstractMyStringBuilder {
                        public void setLength(int length) {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyStringBuilder {
                    ctor public MyStringBuilder();
                    method public void setLength(int);
                  }
                }
                """
        )
    }

    @Test
    fun `Skip inherited package private methods from private parents`() {
        // In non-compat mode, include public methods from hidden parents too.
        // Real life example: StringBuilder.setLength
        // This is just like the above test, but with compat mode disabled.
        check(
            expectedIssues = """
                src/test/pkg/PublicSuper.java:3: error: isContiguous cannot be hidden and abstract when PublicSuper has a visible constructor, in case a third-party attempts to subclass it. [HiddenAbstractMethod]
            """,
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class MyStringBuilder<A,B> extends AbstractMyStringBuilder<A,B> {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    class AbstractMyStringBuilder<C,D> extends PublicSuper<C,D> {
                        public void setLength(int length) {
                        }
                        @Override boolean isContiguous() {
                            return true;
                        }
                        @Override boolean concrete() {
                            return false;
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class PublicSuper<E,F> {
                        abstract boolean isContiguous();
                        boolean concrete() {
                            return false;
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyStringBuilder<A, B> extends test.pkg.PublicSuper<A,B> {
                    ctor public MyStringBuilder();
                    method public void setLength(int);
                  }
                  public class PublicSuper<E, F> {
                    ctor public PublicSuper();
                  }
                }
                """
        )
    }

    @Test
    fun `Annotation class extraction, non-compat mode`() {
        // Interface: makes sure the right modifiers etc are shown (and that "package private" methods
        // in the interface are taken to be public etc)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public @interface Foo {
                        String value();
                    }
                    """
                ),
                java(
                    """
                    package android.annotation;
                    import static java.lang.annotation.ElementType.*;
                    import java.lang.annotation.*;
                    @Target({TYPE, FIELD, METHOD, PARAMETER, CONSTRUCTOR, LOCAL_VARIABLE})
                    @Retention(RetentionPolicy.CLASS)
                    @SuppressWarnings("ALL")
                    public @interface SuppressLint {
                        String[] value();
                    }
                    """
                )
            ),
            compatibilityMode = false,
            api = """
                package android.annotation {
                  @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS) @java.lang.annotation.Target({java.lang.annotation.ElementType.TYPE, java.lang.annotation.ElementType.FIELD, java.lang.annotation.ElementType.METHOD, java.lang.annotation.ElementType.PARAMETER, java.lang.annotation.ElementType.CONSTRUCTOR, java.lang.annotation.ElementType.LOCAL_VARIABLE}) public @interface SuppressLint {
                    method public abstract String[] value();
                  }
                }
                package test.pkg {
                  @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS) public @interface Foo {
                    method public abstract String value();
                  }
                }
                """
        )
    }

    @Test
    fun `Annotation retention`() {
        // For annotations where the java.lang.annotation classes themselves are not
        // part of the source tree, ensure that we compute the right retention (runtime, meaning
        // it should show up in the stubs file.).
        check(
            extraArguments = arrayOf(ARG_EXCLUDE_ALL_ANNOTATIONS),
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public @interface Foo {
                        String value();
                    }
                    """
                ),
                java(
                    """
                    package android.annotation;
                    import static java.lang.annotation.ElementType.*;
                    import java.lang.annotation.*;
                    @Target({TYPE, FIELD, METHOD, PARAMETER, CONSTRUCTOR, LOCAL_VARIABLE})
                    @Retention(RetentionPolicy.CLASS)
                    @SuppressWarnings("ALL")
                    public @interface SuppressLint {
                        String[] value();
                    }
                    """
                ),
                kotlin(
                    """
                    package test.pkg

                    @DslMarker
                    annotation class ImplicitRuntimeRetention

                    @Retention(AnnotationRetention.RUNTIME)
                    annotation class ExplicitRuntimeRetention {
                    }
                    """.trimIndent()
                )
            ),
            format = FileFormat.V3,
            api = """
            // Signature format: 3.0
            package android.annotation {
              @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS) @java.lang.annotation.Target({java.lang.annotation.ElementType.TYPE, java.lang.annotation.ElementType.FIELD, java.lang.annotation.ElementType.METHOD, java.lang.annotation.ElementType.PARAMETER, java.lang.annotation.ElementType.CONSTRUCTOR, java.lang.annotation.ElementType.LOCAL_VARIABLE}) public @interface SuppressLint {
                method public abstract String[] value();
              }
            }
            package test.pkg {
              @kotlin.annotation.Retention(kotlin.annotation.AnnotationRetention) public @interface ExplicitRuntimeRetention {
              }
              @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS) public @interface Foo {
                method public abstract String value();
              }
              @kotlin.DslMarker public @interface ImplicitRuntimeRetention {
              }
            }
            """.trimIndent(),
            compatibilityMode = true,
            stubFiles = arrayOf(
                // For annotations where the java.lang.annotation classes themselves are not
                // part of the source tree, ensure that we compute the right retention (runtime, meaning
                // it should show up in the stubs file.).
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings({"unchecked", "deprecation", "all"})
                    @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS)
                    public @interface Foo {
                    public java.lang.String value();
                    }
                    """
                ),
                java(
                    """
                    package android.annotation;
                    @SuppressWarnings({"unchecked", "deprecation", "all"})
                    @java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.CLASS)
                    @java.lang.annotation.Target({java.lang.annotation.ElementType.TYPE, java.lang.annotation.ElementType.FIELD, java.lang.annotation.ElementType.METHOD, java.lang.annotation.ElementType.PARAMETER, java.lang.annotation.ElementType.CONSTRUCTOR, java.lang.annotation.ElementType.LOCAL_VARIABLE})
                    public @interface SuppressLint {
                    public java.lang.String[] value();
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Superclass signature extraction`() {
        // Make sure superclass statement is correct; inherited method from parent that has same
        // signature isn't included in the child
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class Foo extends Super {
                        @Override public void base() { }
                        public void child() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class Super {
                        public void base() { }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Foo extends test.pkg.Super {
                    ctor public Foo();
                    method public void child();
                  }
                  public class Super {
                    ctor public Super();
                    method public void base();
                  }
                }
                """
        )
    }

    @Test
    fun `Extract fields with types and initial values`() {
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class Foo {
                        private int hidden = 1;
                        int hidden2 = 2;
                        /** @hide */
                        int hidden3 = 3;

                        protected int field00; // No value
                        public static final boolean field01 = true;
                        public static final int field02 = 42;
                        public static final long field03 = 42L;
                        public static final short field04 = 5;
                        public static final byte field05 = 5;
                        public static final char field06 = 'c';
                        public static final float field07 = 98.5f;
                        public static final double field08 = 98.5;
                        public static final String field09 = "String with \"escapes\" and \u00a9...";
                        public static final double field10 = Double.NaN;
                        public static final double field11 = Double.POSITIVE_INFINITY;

                        public static final String GOOD_IRI_CHAR = "a-zA-Z0-9\u00a0-\ud7ff\uf900-\ufdcf\ufdf0-\uffef";
                        public static final char HEX_INPUT = 61184;
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Foo {
                    ctor public Foo();
                    field public static final java.lang.String GOOD_IRI_CHAR = "a-zA-Z0-9\u00a0-\ud7ff\uf900-\ufdcf\ufdf0-\uffef";
                    field public static final char HEX_INPUT = 61184; // 0xef00 '\uef00'
                    field protected int field00;
                    field public static final boolean field01 = true;
                    field public static final int field02 = 42; // 0x2a
                    field public static final long field03 = 42L; // 0x2aL
                    field public static final short field04 = 5; // 0x5
                    field public static final byte field05 = 5; // 0x5
                    field public static final char field06 = 99; // 0x0063 'c'
                    field public static final float field07 = 98.5f;
                    field public static final double field08 = 98.5;
                    field public static final java.lang.String field09 = "String with \"escapes\" and \u00a9...";
                    field public static final double field10 = (0.0/0.0);
                    field public static final double field11 = (1.0/0.0);
                  }
                }
                """
        )
    }

    @Test
    fun `Check all modifiers`() {
        // Include as many modifiers as possible to see which ones are included
        // in the signature files, and the expected sorting order.
        // Note that the signature files treat "deprecated" as a fake modifier.
        // Note also how the "protected" modifier on the interface method gets
        // promoted to public.
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public abstract class Foo {
                        @Deprecated private static final long field1 = 5;
                        @Deprecated private static volatile long field2 = 5;
                        @Deprecated public static strictfp final synchronized void method1() { }
                        @Deprecated public static final synchronized native void method2();
                        @Deprecated protected static final class Inner1 { }
                        @Deprecated protected static abstract  class Inner2 { }
                        @Deprecated protected interface Inner3 {
                            default void method3() { }
                            static void method4(final int arg) { }
                        }
                    }
                    """
                )
            ),

            expectedIssues = """
                src/test/pkg/Foo.java:7: error: Method test.pkg.Foo.method1(): @Deprecated annotation (present) and @deprecated doc tag (not present) do not match [DeprecationMismatch]
                src/test/pkg/Foo.java:8: error: Method test.pkg.Foo.method2(): @Deprecated annotation (present) and @deprecated doc tag (not present) do not match [DeprecationMismatch]
                src/test/pkg/Foo.java:9: error: Class test.pkg.Foo.Inner1: @Deprecated annotation (present) and @deprecated doc tag (not present) do not match [DeprecationMismatch]
                src/test/pkg/Foo.java:10: error: Class test.pkg.Foo.Inner2: @Deprecated annotation (present) and @deprecated doc tag (not present) do not match [DeprecationMismatch]
                src/test/pkg/Foo.java:11: error: Class test.pkg.Foo.Inner3: @Deprecated annotation (present) and @deprecated doc tag (not present) do not match [DeprecationMismatch]
                """,

            api = """
                    package test.pkg {
                      public abstract class Foo {
                        ctor public Foo();
                        method public static final deprecated synchronized void method1();
                        method public static final deprecated synchronized void method2();
                      }
                      protected static final deprecated class Foo.Inner1 {
                        ctor protected Foo.Inner1();
                      }
                      protected static abstract deprecated class Foo.Inner2 {
                        ctor protected Foo.Inner2();
                      }
                      protected static abstract deprecated interface Foo.Inner3 {
                        method public default void method3();
                        method public static void method4(int);
                      }
                    }
                """
        )
    }

    @Test
    fun `Warn about findViewById`() {
        // Include as many modifiers as possible to see which ones are included
        // in the signature files, and the expected sorting order.
        // Note that the signature files treat "deprecated" as a fake modifier.
        // Note also how the "protected" modifier on the interface method gets
        // promoted to public.
        check(
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.Nullable;

                    @SuppressWarnings("ALL")
                    public abstract class Foo {
                        @Nullable public String findViewById(int id) { return ""; }
                    }
                    """
                ),
                nullableSource
            ),

            expectedIssues = """
                src/test/pkg/Foo.java:6: warning: method test.pkg.Foo.findViewById(int) should not be annotated @Nullable; it should be left unspecified to make it a platform type [ExpectedPlatformType]
                """,
            extraArguments = arrayOf(ARG_WARNING, "ExpectedPlatformType"),
            api = """
                package test.pkg {
                  public abstract class Foo {
                    ctor public Foo();
                    method public String findViewById(int);
                  }
                }
                """
        )
    }

    @Test
    fun `Check all modifiers, non-compat mode`() {
        // Like testModifiers but turns off compat mode, such that we have
        // a modifier order more in line with standard code conventions
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public abstract class Foo {
                        @Deprecated private static final long field1 = 5;
                        @Deprecated private static volatile long field2 = 5;
                        /** @deprecated */ @Deprecated public static strictfp final synchronized void method1() { }
                        /** @deprecated */ @Deprecated public static final synchronized native void method2();
                        /** @deprecated */ @Deprecated protected static final class Inner1 { }
                        /** @deprecated */ @Deprecated protected static abstract class Inner2 { }
                        /** @deprecated */ @Deprecated protected interface Inner3 {
                            protected default void method3() { }
                            static void method4(final int arg) { }
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public abstract class Foo {
                    ctor public Foo();
                    method @Deprecated public static final void method1();
                    method @Deprecated public static final void method2();
                  }
                  @Deprecated protected static final class Foo.Inner1 {
                    ctor @Deprecated protected Foo.Inner1();
                  }
                  @Deprecated protected abstract static class Foo.Inner2 {
                    ctor @Deprecated protected Foo.Inner2();
                  }
                  @Deprecated protected static interface Foo.Inner3 {
                    method @Deprecated public default void method3();
                    method @Deprecated public static void method4(int);
                  }
                }
                """
        )
    }

    @Test
    fun `Package with only hidden classes should be removed from signature files`() {
        // Checks that if we have packages that are hidden, or contain only hidden or doconly
        // classes, the entire package is omitted from the signature file. Note how the test.pkg1.sub
        // package is not marked @hide, but doclava now treats subpackages of a hidden package
        // as also hidden.
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    ${"/** @hide hidden package */" /* avoid dangling javadoc warning */}
                    package test.pkg1;
                    """
                ),
                java(
                    """
                    package test.pkg1;
                    @SuppressWarnings("ALL")
                    public class Foo {
                        // Hidden by package hide
                    }
                    """
                ),
                java(
                    """
                    package test.pkg2;
                    /** @hide hidden class in this package */
                    @SuppressWarnings("ALL")
                    public class Bar {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg2;
                    /** @doconly hidden class in this package */
                    @SuppressWarnings("ALL")
                    public class Baz {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg1.sub;
                    // Hidden by @hide in package above
                    @SuppressWarnings("ALL")
                    public class Test {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg3;
                    // The only really visible class
                    @SuppressWarnings("ALL")
                    public class Boo {
                    }
                    """
                )
            ),
            api = """
                package test.pkg3 {
                  public class Boo {
                    ctor public Boo();
                  }
                }
                """
        )
    }

    @Test
    fun `Enums can be abstract`() {
        // As per https://bugs.openjdk.java.net/browse/JDK-6287639
        // abstract methods in enums should not be listed as abstract,
        // but doclava1 does, so replicate this.
        // Also checks that we handle both enum fields and regular fields
        // and that they are listed separately.

        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public enum FooBar {
                        ABC {
                            @Override
                            protected void foo() { }
                        }, DEF {
                            @Override
                            protected void foo() { }
                        };

                        protected abstract void foo();
                        public static int field1 = 1;
                        public int field2 = 2;
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class FooBar extends java.lang.Enum {
                    method protected abstract void foo();
                    method public static test.pkg.FooBar valueOf(java.lang.String) throws java.lang.IllegalArgumentException;
                    method public static final test.pkg.FooBar[] values();
                    enum_constant public static final test.pkg.FooBar ABC;
                    enum_constant public static final test.pkg.FooBar DEF;
                    field public static int field1;
                    field public int field2;
                  }
                }
            """
        )
    }

    @Test
    fun `Check erasure in throws-list`() {
        // Makes sure that when we have a generic signature in the throws list we take
        // the erasure instead (in compat mode); "Throwable" instead of "X" in the below
        // test. Real world example: Optional.orElseThrow.
        check(
            compatibilityMode = true,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    import java.util.function.Supplier;

                    @SuppressWarnings("ALL")
                    public final class Test<T> {
                        public <X extends Throwable> T orElseThrow(Supplier<? extends X> exceptionSupplier) throws X {
                            return null;
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class Test<T> {
                    ctor public Test();
                    method public <X extends java.lang.Throwable> T orElseThrow(java.util.function.Supplier<? extends X>) throws java.lang.Throwable;
                  }
                }
                """
        )
    }

    @Test
    fun `Check various generics signature subtleties`() {
        // Some additional declarations where PSI default type handling diffs from doclava1
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public abstract class Collections {
                        public static <T extends java.lang.Object & java.lang.Comparable<? super T>> T max(java.util.Collection<? extends T> collection) {
                            return null;
                        }
                        public abstract <T extends java.util.Collection<java.lang.String>> T addAllTo(T t);
                        public final class Range<T extends java.lang.Comparable<? super T>> { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    import java.util.Set;

                    @SuppressWarnings("ALL")
                    public class MoreAsserts {
                        public static void assertEquals(String arg0, Set<? extends Object> arg1, Set<? extends Object> arg2) { }
                        public static void assertEquals(Set<? extends Object> arg1, Set<? extends Object> arg2) { }
                    }

                    """
                )
            ),

            // This is the output from doclava1; I'm not quite matching this yet (sorting order differs,
            // and my heuristic to remove "extends java.lang.Object" is somehow preserved here. I'm
            // not clear on when they do it and when they don't.
            /*
            api = """
            package test.pkg {
              public abstract class Collections {
                ctor public Collections();
                method public abstract <T extends java.util.Collection<java.lang.String>> T addAllTo(T);
                method public static <T & java.lang.Comparable<? super T>> T max(java.util.Collection<? extends T>);
              }
              public final class Collections.Range<T extends java.lang.Comparable<? super T>> {
                ctor public Collections.Range();
              }
              public class MoreAsserts {
                ctor public MoreAsserts();
                method public static void assertEquals(java.util.Set<? extends java.lang.Object>, java.util.Set<? extends java.lang.Object>);
                method public static void assertEquals(java.lang.String, java.util.Set<? extends java.lang.Object>, java.util.Set<? extends java.lang.Object>);
              }
            }
            """,
            */
            api = """
                package test.pkg {
                  public abstract class Collections {
                    ctor public Collections();
                    method public abstract <T extends java.util.Collection<java.lang.String>> T addAllTo(T);
                    method public static <T extends java.lang.Object & java.lang.Comparable<? super T>> T max(java.util.Collection<? extends T>);
                  }
                  public final class Collections.Range<T extends java.lang.Comparable<? super T>> {
                    ctor public Collections.Range();
                  }
                  public class MoreAsserts {
                    ctor public MoreAsserts();
                    method public static void assertEquals(java.lang.String, java.util.Set<?>, java.util.Set<?>);
                    method public static void assertEquals(java.util.Set<?>, java.util.Set<?>);
                  }
                }
                """
        )
    }

    @Test
    fun `Check instance methods in enums`() {
        // Make sure that when we have instance methods in an enum they're handled
        // correctly (there's some special casing around enums to insert extra methods
        // that was broken, as exposed by ChronoUnit#toString)
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public interface TempUnit {
                        @Override
                        String toString();
                    }
                     """
                ),
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("ALL")
                    public enum ChronUnit implements TempUnit {
                        C, B, A;

                        public String valueOf(int x) {
                            return Integer.toString(x + 5);
                        }

                        public String values(String separator) {
                            return null;
                        }

                        @Override
                        public String toString() {
                            return name();
                        }
                    }
                """
                )
            ),
            importedPackages = emptyList(),
            api = """
                package test.pkg {
                  public final class ChronUnit extends java.lang.Enum implements test.pkg.TempUnit {
                    method public java.lang.String valueOf(int);
                    method public static test.pkg.ChronUnit valueOf(java.lang.String) throws java.lang.IllegalArgumentException;
                    method public final java.lang.String values(java.lang.String);
                    method public static final test.pkg.ChronUnit[] values();
                    enum_constant public static final test.pkg.ChronUnit A;
                    enum_constant public static final test.pkg.ChronUnit B;
                    enum_constant public static final test.pkg.ChronUnit C;
                  }
                  public abstract interface TempUnit {
                    method public abstract java.lang.String toString();
                  }
                }
                """
        )
    }

    @Test
    fun `Mixing enums and fields`() {
        // Checks sorting order of enum constant values
        val source = """
            package java.nio.file.attribute {
              public final class AclEntryPermission extends java.lang.Enum {
                method public static java.nio.file.attribute.AclEntryPermission valueOf(java.lang.String);
                method public static final java.nio.file.attribute.AclEntryPermission[] values();
                enum_constant public static final java.nio.file.attribute.AclEntryPermission APPEND_DATA;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission DELETE;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission DELETE_CHILD;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission EXECUTE;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission READ_ACL;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission READ_ATTRIBUTES;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission READ_DATA;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission READ_NAMED_ATTRS;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission SYNCHRONIZE;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission WRITE_ACL;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission WRITE_ATTRIBUTES;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission WRITE_DATA;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission WRITE_NAMED_ATTRS;
                enum_constant public static final java.nio.file.attribute.AclEntryPermission WRITE_OWNER;
                field public static final java.nio.file.attribute.AclEntryPermission ADD_FILE;
                field public static final java.nio.file.attribute.AclEntryPermission ADD_SUBDIRECTORY;
                field public static final java.nio.file.attribute.AclEntryPermission LIST_DIRECTORY;
              }
            }
                    """
        check(
            signatureSource = source,
            api = source
        )
    }

    @Test
    fun `Superclass filtering, should skip intermediate hidden classes`() {
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class MyClass extends HiddenParent {
                        public void method4() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    /** @hide */
                    @SuppressWarnings("ALL")
                    public class HiddenParent extends HiddenParent2 {
                        public static final String CONSTANT = "MyConstant";
                        protected int mContext;
                        public void method3() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    /** @hide */
                    @SuppressWarnings("ALL")
                    public class HiddenParent2 extends PublicParent {
                        public void method2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class PublicParent {
                        public void method1() { }
                    }
                    """
                )
            ),
            // Notice how the intermediate methods (method2, method3) have been removed
            includeStrippedSuperclassWarnings = true,
            expectedIssues = "src/test/pkg/MyClass.java:2: warning: Public class test.pkg.MyClass stripped of unavailable superclass test.pkg.HiddenParent [HiddenSuperclass]",
            api = """
                package test.pkg {
                  public class MyClass extends test.pkg.PublicParent {
                    ctor public MyClass();
                    method public void method4();
                  }
                  public class PublicParent {
                    ctor public PublicParent();
                    method public void method1();
                  }
                }
                """
        )
    }

    @Test
    fun `Inheriting from package private classes, package private class should be included`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class MyClass extends HiddenParent {
                        public void method1() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    class HiddenParent {
                        public static final String CONSTANT = "MyConstant";
                        protected int mContext;
                        public void method2() { }
                    }
                    """
                )
            ),
            expectedIssues = "",
            api = """
                    package test.pkg {
                      public class MyClass {
                        ctor public MyClass();
                        method public void method1();
                        method public void method2();
                        field public static final String CONSTANT = "MyConstant";
                      }
                    }
            """
        )
    }

    @Test
    fun `Inheriting generic method from package private class`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class MyClass extends HiddenParent {
                        public void method1() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    class HiddenParent {
                        public <T> T method2(T t) { }
                        public String method3(String s) { }
                    }
                    """
                )
            ),
            expectedIssues = "",
            api = """
                    package test.pkg {
                      public class MyClass {
                        ctor public MyClass();
                        method public void method1();
                        method public <T> T method2(T);
                        method public String method3(String);
                      }
                    }
            """
        )
    }

    @Test
    fun `Type substitution for generic method referencing parent type parameter`() {
        // Type parameters from parent classes need to be replaced with their bounds in the child.
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class MyClass extends HiddenParent<String> {
                        public void method1() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    class HiddenParent<T> {
                        public T method2(T t) { }
                    }
                    """
                )
            ),
            expectedIssues = "",
            api = """
                    package test.pkg {
                      public class MyClass {
                        ctor public MyClass();
                        method public void method1();
                        method public String method2(String);
                      }
                    }
            """
        )
    }

    @Test
    fun `Using compatibility flag manually`() {
        // Like previous test, but using compatibility mode and explicitly turning on
        // the hidden super class compatibility flag. This test is mostly intended
        // to test the flag handling for individual compatibility flags.
        check(
            compatibilityMode = true,
            extraArguments = arrayOf("--skip-inherited-methods=false"),
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public class MyClass extends HiddenParent {
                        public void method1() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    class HiddenParent {
                        public static final String CONSTANT = "MyConstant";
                        protected int mContext;
                        public void method2() { }
                    }
                    """
                )
            ),
            expectedIssues = "",
            api = """
                    package test.pkg {
                      public class MyClass {
                        ctor public MyClass();
                        method public void method1();
                        method public void method2();
                      }
                    }
            """
        )
    }

    @Test
    fun `When implementing rather than extending package private class, inline members instead`() {
        // If you implement a package private interface, we just remove it and inline the members into
        // the subclass
        check(
            compatibilityMode = true,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class MyClass implements HiddenInterface {
                        @Override public void method() { }
                        @Override public void other() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface OtherInterface {
                        void other();
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    interface HiddenInterface extends OtherInterface {
                        void method() { }
                        String CONSTANT = "MyConstant";
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyClass implements test.pkg.OtherInterface {
                    ctor public MyClass();
                    method public void method();
                    method public void other();
                    field public static final java.lang.String CONSTANT = "MyConstant";
                  }
                  public abstract interface OtherInterface {
                    method public abstract void other();
                  }
                }
                """
        )
    }

    @Test
    fun `Implementing package private class, non-compat mode`() {
        // Like the previous test, but in non compat mode we correctly
        // include all the non-hidden public interfaces into the signature

        // BUG: Note that we need to implement the parent
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class MyClass implements HiddenInterface {
                        @Override public void method() { }
                        @Override public void other() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface OtherInterface {
                        void other();
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    interface HiddenInterface extends OtherInterface {
                        void method() { }
                        String CONSTANT = "MyConstant";
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyClass implements test.pkg.OtherInterface {
                    ctor public MyClass();
                    method public void method();
                    method public void other();
                    field public static final String CONSTANT = "MyConstant";
                  }
                  public interface OtherInterface {
                    method public void other();
                  }
                }
                """
        )
    }

    @Test
    fun `Default modifiers should be omitted`() {
        // If signatures vary only by the "default" modifier in the interface, don't show it on the implementing
        // class
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    public class MyClass implements SuperInterface {
                        @Override public void method() {  }
                        @Override public void method2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    public interface SuperInterface {
                        void method();
                        default void method2() {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class MyClass implements test.pkg.SuperInterface {
                    ctor public MyClass();
                    method public void method();
                  }
                  public abstract interface SuperInterface {
                    method public abstract void method();
                    method public default void method2();
                  }
                }
            """
        )
    }

    @Test
    fun `Override via different throws list should be included`() {
        // If a method overrides another but changes the throws list, the overriding
        // method must be listed in the subclass. This is observed for example in
        // AbstractCursor#finalize, which omits the throws clause from Object's finalize.
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    public abstract class AbstractCursor extends Parent {
                        @Override protected void finalize2() {  } // note: not throws Throwable!
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    @SuppressWarnings("RedundantThrows")
                    public class Parent {
                        protected void finalize2() throws Throwable {
                        }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public abstract class AbstractCursor extends test.pkg.Parent {
                    ctor public AbstractCursor();
                    method protected void finalize2();
                  }
                  public class Parent {
                    ctor public Parent();
                    method protected void finalize2() throws java.lang.Throwable;
                  }
                }
            """
        )
    }

    @Test
    fun `Implementing interface method`() {
        // If you have a public method that implements an interface method,
        // they'll vary in the "abstract" modifier, but it shouldn't be listed on the
        // class. This is an issue for example for the ZonedDateTime#getLong method
        // implementing the TemporalAccessor#getLong method
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public interface SomeInterface2 {
                        @Override default long getLong() {
                            return 42;
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Foo implements SomeInterface2 {
                        @Override
                        public long getLong() { return 0L; }
                    }
                    """
                )
            ),
            api = """
            package test.pkg {
              public class Foo implements test.pkg.SomeInterface2 {
                ctor public Foo();
              }
              public abstract interface SomeInterface2 {
                method public default long getLong();
              }
            }
        """
        )
    }

    @Test
    fun `Implementing interface method 2`() {
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public interface SomeInterface {
                        long getLong();
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public interface SomeInterface2 {
                        @Override default long getLong() {
                            return 42;
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Foo implements SomeInterface, SomeInterface2 {
                        @Override
                        public long getLong() { return 0L; }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public class Foo implements test.pkg.SomeInterface test.pkg.SomeInterface2 {
                    ctor public Foo();
                  }
                  public abstract interface SomeInterface {
                    method public abstract long getLong();
                  }
                  public abstract interface SomeInterface2 {
                    method public default long getLong();
                  }
                }
                """
        )
    }

    @Test
    fun `Check basic @remove scenarios`() {
        // Test basic @remove handling for methods and fields
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("JavaDoc")
                    public class Bar {
                        /** @removed */
                        public Bar() { }
                        public int field;
                        public void test() { }
                        /** @removed */
                        public int removedField;
                        /** @removed */
                        public void removedMethod() { }
                        /** @removed and @hide - should not be listed */
                        public int hiddenField;

                        /** @removed */
                        public class Inner { }

                        public class Inner2 {
                            public class Inner3 {
                                /** @removed */
                                public class Inner4 { }
                            }
                        }

                        public class Inner5 {
                            public class Inner6 {
                                public class Inner7 {
                                    /** @removed */
                                    public int removed;
                                }
                            }
                        }
                    }
                    """
                )
            ),
            removedApi = """
                package test.pkg {
                  public class Bar {
                    ctor public Bar();
                    method public void removedMethod();
                    field public int removedField;
                  }
                  public class Bar.Inner {
                    ctor public Bar.Inner();
                  }
                  public class Bar.Inner2.Inner3.Inner4 {
                    ctor public Bar.Inner2.Inner3.Inner4();
                  }
                  public class Bar.Inner5.Inner6.Inner7 {
                    field public int removed;
                  }
                }
                """,
            removedDexApi = "" +
                "Ltest/pkg/Bar;-><init>()V\n" +
                "Ltest/pkg/Bar;->removedMethod()V\n" +
                "Ltest/pkg/Bar;->removedField:I\n" +
                "Ltest/pkg/Bar\$Inner;\n" +
                "Ltest/pkg/Bar\$Inner;-><init>()V\n" +
                "Ltest/pkg/Bar\$Inner2\$Inner3\$Inner4;\n" +
                "Ltest/pkg/Bar\$Inner2\$Inner3\$Inner4;-><init>()V\n" +
                "Ltest/pkg/Bar\$Inner5\$Inner6\$Inner7;->removed:I"
        )
    }

    @Test
    fun `Check @remove class`() {
        // Test removing classes
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    /** @removed */
                    @SuppressWarnings("JavaDoc")
                    public class Foo {
                        public void foo() { }
                        public class Inner {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("JavaDoc")
                    public class Bar implements Parcelable {
                        public int field;
                        public void method();

                        /** @removed */
                        public int removedField;
                        /** @removed */
                        public void removedMethod() { }

                        public class Inner1 {
                        }
                        /** @removed */
                        public class Inner2 {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("ALL")
                    public interface Parcelable {
                        void method();
                    }
                    """
                )
            ),
            /*
            I expected this: but doclava1 doesn't do that (and we now match its behavior)
            package test.pkg {
              public class Bar {
                method public void removedMethod();
                field public int removedField;
              }
              public class Bar.Inner2 {
              }
              public class Foo {
                method public void foo();
              }
            }
             */
            removedApi = """
                    package test.pkg {
                      public class Bar implements test.pkg.Parcelable {
                        method public void removedMethod();
                        field public int removedField;
                      }
                      public class Bar.Inner2 {
                        ctor public Bar.Inner2();
                      }
                      public class Foo {
                        ctor public Foo();
                        method public void foo();
                      }
                      public class Foo.Inner {
                        ctor public Foo.Inner();
                      }
                    }
                """
        )
    }

    @Test
    fun `Test include overridden @Deprecated even if annotated with @hide`() {
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    @SuppressWarnings("JavaDoc")
                    public class Child extends Parent {
                        /**
                        * @deprecated
                        * @hide
                        */
                        @Deprecated @Override
                        public String toString() {
                            return "Child";
                        }

                        /**
                         * @hide
                         */
                        public void hiddenApi() {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Parent {
                        public String toString() {
                            return "Parent";
                        }
                    }
                    """
                )
            ),
            api = """
                    package test.pkg {
                      public class Child extends test.pkg.Parent {
                        ctor public Child();
                        method public deprecated java.lang.String toString();
                      }
                      public class Parent {
                        ctor public Parent();
                      }
                    }
                    """,
            dexApi = """
                Ltest/pkg/Child;
                Ltest/pkg/Child;-><init>()V
                Ltest/pkg/Child;->toString()Ljava/lang/String;
                Ltest/pkg/Parent;
                Ltest/pkg/Parent;-><init>()V
                Ltest/pkg/Parent;->toString()Ljava/lang/String;
            """
        )
    }

    @Test
    fun `Test invalid class name`() {
        // Regression test for b/73018978
        check(
            sourceFiles = arrayOf(
                kotlin(
                    "src/test/pkg/Foo.kt",
                    """
                    @file:JvmName("-Foo")

                    package test.pkg

                    @Suppress("unused")
                    inline fun String.printHelloWorld() { println("Hello World") }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class -Foo {
                    method public static inline void printHelloWorld(java.lang.String);
                  }
                }
                """
        )
    }

    @Test
    fun `Indirect Field Includes from Interfaces`() {
        // Real-world example: include ZipConstants into ZipFile and JarFile
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg1;
                    interface MyConstants {
                        long CONSTANT1 = 12345;
                        long CONSTANT2 = 67890;
                        long CONSTANT3 = 42;
                    }
                    """
                ),
                java(
                    """
                    package test.pkg1;
                    import java.io.Closeable;
                    @SuppressWarnings("WeakerAccess")
                    public class MyParent implements MyConstants, Closeable {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg2;

                    import test.pkg1.MyParent;
                    public class MyChild extends MyParent {
                    }
                    """
                )

            ),
            api = """
                    package test.pkg1 {
                      public class MyParent implements java.io.Closeable {
                        ctor public MyParent();
                        field public static final long CONSTANT1 = 12345L; // 0x3039L
                        field public static final long CONSTANT2 = 67890L; // 0x10932L
                        field public static final long CONSTANT3 = 42L; // 0x2aL
                      }
                    }
                    package test.pkg2 {
                      public class MyChild extends test.pkg1.MyParent {
                        ctor public MyChild();
                        field public static final long CONSTANT1 = 12345L; // 0x3039L
                        field public static final long CONSTANT2 = 67890L; // 0x10932L
                        field public static final long CONSTANT3 = 42L; // 0x2aL
                      }
                    }
                """
        )
    }

    @Test
    fun `Skip interfaces from packages explicitly hidden via arguments`() {
        // Real-world example: HttpResponseCache implements OkCacheContainer but hides the only inherited method
        check(
            extraArguments = arrayOf(
                ARG_HIDE_PACKAGE, "com.squareup.okhttp"
            ),
            sourceFiles = arrayOf(
                java(
                    """
                    package android.net.http;
                    import com.squareup.okhttp.Cache;
                    import com.squareup.okhttp.OkCacheContainer;
                    import java.io.Closeable;
                    import java.net.ResponseCache;
                    @SuppressWarnings("JavaDoc")
                    public final class HttpResponseCache implements Closeable, OkCacheContainer {
                        /** @hide Needed for OkHttp integration. */
                        @Override
                        public Cache getCache() {
                            return delegate.getCache();
                        }
                    }
                    """
                ),
                java(
                    """
                    package com.squareup.okhttp;
                    public interface OkCacheContainer {
                      Cache getCache();
                    }
                    """
                ),
                java(
                    """
                    package com.squareup.okhttp;
                    public class Cache {
                    }
                    """
                )
            ),
            api = """
                package android.net.http {
                  public final class HttpResponseCache implements java.io.Closeable {
                    ctor public HttpResponseCache();
                  }
                }
                """
        )
    }

    @Test
    fun `Extend from multiple interfaces`() {
        // Real-world example: XmlResourceParser
        check(
            checkCompilation = true,
            sourceFiles = arrayOf(
                java(
                    """
                    package android.content.res;
                    import android.util.AttributeSet;
                    import org.xmlpull.v1.XmlPullParser;
                    import my.AutoCloseable;

                    @SuppressWarnings("UnnecessaryInterfaceModifier")
                    public interface XmlResourceParser extends XmlPullParser, AttributeSet, AutoCloseable {
                        public void close();
                    }
                    """
                ),
                java(
                    """
                    package android.util;
                    @SuppressWarnings("WeakerAccess")
                    public interface AttributeSet {
                    }
                    """
                ),
                java(
                    """
                    package my;
                    public interface AutoCloseable {
                    }
                    """
                ),
                java(
                    """
                    package org.xmlpull.v1;
                    @SuppressWarnings("WeakerAccess")
                    public interface XmlPullParser {
                    }
                    """
                )
            ),
            api = """
                package android.content.res {
                  public abstract interface XmlResourceParser implements android.util.AttributeSet my.AutoCloseable org.xmlpull.v1.XmlPullParser {
                    method public abstract void close();
                  }
                }
                package android.util {
                  public abstract interface AttributeSet {
                  }
                }
                package my {
                  public abstract interface AutoCloseable {
                  }
                }
                package org.xmlpull.v1 {
                  public abstract interface XmlPullParser {
                  }
                }
                """
        )
    }

    @Test
    fun `Test KDoc suppress`() {
        // Basic class; also checks that default constructor is made explicit
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Foo {
                        private Foo() { }
                        /** @suppress */
                        public void hidden() {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    /**
                    * Some comment.
                    * @suppress
                    */
                    public class Hidden {
                        private Hidden() { }
                        public void hidden() {
                        }
                        public class Inner {
                        }
                    }
                    """
                )
            ),
            api = """
                    package test.pkg {
                      public class Foo {
                      }
                    }
                """
        )
    }

    @Test
    fun `Check skipping implicit final or deprecated override`() {
        // Regression test for 122358225
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;

                    public class Parent {
                        public void foo1() { }
                        public void foo2() { }
                        public void foo3() { }
                        public void foo4() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    public final class Child1 extends Parent {
                        private Child1() { }
                        public final void foo1() { }
                        public void foo2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    /** @deprecated */
                    @Deprecated
                    public final class Child2 extends Parent {
                        private Child2() { }
                        /** @deprecated */
                        @Deprecated
                        public void foo3() { }
                        public void foo4() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;

                    /** @deprecated */
                    @Deprecated
                    public final class Child3 extends Parent {
                        private Child3() { }
                        public final void foo1() { }
                        public void foo2() { }
                        /** @deprecated */
                        @Deprecated
                        public void foo3() { }
                        /** @deprecated */
                        @Deprecated
                        public final void foo4() { }
                    }
                    """
                )
            ),
            api = """
                package test.pkg {
                  public final class Child1 extends test.pkg.Parent {
                  }
                  @Deprecated public final class Child2 extends test.pkg.Parent {
                  }
                  @Deprecated public final class Child3 extends test.pkg.Parent {
                  }
                  public class Parent {
                    ctor public Parent();
                    method public void foo1();
                    method public void foo2();
                    method public void foo3();
                    method public void foo4();
                  }
                }
                """
        )
    }

    @Test
    fun `Ignore synchronized differences`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg2;

                    public class Parent {
                        public void foo1() { }
                        public synchronized void foo2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg2;

                    public class Child1 extends Parent {
                        private Child1() { }
                        public synchronized void foo1() { }
                        public void foo2() { }
                    }
                    """
                )
            ),
            api = """
                package test.pkg2 {
                  public class Child1 extends test.pkg2.Parent {
                  }
                  public class Parent {
                    ctor public Parent();
                    method public void foo1();
                    method public void foo2();
                  }
                }
                """
        )
    }

    @Test
    fun `Skip incorrect inherit`() {
        check(
            // Simulate test-mock scenario for getIContentProvider
            extraArguments = arrayOf("--stub-packages", "android.test.mock"),
            compatibilityMode = false,
            expectedIssues = "src/android/test/mock/MockContentProvider.java:6: warning: Public class android.test.mock.MockContentProvider stripped of unavailable superclass android.content.ContentProvider [HiddenSuperclass]",
            sourceFiles = arrayOf(
                java(
                    """
                    package android.test.mock;

                    import android.content.ContentProvider;
                    import android.content.IContentProvider;

                    public abstract class MockContentProvider extends ContentProvider {
                        /**
                         * Returns IContentProvider which calls back same methods in this class.
                         * By overriding this class, we avoid the mechanism hidden behind ContentProvider
                         * (IPC, etc.)
                         *
                         * @hide
                         */
                        @Override
                        public final IContentProvider getIContentProvider() {
                            return mIContentProvider;
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.content;

                    /** @hide */
                    public abstract class ContentProvider {
                        protected boolean isTemporary() {
                            return false;
                        }

                        // This is supposed to be @hide, but in turbine-combined/framework.jar included
                        // by java_sdk_library like test-mock, it's not; this is what the special
                        // flag is used to test
                        public IContentProvider getIContentProvider() {
                            return null;
                        }
                    }
                    """
                ),
                java(
                    """
                    package android.content;
                    import android.os.IInterface;

                    /**
                     * The ipc interface to talk to a content provider.
                     * @hide
                     */
                    public interface IContentProvider extends IInterface {
                    }
                    """
                ),
                java(
                    """
                    package android.content;

                    // Not hidden. Here to make sure that we respect stub-packages
                    // and exclude it from everything, including signatures.
                    public class ClipData {
                    }
                    """
                )
            ),
            api = """
                package android.test.mock {
                  public abstract class MockContentProvider {
                    ctor public MockContentProvider();
                  }
                }
                """
        )
    }

    @Test
    fun `Test Visible For Testing`() {
        // Use the otherwise= visibility in signatures
        // Regression test for issue 118763806
        check(
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.VisibleForTesting;

                    @SuppressWarnings({"ClassNameDiffersFromFileName", "WeakerAccess"})
                    public class ProductionCodeJava {
                        private ProductionCodeJava() { }

                        @VisibleForTesting(otherwise = VisibleForTesting.PROTECTED)
                        public void shouldBeProtected() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
                        protected void shouldBePrivate1() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
                        public void shouldBePrivate2() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PACKAGE_PRIVATE)
                        public void shouldBePackagePrivate() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.NONE)
                        public void shouldBeHidden() {
                        }
                    }
                    """
                ).indented(),
                kotlin(
                    """
                    package test.pkg
                    import androidx.annotation.VisibleForTesting

                    open class ProductionCodeKotlin private constructor() {

                        @VisibleForTesting(otherwise = VisibleForTesting.PROTECTED)
                        fun shouldBeProtected() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
                        protected fun shouldBePrivate1() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
                        fun shouldBePrivate2() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.PACKAGE_PRIVATE)
                        fun shouldBePackagePrivate() {
                        }

                        @VisibleForTesting(otherwise = VisibleForTesting.NONE)
                        fun shouldBeHidden() {
                        }
                    }
                    """
                ).indented(),
                visibleForTestingSource
            ),
            api = """
                package test.pkg {
                  public class ProductionCodeJava {
                    method protected void shouldBeProtected();
                  }
                  public class ProductionCodeKotlin {
                    method protected final void shouldBeProtected();
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `References Deprecated`() {
        check(
            extraArguments = arrayOf(
                ARG_ERROR, "ReferencesDeprecated",
                ARG_ERROR, "ExtendsDeprecated"
            ),
            expectedIssues = """
            src/test/pkg/MyClass.java:3: error: Parameter of deprecated type test.pkg.DeprecatedClass in test.pkg.MyClass.method1(): this method should also be deprecated [ReferencesDeprecated]
            src/test/pkg/MyClass.java:4: error: Return type of deprecated type test.pkg.DeprecatedInterface in test.pkg.MyClass.method2(): this method should also be deprecated [ReferencesDeprecated]
            src/test/pkg/MyClass.java:4: error: Returning deprecated type test.pkg.DeprecatedInterface from test.pkg.MyClass.method2(): this method should also be deprecated [ReferencesDeprecated]
            src/test/pkg/MyClass.java:2: error: Extending deprecated super class class test.pkg.DeprecatedClass from test.pkg.MyClass: this class should also be deprecated [ExtendsDeprecated]
            src/test/pkg/MyClass.java:2: error: Implementing interface of deprecated type test.pkg.DeprecatedInterface in test.pkg.MyClass: this class should also be deprecated [ExtendsDeprecated]
            """,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    /** @deprecated */
                    @Deprecated
                    public class DeprecatedClass {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    /** @deprecated */
                    @Deprecated
                    public interface DeprecatedInterface {
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class MyClass extends DeprecatedClass implements DeprecatedInterface {
                        public void method1(DeprecatedClass p, int i) { }
                        public DeprecatedInterface method2(int i) { return null; }

                        /** @deprecated */
                        @Deprecated
                        public void method3(DeprecatedClass p, int i) { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `v3 format for qualified references in types`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                java(
                    """
                    package androidx.appcompat.app;
                    import android.view.View;
                    import android.view.View.OnClickListener;

                    public class ActionBarDrawerToggle {
                        private ActionBarDrawerToggle() { }
                        public View.OnClickListener getToolbarNavigationClickListener1() {
                            return null;
                        }
                        public OnClickListener getToolbarNavigationClickListener2() {
                            return null;
                        }
                        public android.view.View.OnClickListener getToolbarNavigationClickListener3() {
                            return null;
                        }
                    }
                    """
                )
            ),
            api = """
                // Signature format: 3.0
                package androidx.appcompat.app {
                  public class ActionBarDrawerToggle {
                    method public android.view.View.OnClickListener! getToolbarNavigationClickListener1();
                    method public android.view.View.OnClickListener! getToolbarNavigationClickListener2();
                    method public android.view.View.OnClickListener! getToolbarNavigationClickListener3();
                  }
                }
                """
        )
    }

    @Test
    fun `FooKt class constructors are not public`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin("src/main/java/test/pkg/Foo.kt",
                    """
                    package test.pkg
                    fun myCall() : Boolean = false
                    class Bar
                    """
                )
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class Bar {
                    ctor public Bar();
                  }
                  public final class FooKt {
                    method public static boolean myCall();
                  }
                }
                """
        )
    }

    @Test
    fun `Test inherited hidden methods for descendant classes - Package private`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Class4 extends Class3 {
                        public void method4() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Class3 extends Class2 {
                        public void method3() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    class Class2 extends Class1 {
                        public void method2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Class1 {
                        public void method1() { }
                    }
                    """
                )
            ),
            expectedIssues = "",
            api =
            """
                package test.pkg {
                  public class Class1 {
                    ctor public Class1();
                    method public void method1();
                  }
                  public class Class3 extends test.pkg.Class1 {
                    ctor public Class3();
                    method public void method2();
                    method public void method3();
                  }
                  public class Class4 extends test.pkg.Class3 {
                    ctor public Class4();
                    method public void method4();
                  }
                }
                """
        )
    }

    @Test
    fun `Test inherited hidden methods for descendant classes - Hidden annotation`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Class4 extends Class3 {
                        public void method4() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Class3 extends Class2 {
                        public void method3() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    /** @hide */
                    public class Class2 extends Class1 {
                        public void method2() { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    public class Class1 {
                        public void method1() { }
                    }
                    """
                )
            ),
            expectedIssues = "src/test/pkg/Class3.java:2: warning: Public class test.pkg.Class3 stripped of unavailable superclass test.pkg.Class2 [HiddenSuperclass]",
            api =
            """
                package test.pkg {
                  public class Class1 {
                    ctor public Class1();
                    method public void method1();
                  }
                  public class Class3 extends test.pkg.Class1 {
                    ctor public Class3();
                    method public void method2();
                    method public void method3();
                  }
                  public class Class4 extends test.pkg.Class3 {
                    ctor public Class4();
                    method public void method4();
                  }
                }
                """

        )
    }

    @Test
    fun `Test inherited methods that use generics`() {
        check(
            compatibilityMode = false,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.NonNull;
                    public class Class2 extends Class1<String> {
                        @Override
                        public void method1(String input) { }
                        @Override
                        public void method2(@NonNull String input) { }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.NonNull;
                    class Class1<T> {
                        public void method1(T input) { }
                        public void method2(T input) { }
                        public void method3(T input) { }
                        @NonNull
                        public String method4(T input) { return ""; }
                        public T method5(@NonNull String input) { return null; }
                    }
                    """
                ),
                androidxNonNullSource
            ),
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation"),
            expectedIssues = "",
            api =
                """
                package test.pkg {
                  public class Class2 {
                    ctor public Class2();
                    method public void method1(String);
                    method public void method2(@NonNull String);
                    method public void method3(String);
                    method @NonNull public String method4(String);
                    method public String method5(@NonNull String);
                  }
                }
                """

        )
    }

    @Test
    fun `Test merging API signature files`() {
        val source1 = """
            package Test.pkg {
              public final class Class1 {
                method public void method1();
              }
            }
            package Test.pkg1 {
              public final class Class1 {
                method public void method1();
              }
            }
                    """
        val source2 = """
            package Test.pkg {
              public final class Class2 {
                method public void method1(String);
              }
            }
            package Test.pkg2 {
              public final class Class1 {
                method public void method1(String, String);
              }
            }
                    """
        val expected = """
            package Test.pkg {
              public final class Class1 {
                method public void method1();
              }
              public final class Class2 {
                method public void method1(java.lang.String);
              }
            }
            package Test.pkg1 {
              public final class Class1 {
                method public void method1();
              }
            }
            package Test.pkg2 {
              public final class Class1 {
                method public void method1(java.lang.String, java.lang.String);
              }
            }
                    """
        check(
            signatureSources = arrayOf(source1, source2),
            api = expected
        )
    }

    val MERGE_TEST_SOURCE_1 = """
            package test.pkg {
              public final class BaseClass {
                method public void method1();
              }
            }
                    """
    val MERGE_TEST_SOURCE_2 = """
            package test.pkg {
              public final class SubClass extends test.pkg.BaseClass {
              }
            }
                    """
    val MERGE_TEST_EXPECTED = """
            package test.pkg {
              public final class BaseClass {
                method public void method1();
              }
              public final class SubClass extends test.pkg.BaseClass {
              }
            }
            """

    @Test
    fun `Test merging API signature files, one refer to another`() {
        check(
            signatureSources = arrayOf(MERGE_TEST_SOURCE_1, MERGE_TEST_SOURCE_2),
            api = MERGE_TEST_EXPECTED
        )
    }

    @Test
    fun `Test merging API signature files, one refer to another, in reverse order`() {
        // Exactly the same as the previous test, but read them in the reverse order
        check(
            signatureSources = arrayOf(MERGE_TEST_SOURCE_2, MERGE_TEST_SOURCE_1),
            api = MERGE_TEST_EXPECTED
        )
    }

    @Test
    fun `Test merging API signature files with reverse dependency`() {
        val source1 = """
            package test.pkg {
              public final class Class1 {
                method public void method1(test.pkg.Class2 arg);
              }
            }
                    """
        val source2 = """
            package test.pkg {
              public final class Class2 {
              }
            }
                    """
        val expected = """
            package test.pkg {
              public final class Class1 {
                method public void method1(test.pkg.Class2);
              }
              public final class Class2 {
              }
            }
                    """
        check(
            signatureSources = arrayOf(source1, source2),
            api = expected
        )
    }

    @Test
    fun `Test merging 3 API signature files`() {
        val source1 = """
            package test.pkg1 {
              public final class BaseClass1 {
                method public void method1();
              }

              public final class AnotherSubClass extends test.pkg2.AnotherBase {
                method public void method1();
              }
            }
                    """
        val source2 = """
            package test.pkg2 {
              public final class SubClass1 extends test.pkg1.BaseClass1 {
              }
            }
                    """
        val source3 = """
            package test.pkg2 {
              public final class SubClass2 extends test.pkg2.SubClass1 {
                method public void bar();
              }

              public final class AnotherBase {
                method public void baz();
              }
            }
                    """
        val expected = """
            package test.pkg1 {
              public final class AnotherSubClass extends test.pkg2.AnotherBase {
                method public void method1();
              }
              public final class BaseClass1 {
                method public void method1();
              }
            }
            package test.pkg2 {
              public final class AnotherBase {
                method public void baz();
              }
              public final class SubClass1 extends test.pkg1.BaseClass1 {
              }
              public final class SubClass2 extends test.pkg2.SubClass1 {
                method public void bar();
              }
            }
                    """
        check(
            signatureSources = arrayOf(source1, source2, source3),
            api = expected
        )
    }

    @Test
    fun `Test cannot merging API signature files with duplicate class`() {
        val source1 = """
            package Test.pkg {
              public final class Class1 {
                method public void method1();
              }
            }
                    """
        val source2 = """
            package Test.pkg {
              public final class Class1 {
                method public void method1();
              }
            }
                    """
        check(
            signatureSources = arrayOf(source1, source2),
            expectedFail = "Aborting: Unable to parse signature file: TESTROOT/project/load-api2.txt:2: Duplicate class found: Test.pkg.Class1"
        )
    }

    @Test
    fun `Test cannot merging API signature files with different file formats`() {
        val source1 = """
            // Signature format: 2.0
            package Test.pkg {
            }
                    """
        val source2 = """
            // Signature format: 3.0
            package Test.pkg {
            }
                    """
        check(
            signatureSources = arrayOf(source1, source2),
            expectedFail = "Aborting: Unable to parse signature file: Cannot merge different formats of signature files. " +
                "First file format=V2, current file format=V3: file=TESTROOT/project/load-api2.txt"
        )
    }

    @Test
    fun `Test tracking of @Composable annotation from classpath`() {
        check(
            format = FileFormat.V3,
            classpath = arrayOf(
                /* The following source file, compiled, and root folder jar'ed and stored as base64 gzip:
                    package test.pkg
                    @MustBeDocumented
                    @Retention(AnnotationRetention.BINARY)
                    @Target(
                        AnnotationTarget.CLASS,
                        AnnotationTarget.FUNCTION,
                        AnnotationTarget.TYPE,
                        AnnotationTarget.TYPE_PARAMETER,
                        AnnotationTarget.PROPERTY
                    )
                    annotation class Composable
                 */
                base64gzip(
                    "test.jar", "" +
                        "UEsDBAoAAAgIAKx6s1AAAAAAAgAAAAAAAAAJAAAATUVUQS1JTkYvAwBQSwMECgAACAgAZ3qzULJ/" +
                        "Au4bAAAAGQAAABQAAABNRVRBLUlORi9NQU5JRkVTVC5NRvNNzMtMSy0u0Q1LLSrOzM+zUjDUM+Dl" +
                        "4uUCAFBLAwQKAAAICABnerNQDArdZgwAAAAQAAAAGwAAAE1FVEEtSU5GL3RlbXAua290bGluX21v" +
                        "ZHVsZWNgYGBmYGBghGIBAFBLAwQKAAAICABnerNQAAAAAAIAAAAAAAAABQAAAHRlc3QvAwBQSwME" +
                        "CgAACAgAZ3qzUAAAAAACAAAAAAAAAAkAAAB0ZXN0L3BrZy8DAFBLAwQKAAAICABnerNQbrgjGPQB" +
                        "AACVAwAAGQAAAHRlc3QvcGtnL0NvbXBvc2FibGUuY2xhc3OFUk1v2kAQfWtioG6TkKRpSdI0H01I" +
                        "P6S65doTEEdF4kvGrRRxqBZYIQdjo+xClRu3Xvsz+ht6qFCO/VFVZ4kCVLJU2Xo7O/PGM/M8v//8" +
                        "/AUgjzcMW0pIZQ/7PbsUDYaR5O1ApMAYMld8zO2Ahz273r4SHZVCguFg4eVhGCmu/Ci0C3MzBZPh" +
                        "pNKPVOCHy5TqSKqiOI86o4EIleh+YNiPoblCUZgsiptjHowEw1kMb1FxOSNZLNcK7iXDbkyKx697" +
                        "QhFrjQdB9FV07xwyvt9FgXmeWaoUmk2G9MWnWskr12sMK95lw6Ev6uNLo+AWqo7nuERpuPWG43rU" +
                        "ylElVrJ/lDiM5yyPlvsPpREFfudmpmoscT7FcXzcCYRux7sZCi0kzfGxfs6wcS9NVSje5YpT0BiM" +
                        "E7Q+TEOGru3ZFRpoQ1ifXN33NNR0YllG1rCMzJ41naRvvxnZ6SRvvGPF6eT2R9LQvDzDdiVmBakM" +
                        "SF4lBkOG1YX/bV8xWM1odN0RF35A27HjjkiAgfjsS58Ii/8mc1QAK/SZpG6P7FczfInXdH5Hih4g" +
                        "TfEHAhYe4hGZqy2YAmtY15DRsKFhU8MWHlPC9l3CE6zjqTZbMASympbFDnZhYq+FRBnPZu8+nt/f" +
                        "Dso4xBGZOG6BSbzACYUkTiVyEmd/AVBLAQIUAwoAAAgIAKx6s1AAAAAAAgAAAAAAAAAJAAAAAAAA" +
                        "AAAAEADtQQAAAABNRVRBLUlORi9QSwECFAMKAAAICABnerNQsn8C7hsAAAAZAAAAFAAAAAAAAAAA" +
                        "AAAApIEpAAAATUVUQS1JTkYvTUFOSUZFU1QuTUZQSwECFAMKAAAICABnerNQDArdZgwAAAAQAAAA" +
                        "GwAAAAAAAAAAAAAAoIF2AAAATUVUQS1JTkYvdGVtcC5rb3RsaW5fbW9kdWxlUEsBAhQDCgAACAgA" +
                        "Z3qzUAAAAAACAAAAAAAAAAUAAAAAAAAAAAAQAOhBuwAAAHRlc3QvUEsBAhQDCgAACAgAZ3qzUAAA" +
                        "AAACAAAAAAAAAAkAAAAAAAAAAAAQAOhB4AAAAHRlc3QvcGtnL1BLAQIUAwoAAAgIAGd6s1BuuCMY" +
                        "9AEAAJUDAAAZAAAAAAAAAAAAAACggQkBAAB0ZXN0L3BrZy9Db21wb3NhYmxlLmNsYXNzUEsFBgAA" +
                        "AAAGAAYAcwEAADQDAAAAAA=="
                )
            ),
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    class RadioGroupScope() {
                        @Composable
                        fun RadioGroupItem(
                            selected: Boolean,
                            onSelect: () -> Unit,
                            content: @Composable () -> Unit
                        ) { }
                    }
                """
                )
            ),
            expectedIssues = "",
            api =
            """
                // Signature format: 3.0
                package test.pkg {
                  public final class RadioGroupScope {
                    ctor public RadioGroupScope();
                    method @test.pkg.Composable public void RadioGroupItem(boolean selected, kotlin.jvm.functions.Function0<kotlin.Unit> onSelect, kotlin.jvm.functions.Function0<kotlin.Unit> content);
                  }
                }
            """
        )
    }

    @Test
    fun `Test for experimental annotations from classpath`() {
        check(
            format = FileFormat.V3,
            classpath = arrayOf(
                /* The following source file, compiled, and root folder jar'ed and stored as base64 gzip
                   Encoded using openssl base64 < test.jar | tr -d '\n'

                    package test.pkg
                    @RequiresOptIn
                    annotation class ExternalExperimentalAnnotation
                 */
                base64gzip(
                    "test.jar", "" +
                        "UEsDBAoAAAgIADt2U1IAAAAAAgAAAAAAAAAJAAAATUVUQS1JTkYvAwBQSwMECgAACAgAFXZ" +
                        "TUrJ/Au4bAAAAGQAAABQAAABNRVRBLUlORi9NQU5JRkVTVC5NRvNNzMtMSy0u0Q1LLSrOzM" +
                        "+zUjDUM+Dl4uUCAFBLAwQKAAAICAA7dlNSDWpm1BUAAAAYAAAAGwAAAE1FVEEtSU5GL3Rlc" +
                        "3Qua290bGluX21vZHVsZWNgYGBmYGBgBGIWIGYCYgYlBi0GAFBLAwQKAAAICAA7dlNSAAAA" +
                        "AAIAAAAAAAAABQAAAHRlc3QvAwBQSwMECgAACAgAO3ZTUgAAAAACAAAAAAAAAAkAAAB0ZXN" +
                        "0L3BrZy8DAFBLAwQKAAAICAA7dlNSPYCyXGwBAABkAgAALQAAAHRlc3QvcGtnL0V4dGVybm" +
                        "FsRXhwZXJpbWVudGFsQW5ub3RhdGlvbi5jbGFzc41Qy04CQRCsWZ6uL/CBICp6wXhxlasnT" +
                        "TBuAmLwceE0wIQsLLPIzhK87c1f8Rs8GMLRjzL2qoiJRr309HRVdXf188vjE4ACcgy7SrjK" +
                        "6HVaRnGoRF9yuzjsib7VFVJx+1hKR3FlOTIGxpBo8wE3bC5bRqXeFg0VQ4ghN63yT77xVRp" +
                        "hSJU6jrItaVTFrWf1hVvpKVMeMWyXfpRXhaINKCNKZMBtTzDk/6BeOLbVuCNBrHp9fmWWiw" +
                        "zJydiyULzJFSdU6w5CZJ8FIRwEjWr1txqCQJZYh0rNQ9pu5Ou6ltZ0LZHVR358fK+lR35BO" +
                        "2AnI3/8EA2kzQLDXumfd6T5YAgHbIad37n7HeLol47Xb4hTy6YLZKoeOe2KG8u16raYUl2G" +
                        "7AdmysE3NE9rIkyDo3h3uRHYRhab9J5RFidsRkDHLOYQwXwNIRMLJhZNJJCc/JZMLGOFUqz" +
                        "WwFyksEaQi7SLjIt1bFG3KHWKAa9QSwECFAMKAAAICAA7dlNSAAAAAAIAAAAAAAAACQAAAA" +
                        "AAAAAAABAA7UEAAAAATUVUQS1JTkYvUEsBAhQDCgAACAgAFXZTUrJ/Au4bAAAAGQAAABQAA" +
                        "AAAAAAAAAAAAKSBKQAAAE1FVEEtSU5GL01BTklGRVNULk1GUEsBAhQDCgAACAgAO3ZTUg1q" +
                        "ZtQVAAAAGAAAABsAAAAAAAAAAAAAAKCBdgAAAE1FVEEtSU5GL3Rlc3Qua290bGluX21vZHV" +
                        "sZVBLAQIUAwoAAAgIADt2U1IAAAAAAgAAAAAAAAAFAAAAAAAAAAAAEADoQcQAAAB0ZXN0L1" +
                        "BLAQIUAwoAAAgIADt2U1IAAAAAAgAAAAAAAAAJAAAAAAAAAAAAEADoQekAAAB0ZXN0L3BrZ" +
                        "y9QSwECFAMKAAAICAA7dlNSPYCyXGwBAABkAgAALQAAAAAAAAAAAAAAoIESAQAAdGVzdC9w" +
                        "a2cvRXh0ZXJuYWxFeHBlcmltZW50YWxBbm5vdGF0aW9uLmNsYXNzUEsFBgAAAAAGAAYAhwE" +
                        "AAMkCAAAAAA=="
                )
            ),
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg

                    @ExternalExperimentalAnnotation
                    class ClassUsingExternalExperimentalApi

                    @InLibraryExperimentalAnnotation
                    class ClassUsingInLibraryExperimentalApi
                """
                ),
                kotlin(
                    """
                        package test.pkg
                        @RequiresOptIn
                        annotation class InLibraryExperimentalAnnotation
                    """
                )
            ),
            expectedIssues = "",
            api =
            """
                // Signature format: 3.0
                package test.pkg {
                  @kotlin.RequiresOptIn public @interface InLibraryExperimentalAnnotation {
                  }
                }
            """,
            extraArguments = arrayOf(
                ARG_HIDE_META_ANNOTATION, "kotlin.RequiresOptIn"
            )
        )
    }

    @Test
    fun `@IntRange value in kotlin`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin("""
                    package test.pkg

                    import androidx.annotation.IntRange

                    class KotlinClass(@IntRange(from = 1) val param: Int) {
                        constructor(@IntRange(from = 2) val differentParam: Int)
                        fun myMethod(@IntRange(from = 3) val methodParam: Int) {}
                    }
                """
                ),
                androidxIntRangeSource
            ),
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation"),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class KotlinClass {
                    ctor public KotlinClass(@IntRange(from=1) int param);
                    ctor public KotlinClass(@IntRange(from=2) int differentParam);
                    method public int getParam();
                    method public void myMethod(@IntRange(from=3) int methodParam);
                    property public final int param;
                  }
                }
            """
        )
    }

    @Test
    fun `Annotation value visibility`() {
        check(
            format = FileFormat.V2,
            sourceFiles = arrayOf(
                java("""
                    package test.pkg

                    import androidx.annotation.IntRange

                    public final class ApiClass {
                        private int hiddenConstant = 1;
                        public ApiClass(@IntRange(from=1) int x) {}
                        public void method(@IntRange(from = hiddenConstant) int x) {}
                    }
                """
                ),
                androidxIntRangeSource
            ),
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation"),
            api = """
                // Signature format: 2.0
                package test.pkg {
                  public final class ApiClass {
                    ctor public ApiClass(@IntRange(from=1) int);
                    method public void method(@IntRange(from=0x1) int);
                  }
                }
            """
        )
    }

    @Test
    fun `Kotlin properties with overriding get`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin("""
                    package test.pkg

                    import androidx.annotation.IntRange

                    class KotlinClass() {
                        val propertyWithGetter: Boolean get() = true
                        val propertyWithNoGetter: Boolean = true
                    }
                """
                ),
                androidxIntRangeSource
            ),
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation"),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public final class KotlinClass {
                    ctor public KotlinClass();
                    method public boolean getPropertyWithGetter();
                    method public boolean getPropertyWithNoGetter();
                    property public final boolean propertyWithGetter;
                    property public final boolean propertyWithNoGetter;
                  }
                }
            """
        )
    }

    @Test
    fun `Constructor property tracking`() {
        check(
            format = FileFormat.V3,
            sourceFiles = arrayOf(
                kotlin("""
                    package test.pkg
                    sealed class MyClass(
                        val firstConstructorProperty: Int,
                        val secondConstructorProperty: Boolean
                    ) {
                        val nonConstructorProperty: String = "PROP"
                    }
                    """
                ),
                kotlin("""
                    package test.pkg
                    data class MyDataClass(
                        val constructorProperty: String,
                        internal val internalConstructorProperty: String
                    )
                """)
            ),
            api = """
                // Signature format: 3.0
                package test.pkg {
                  public abstract sealed class MyClass {
                    method public final int getFirstConstructorProperty();
                    method public final String getNonConstructorProperty();
                    method public final boolean getSecondConstructorProperty();
                    property public final int firstConstructorProperty;
                    property public final String nonConstructorProperty;
                    property public final boolean secondConstructorProperty;
                  }
                  public final class MyDataClass {
                    ctor public MyDataClass(String constructorProperty, String internalConstructorProperty);
                    method public String component1();
                    method public test.pkg.MyDataClass copy(String constructorProperty, String internalConstructorProperty);
                    method public String getConstructorProperty();
                    property public final String constructorProperty;
                  }
                }
            """
        )
    }

    @Test
    fun `Concise default Values Names in Java`() {
        // Java code which explicitly specifies parameter names
        check(
            format = FileFormat.V4,
            sourceFiles = arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.DefaultValue;

                    public class Foo {
                        public void foo(
                            @DefaultValue("null") String prefix,
                            @DefaultValue("\"Hello World\"") String greeting,
                            @DefaultValue("42") int meaning) {
                        }
                    }
                    """
                ),
                supportDefaultValue
            ),
            api = """
                // Signature format: 4.0
                package test.pkg {
                  public class Foo {
                    ctor public Foo();
                    method public void foo(optional String!, optional String!, optional int);
                  }
                }
                 """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation")
        )
    }

    @Test
    fun `Concise default Values and Names in Kotlin`() {
        // Kotlin code which explicitly specifies parameter names
        check(
            format = FileFormat.V4,
            compatibilityMode = false,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package test.pkg
                    import some.other.pkg.Constants.Misc.SIZE
                    import android.graphics.Bitmap
                    import android.view.View

                    class Foo(a: String = "1", b: String = "2") {
                        fun method1(myInt: Int = 42,
                            myInt2: Int? = null,
                            myByte: Int = 2 * 21,
                            str: String = "hello " + "world",
                            vararg args: String) { }

                        fun method2(myInt: Int, myInt2: Int = (2*int) * SIZE) { }

                        fun method3(str: String, myInt: Int, myInt2: Int = double(int) + str.length) { }

                        fun emptyLambda(sizeOf: () -> Unit = {  }) {}

                        fun View.drawToBitmap(config: Bitmap.Config = Bitmap.Config.ARGB_8888): Bitmap? = null

                        companion object {
                            fun double(myInt: Int) = 2 * myInt
                            fun print(foo: Foo = Foo()) { println(foo) }
                        }
                    }
                    """
                ),
                java(
                    """
                    package some.other.pkg;
                    public class Constants {
                        public static class Misc {
                            public static final int SIZE = 5;
                        }
                    }
                    """
                )
            ),
            api = """
                // Signature format: 4.0
                package test.pkg {
                  public final class Foo {
                    ctor public Foo(optional String a, optional String b);
                    method public android.graphics.Bitmap? drawToBitmap(android.view.View, optional android.graphics.Bitmap.Config config);
                    method public void emptyLambda(optional kotlin.jvm.functions.Function0<kotlin.Unit> sizeOf);
                    method public void method1(optional int myInt, optional Integer? myInt2, optional int myByte, optional String str, java.lang.String... args);
                    method public void method2(int myInt, optional int myInt2);
                    method public void method3(String str, int myInt, optional int myInt2);
                    field public static final test.pkg.Foo.Companion Companion;
                  }
                  public static final class Foo.Companion {
                    method public int double(int myInt);
                    method public void print(optional test.pkg.Foo foo);
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation", ARG_HIDE_PACKAGE, "some.other.pkg"),
            includeSignatureVersion = true
        )
    }

    @Test
    fun `Concise default Values in Kotlin for expressions`() {
        // Testing trickier default values; regression test for problem
        // observed in androidx.core.util with LruCache
        check(
            format = FileFormat.V4,
            sourceFiles = arrayOf(
                kotlin(
                    """
                    package androidx.core.util

                    import android.util.LruCache

                    inline fun <K : Any, V : Any> lruCache(
                        maxSize: Int,
                        crossinline sizeOf: (key: K, value: V) -> Int = { _, _ -> 1 },
                        @Suppress("USELESS_CAST") // https://youtrack.jetbrains.com/issue/KT-21946
                        crossinline create: (key: K) -> V? = { null as V? },
                        crossinline onEntryRemoved: (evicted: Boolean, key: K, oldValue: V, newValue: V?) -> Unit =
                            { _, _, _, _ -> }
                    ): LruCache<K, V> {
                        return object : LruCache<K, V>(maxSize) {
                            override fun sizeOf(key: K, value: V) = sizeOf(key, value)
                            override fun create(key: K) = create(key)
                            override fun entryRemoved(evicted: Boolean, key: K, oldValue: V, newValue: V?) {
                                onEntryRemoved(evicted, key, oldValue, newValue)
                            }
                        }
                    }
                    """
                ),
                java(
                    """
                    package androidx.collection;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;

                    import java.util.LinkedHashMap;
                    import java.util.Locale;
                    import java.util.Map;

                    public class LruCache<K, V> {
                        @Nullable
                        protected V create(@NonNull K key) {
                            return null;
                        }

                        protected int sizeOf(@NonNull K key, @NonNull V value) {
                            return 1;
                        }

                        protected void entryRemoved(boolean evicted, @NonNull K key, @NonNull V oldValue,
                                @Nullable V newValue) {
                        }
                    }
                    """
                ),
                androidxNullableSource,
                androidxNonNullSource
            ),
            api = """
                // Signature format: 4.0
                package androidx.core.util {
                  public final class TestKt {
                    method public static inline <K, V> android.util.LruCache<K,V> lruCache(int maxSize, optional kotlin.jvm.functions.Function2<? super K,? super V,java.lang.Integer> sizeOf, optional kotlin.jvm.functions.Function1<? super K,? extends V> create, optional kotlin.jvm.functions.Function4<? super java.lang.Boolean,? super K,? super V,? super V,kotlin.Unit> onEntryRemoved);
                  }
                }
                """,
            extraArguments = arrayOf(ARG_HIDE_PACKAGE, "androidx.annotation", ARG_HIDE_PACKAGE, "androidx.collection"),
            includeSignatureVersion = true
        )
    }

    @Test
    fun `Test type erasure and dexApi from signature`() {
        check(
            signatureSources = arrayOf("""
                package android.widget {

                  @android.widget.RemoteViews.RemoteView public class ListView extends android.widget.AbsListView {
                    method protected <T extends android.view.View> T findViewTraversal(@IdRes int);
                    method protected long tryAcquireShared(long);
                  }

                }
"""),
            dexApi = """
            Landroid/widget/ListView;
            Landroid/widget/ListView;->findViewTraversal(I)Landroid/view/View;
            Landroid/widget/ListView;->tryAcquireShared(J)J
            """
        )
    }

    @Test
    fun `Functional interface in signature`() {
        check(
            format = FileFormat.V4,
            sourceFiles = arrayOf(
                kotlin("""
                    package test.pkg

                    fun interface FunctionalInterface {
                        fun methodOne(number: Int): Boolean
                    }

                    fun userOfFunctionalInterface(parameter: FunctionalInterface) { }
                """
                )
            ),
            api = """
                // Signature format: 4.0
                package test.pkg {
                  public fun interface FunctionalInterface {
                    method public boolean methodOne(int number);
                  }
                  public final class FunctionalInterfaceKt {
                    method public static void userOfFunctionalInterface(test.pkg.FunctionalInterface parameter);
                  }
                }
            """
        )
    }
}
