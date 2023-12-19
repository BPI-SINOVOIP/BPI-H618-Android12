/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.enterprise.connectedapps.processor.containers;

import com.google.android.enterprise.connectedapps.annotations.CustomFutureWrapper;
import com.google.auto.value.AutoValue;
import com.squareup.javapoet.ClassName;
import java.util.ArrayList;
import java.util.Collection;
import javax.lang.model.element.TypeElement;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

/** Information about future wrapper. */
@AutoValue
public abstract class FutureWrapper {

  /** The type of the Wrapper. This controls how supporting code is generated. */
  public enum WrapperType {
    DEFAULT, // Copied from a resource
    CUSTOM // Included in classpath
  }

  public static final String FUTURE_WRAPPER_PACKAGE =
      "com.google.android.enterprise.connectedapps.futurewrappers";

  public abstract TypeMirror wrappedType();

  public abstract ClassName defaultWrapperClassName();

  public abstract ClassName wrapperClassName();

  public abstract WrapperType wrapperType();

  private static FutureWrapper create(
      TypeMirror wrappedType, ClassName defaultWrapperClassName, WrapperType wrapperType) {
    return create(wrappedType, defaultWrapperClassName, defaultWrapperClassName, wrapperType);
  }

  public static FutureWrapper create(
      TypeMirror wrappedType,
      ClassName defaultWrapperClassName,
      ClassName wrapperClassName,
      WrapperType wrapperType) {
    return new AutoValue_FutureWrapper(
        wrappedType, defaultWrapperClassName, wrapperClassName, wrapperType);
  }

  public static Collection<FutureWrapper> createGlobalFutureWrappers(Elements elements) {
    Collection<FutureWrapper> wrappers = new ArrayList<>();

    addDefaultFutureWrappers(elements, wrappers);

    return wrappers;
  }

  private static void addDefaultFutureWrappers(
      Elements elements, Collection<FutureWrapper> wrappers) {
    tryAddWrapper(
        elements,
        wrappers,
        "com.google.common.util.concurrent.ListenableFuture",
        ClassName.get(FUTURE_WRAPPER_PACKAGE, "ListenableFutureWrapper"),
        WrapperType.DEFAULT);
  }

  public static Collection<FutureWrapper> createCustomFutureWrappers(
      Types types, Elements elements, Collection<TypeElement> customFutureWrappers) {
    Collection<FutureWrapper> wrappers = new ArrayList<>();

    addCustomFutureWrappers(types, elements, wrappers, customFutureWrappers);

    return wrappers;
  }

  private static void addCustomFutureWrappers(
      Types types,
      Elements elements,
      Collection<FutureWrapper> wrappers,
      Collection<TypeElement> customFutureWrappers) {
    for (TypeElement customFutureWrapper : customFutureWrappers) {
      addCustomFutureWrapper(types, elements, wrappers, customFutureWrapper);
    }
  }

  private static void addCustomFutureWrapper(
      Types types,
      Elements elements,
      Collection<FutureWrapper> wrappers,
      TypeElement customFutureWrapper) {
    CustomFutureWrapper customFutureWrapperAnnotation =
        customFutureWrapper.getAnnotation(CustomFutureWrapper.class);

    if (customFutureWrapperAnnotation == null) {
      // This will be dealt with as part of early validation
      return;
    }

    tryAddWrapper(
        elements,
        wrappers,
        FutureWrapperAnnotationInfo.extractFromFutureWrapperAnnotation(
                types, customFutureWrapperAnnotation)
            .originalType()
            .toString(),
        ClassName.get(customFutureWrapper),
        WrapperType.CUSTOM);
  }

  private static void tryAddWrapper(
      Elements elements,
      Collection<FutureWrapper> wrappers,
      String typeQualifiedName,
      ClassName wrapperClassName,
      WrapperType wrapperType) {
    TypeElement typeElement = elements.getTypeElement(typeQualifiedName);

    if (typeElement == null) {
      // The type isn't supported at compile-time - so won't be included in this app
      return;
    }

    wrappers.add(FutureWrapper.create(typeElement.asType(), wrapperClassName, wrapperType));
  }
}
