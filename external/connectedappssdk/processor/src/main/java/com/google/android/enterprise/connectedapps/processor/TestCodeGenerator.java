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
package com.google.android.enterprise.connectedapps.processor;

import static com.google.common.base.Preconditions.checkNotNull;

import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileConfigurationInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileTestInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileTypeInfo;
import com.google.android.enterprise.connectedapps.processor.containers.GeneratorContext;
import com.google.android.enterprise.connectedapps.processor.containers.ProfileConnectorInfo;
import com.google.android.enterprise.connectedapps.processor.containers.ProviderClassInfo;
import java.util.HashSet;
import java.util.Set;

/**
 * Generator of cross-profile test code.
 *
 * <p>This is intended to be initialised and used once, which will generate all needed code.
 *
 * <p>This must only be used once. It should be used after {@link EarlyValidator} has been used to
 * validate that the annotated code is correct.
 */
final class TestCodeGenerator {
  private boolean generated = false;
  private final GeneratorContext generatorContext;
  private final Set<CrossProfileTypeInfo> fakedTypes = new HashSet<>();
  private final Set<ProfileConnectorInfo> fakedConnectors = new HashSet<>();

  TestCodeGenerator(GeneratorContext generatorContext) {
    this.generatorContext = checkNotNull(generatorContext);
  }

  void generate() {
    if (generated) {
      throw new IllegalStateException("TestCodeGenerator#generate can only be called once");
    }
    generated = true;

    collectTestTypes();
    generateFakes();
  }

  private void generateFakes() {
    for (ProfileConnectorInfo connector : fakedConnectors) {
      new FakeProfileConnectorGenerator(generatorContext, connector).generate();
    }

    for (CrossProfileTypeInfo type : fakedTypes) {
      new FakeCrossProfileTypeGenerator(generatorContext, type).generate();
      new FakeOtherGenerator(generatorContext, type).generate();
    }
  }

  private void collectTestTypes() {
    for (CrossProfileTestInfo crossProfileTest : generatorContext.crossProfileTests()) {
      collectTestTypes(crossProfileTest);
    }
  }

  private void collectTestTypes(CrossProfileTestInfo crossProfileTest) {
    for (CrossProfileConfigurationInfo configuration : crossProfileTest.configurations()) {
      collectTestTypes(configuration);
    }
  }

  private void collectTestTypes(CrossProfileConfigurationInfo configuration) {
    for (ProviderClassInfo provider : configuration.providers()) {
      collectTestTypes(provider);
    }

    fakedConnectors.add(configuration.profileConnector());
  }

  private void collectTestTypes(ProviderClassInfo provider) {
    for (CrossProfileTypeInfo type : provider.allCrossProfileTypes()) {
      collectTestTypes(type);
    }
  }

  private void collectTestTypes(CrossProfileTypeInfo type) {
    fakedTypes.add(type);
  }
}
