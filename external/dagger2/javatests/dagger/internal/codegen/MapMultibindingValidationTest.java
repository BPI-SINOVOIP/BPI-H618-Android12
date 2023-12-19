/*
 * Copyright (C) 2014 The Dagger Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package dagger.internal.codegen;

import static com.google.testing.compile.CompilationSubject.assertThat;
import static dagger.internal.codegen.Compilers.compilerWithOptions;
import static dagger.internal.codegen.Compilers.daggerCompiler;
import static dagger.internal.codegen.TestUtils.message;

import com.google.common.collect.ImmutableList;
import com.google.testing.compile.Compilation;
import com.google.testing.compile.JavaFileObjects;
import javax.tools.JavaFileObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class MapMultibindingValidationTest {
  @Test
  public void duplicateMapKeys_UnwrappedMapKey() {
    JavaFileObject module =
        JavaFileObjects.forSourceLines(
            "test.MapModule",
            "package test;",
            "",
            "import dagger.Module;",
            "import dagger.Provides;",
            "import dagger.multibindings.StringKey;",
            "import dagger.multibindings.IntoMap;",
            "",
            "@Module",
            "final class MapModule {",
            "  @Provides @IntoMap @StringKey(\"AKey\") Object provideObjectForAKey() {",
            "    return \"one\";",
            "  }",
            "",
            "  @Provides @IntoMap @StringKey(\"AKey\") Object provideObjectForAKeyAgain() {",
            "    return \"one again\";",
            "  }",
            "}");

    // If they're all there, report only Map<K, V>.
    Compilation compilation =
        daggerCompiler()
            .compile(
                module,
                component(
                    "Map<String, Object> objects();",
                    "Map<String, Provider<Object>> objectProviders();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Object>");
    assertThat(compilation).hadErrorContaining("provideObjectForAKey()");
    assertThat(compilation).hadErrorContaining("provideObjectForAKeyAgain()");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        compilerWithOptions("-Adagger.fullBindingGraphValidation=ERROR")
            .compile(module);
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Provider<Object>>")
        .inFile(module)
        .onLineContaining("class MapModule");
    assertThat(compilation).hadErrorContaining("provideObjectForAKey()");
    assertThat(compilation).hadErrorContaining("provideObjectForAKeyAgain()");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, V> and Map<K, Provider<V>>, report only Map<K, V>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                component(
                    "Map<String, Object> objects();",
                    "Map<String, Provider<Object>> objectProviders();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Object>");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, V> and Map<K, Producer<V>>, report only Map<K, V>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                component(
                    "Map<String, Object> objects();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Object>");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, Provider<V>> and Map<K, Producer<V>>, report only Map<K, Provider<V>>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                component(
                    "Map<String, Provider<Object>> objectProviders();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Provider<Object>>");
    assertThat(compilation).hadErrorCount(1);

    compilation = daggerCompiler().compile(module, component("Map<String, Object> objects();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Object>");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        daggerCompiler()
            .compile(module, component("Map<String, Provider<Object>> objectProviders();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Provider<Object>>");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        daggerCompiler()
            .compile(
                module, component("Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "The same map key is bound more than once for "
                + "Map<String,Producer<Object>>");
    assertThat(compilation).hadErrorCount(1);
  }

  @Test
  public void duplicateMapKeys_WrappedMapKey() {
    JavaFileObject module =
        JavaFileObjects.forSourceLines(
            "test.MapModule",
            "package test;",
            "",
            "import dagger.Module;",
            "import dagger.Provides;",
            "import dagger.multibindings.IntoMap;",
            "import dagger.MapKey;",
            "",
            "@Module",
            "abstract class MapModule {",
            "",
            "  @MapKey(unwrapValue = false)",
            "  @interface WrappedMapKey {",
            "    String value();",
            "  }",
            "",
            "  @Provides",
            "  @IntoMap",
            "  @WrappedMapKey(\"foo\")",
            "  static String stringMapEntry1() { return \"\"; }",
            "",
            "  @Provides",
            "  @IntoMap",
            "  @WrappedMapKey(\"foo\")",
            "  static String stringMapEntry2() { return \"\"; }",
            "}");

    JavaFileObject component = component("Map<test.MapModule.WrappedMapKey, String> objects();");

    Compilation compilation = daggerCompiler().compile(component, module);
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            message(
                "\033[1;31m[Dagger/MapKeys]\033[0m The same map key is bound more than once for "
                    + "Map<MapModule.WrappedMapKey,String>",
                "    @Provides @IntoMap @MapModule.WrappedMapKey(\"foo\") String "
                    + "MapModule.stringMapEntry1()",
                "    @Provides @IntoMap @MapModule.WrappedMapKey(\"foo\") String "
                    + "MapModule.stringMapEntry2()"))
        .inFile(component)
        .onLineContaining("interface TestComponent");
  }

  @Test
  public void inconsistentMapKeyAnnotations() {
    JavaFileObject module =
        JavaFileObjects.forSourceLines(
            "test.MapModule",
            "package test;",
            "",
            "import dagger.Module;",
            "import dagger.Provides;",
            "import dagger.multibindings.StringKey;",
            "import dagger.multibindings.IntoMap;",
            "",
            "@Module",
            "final class MapModule {",
            "  @Provides @IntoMap @StringKey(\"AKey\") Object provideObjectForAKey() {",
            "    return \"one\";",
            "  }",
            "",
            "  @Provides @IntoMap @StringKeyTwo(\"BKey\") Object provideObjectForBKey() {",
            "    return \"two\";",
            "  }",
            "}");
    JavaFileObject stringKeyTwoFile =
        JavaFileObjects.forSourceLines(
            "test.StringKeyTwo",
            "package test;",
            "",
            "import dagger.MapKey;",
            "",
            "@MapKey(unwrapValue = true)",
            "public @interface StringKeyTwo {",
            "  String value();",
            "}");

    // If they're all there, report only Map<K, V>.
    Compilation compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component(
                    "Map<String, Object> objects();",
                    "Map<String, Provider<Object>> objectProviders();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Object>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorContaining("provideObjectForAKey()");
    assertThat(compilation).hadErrorContaining("provideObjectForBKey()");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        compilerWithOptions("-Adagger.fullBindingGraphValidation=ERROR")
            .compile(module, stringKeyTwoFile);
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Provider<Object>>"
                + " uses more than one @MapKey annotation type")
        .inFile(module)
        .onLineContaining("class MapModule");
    assertThat(compilation).hadErrorContaining("provideObjectForAKey()");
    assertThat(compilation).hadErrorContaining("provideObjectForBKey()");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, V> and Map<K, Provider<V>>, report only Map<K, V>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component(
                    "Map<String, Object> objects();",
                    "Map<String, Provider<Object>> objectProviders();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Object>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, V> and Map<K, Producer<V>>, report only Map<K, V>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component(
                    "Map<String, Object> objects();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Object>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);

    // If there's Map<K, Provider<V>> and Map<K, Producer<V>>, report only Map<K, Provider<V>>.
    compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component(
                    "Map<String, Provider<Object>> objectProviders();",
                    "Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Provider<Object>>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        daggerCompiler()
            .compile(module, stringKeyTwoFile, component("Map<String, Object> objects();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Object>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component("Map<String, Provider<Object>> objectProviders();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Provider<Object>>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);

    compilation =
        daggerCompiler()
            .compile(
                module,
                stringKeyTwoFile,
                component("Producer<Map<String, Producer<Object>>> objectProducers();"));
    assertThat(compilation).failed();
    assertThat(compilation)
        .hadErrorContaining(
            "Map<String,Producer<Object>>"
                + " uses more than one @MapKey annotation type");
    assertThat(compilation).hadErrorCount(1);
  }

  private static JavaFileObject component(String... entryPoints) {
    return JavaFileObjects.forSourceLines(
        "test.TestComponent",
        ImmutableList.<String>builder()
            .add(
                "package test;",
                "",
                "import dagger.Component;",
                "import dagger.producers.Producer;",
                "import java.util.Map;",
                "import javax.inject.Provider;",
                "",
                "@Component(modules = {MapModule.class})",
                "interface TestComponent {")
            .add(entryPoints)
            .add("}")
            .build());
  }
}
