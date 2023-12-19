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

import static java.util.stream.Collectors.toSet;

import com.google.android.enterprise.connectedapps.annotations.CrossProfileConfiguration;
import com.google.android.enterprise.connectedapps.processor.SupportedTypes;
import com.google.android.enterprise.connectedapps.processor.TypeUtils;
import com.google.auto.value.AutoValue;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Streams;
import com.squareup.javapoet.ClassName;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Optional;
import javax.lang.model.element.TypeElement;
import javax.lang.model.type.TypeMirror;

/** Wrapper of a {@link CrossProfileConfiguration} annotated class. */
@AutoValue
public abstract class CrossProfileConfigurationInfo {

  public static final String CROSS_PROFILE_CONNECTOR_QUALIFIED_NAME =
      "com.google.android.enterprise.connectedapps.CrossProfileConnector";

  public abstract TypeElement configurationElement();

  public abstract ImmutableCollection<ProviderClassInfo> providers();

  public abstract ClassName serviceSuperclass();

  public abstract Optional<TypeElement> serviceClass();

  public String simpleName() {
    return configurationElement().getSimpleName().toString();
  }

  public ClassName className() {
    return ClassName.get(configurationElement());
  }

  public abstract ProfileConnectorInfo profileConnector();

  public static CrossProfileConfigurationInfo create(
      ValidatorContext context, ValidatorCrossProfileConfigurationInfo configuration) {
    Collection<ProviderClassInfo> providerClasses =
        configuration.providerClassElements().stream()
            .map(
                m ->
                    ProviderClassInfo.create(
                        context, ValidatorProviderClassInfo.create(context.processingEnv(), m)))
            .collect(toSet());

    ProfileConnectorInfo profileConnectorInfo =
        providerClasses.stream()
            .flatMap(m -> m.allCrossProfileTypes().stream())
            .map(CrossProfileTypeInfo::profileConnector)
            .flatMap(Streams::stream)
            .findFirst()
            .orElseGet(
                () ->
                    ProfileConnectorInfo.create(
                        context.processingEnv(),
                        getConfiguredConnectorOrDefault(context, configuration),
                        context.globalSupportedTypes()));

    return new AutoValue_CrossProfileConfigurationInfo(
        configuration.configurationElement(),
        ImmutableSet.copyOf(providerClasses),
        configuration.serviceSuperclass(),
        configuration.serviceClass(),
        profileConnectorInfo);
  }

  private static TypeElement getConfiguredConnectorOrDefault(
      ValidatorContext context, ValidatorCrossProfileConfigurationInfo configuration) {
    return configuration
        .connector()
        .orElseGet(() -> context.elements().getTypeElement(CROSS_PROFILE_CONNECTOR_QUALIFIED_NAME));
  }

  private static Collection<Type> convertTypeMirrorToSupportedTypes(
      SupportedTypes supportedTypes, TypeMirror typeMirror) {
    if (TypeUtils.isGeneric(typeMirror)) {
      return convertGenericTypeMirrorToSupportedTypes(supportedTypes, typeMirror);
    }
    return Collections.singleton(supportedTypes.getType(typeMirror));
  }

  private static Collection<Type> convertGenericTypeMirrorToSupportedTypes(
      SupportedTypes supportedTypes, TypeMirror typeMirror) {
    Collection<Type> types = new HashSet<>();
    TypeMirror genericType = TypeUtils.removeTypeArguments(typeMirror);
    Type supportedType = supportedTypes.getType(genericType);
    if (!supportedType.isSupportedWithAnyGenericType()) {
      for (TypeMirror typeArgument : TypeUtils.extractTypeArguments(typeMirror)) {
        types.addAll(convertTypeMirrorToSupportedTypes(supportedTypes, typeArgument));
      }
    }
    types.add(supportedTypes.getType(genericType));
    return types;
  }
}
