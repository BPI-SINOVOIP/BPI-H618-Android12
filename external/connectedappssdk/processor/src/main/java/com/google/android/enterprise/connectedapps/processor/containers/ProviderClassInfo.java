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

import static com.google.android.enterprise.connectedapps.processor.GeneratorUtilities.findCrossProfileProviderMethodsInClass;
import static java.util.stream.Collectors.toSet;

import com.google.auto.value.AutoValue;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.squareup.javapoet.ClassName;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.util.Elements;

/** Wrapper of a cross-profile provider class. */
@AutoValue
public abstract class ProviderClassInfo {

  public abstract TypeElement providerClassElement();

  public ImmutableCollection<CrossProfileTypeInfo> allCrossProfileTypes() {
    Set<CrossProfileTypeInfo> types = new HashSet<>();
    types.addAll(nonStaticTypes());
    types.addAll(staticTypes());
    return ImmutableSet.copyOf(types);
  }

  public abstract ImmutableCollection<CrossProfileTypeInfo> nonStaticTypes();

  public abstract ImmutableCollection<CrossProfileTypeInfo> staticTypes();

  public String simpleName() {
    return providerClassElement().getSimpleName().toString();
  }

  public ClassName className() {
    return ClassName.get(providerClassElement());
  }

  public ImmutableCollection<VariableElement> publicConstructorArgumentTypes() {
    return ImmutableList.copyOf(
        providerClassElement().getEnclosedElements().stream()
            .filter(e -> e instanceof ExecutableElement)
            .map(e -> (ExecutableElement) e)
            .filter(e -> e.getKind().equals(ElementKind.CONSTRUCTOR))
            .filter(e -> e.getModifiers().contains(Modifier.PUBLIC))
            .findFirst()
            .get()
            .getParameters());
  }

  public ExecutableElement findProviderMethodFor(
      GeneratorContext generatorContext, CrossProfileTypeInfo crossProfileType) {
    if (!nonStaticTypes().contains(crossProfileType)) {
      throw new IllegalArgumentException("This provider class does not provide this type");
    }

    return providerClassElement().getEnclosedElements().stream()
        .filter(e -> e instanceof ExecutableElement)
        .map(e -> (ExecutableElement) e)
        .filter(
            e ->
                generatorContext
                    .types()
                    .isSameType(
                        e.getReturnType(), crossProfileType.crossProfileTypeElement().asType()))
        .findFirst()
        .get();
  }

  public static ProviderClassInfo create(
      ValidatorContext context, ValidatorProviderClassInfo provider) {
    Set<CrossProfileTypeInfo> nonStaticTypes =
        extractCrossProfileTypeElementsFromReturnValues(
                context.elements(), provider.providerClassElement())
            .stream()
            .map(
                crossProfileTypeElement ->
                    ValidatorCrossProfileTypeInfo.create(
                        context.processingEnv(),
                        crossProfileTypeElement,
                        context.globalSupportedTypes()))
            .map(crossProfileType -> CrossProfileTypeInfo.create(context, crossProfileType))
            .collect(toSet());

    Set<CrossProfileTypeInfo> staticTypes =
        provider.staticTypes().stream()
            .map(
                crossProfileTypeElement ->
                    ValidatorCrossProfileTypeInfo.create(
                        context.processingEnv(),
                        crossProfileTypeElement,
                        context.globalSupportedTypes()))
            .map(crossProfileType -> CrossProfileTypeInfo.create(context, crossProfileType))
            .collect(toSet());

    return new AutoValue_ProviderClassInfo(
        provider.providerClassElement(),
        ImmutableSet.copyOf(nonStaticTypes),
        ImmutableSet.copyOf(staticTypes));
  }

  public static Collection<TypeElement> extractCrossProfileTypeElementsFromReturnValues(
      Elements elements, TypeElement providerClassElement) {
    return findCrossProfileProviderMethodsInClass(providerClassElement).stream()
        .map(e -> elements.getTypeElement(e.getReturnType().toString()))
        .collect(toSet());
  }
}
