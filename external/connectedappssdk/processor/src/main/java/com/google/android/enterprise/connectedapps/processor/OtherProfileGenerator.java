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

import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.EXCEPTION_CALLBACK_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.LOCAL_CALLBACK_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PROFILE_CONNECTOR_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.containers.CrossProfileMethodInfo.AutomaticallyResolvedParameterFilterBehaviour.REMOVE_AUTOMATICALLY_RESOLVED_PARAMETERS;
import static com.google.common.base.Preconditions.checkNotNull;

import com.google.android.enterprise.connectedapps.processor.annotationdiscovery.interfaces.CrossProfileAnnotation;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileCallbackInterfaceInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileMethodInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileTypeInfo;
import com.google.android.enterprise.connectedapps.processor.containers.FutureWrapper;
import com.google.android.enterprise.connectedapps.processor.containers.GeneratorContext;
import com.squareup.javapoet.AnnotationSpec;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.FieldSpec;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;

/**
 * Generate the {@code Profile_*_OtherProfile} class for a single cross-profile type.
 *
 * <p>This must only be used once. It should be used after {@link EarlyValidator} has been used to
 * validate that the annotated code is correct.
 */
final class OtherProfileGenerator {

  private boolean generated = false;
  private final GeneratorContext generatorContext;
  private final GeneratorUtilities generatorUtilities;
  private final CrossProfileTypeInfo crossProfileType;

  OtherProfileGenerator(GeneratorContext generatorContext, CrossProfileTypeInfo crossProfileType) {
    this.generatorContext = checkNotNull(generatorContext);
    this.generatorUtilities = new GeneratorUtilities(generatorContext);
    this.crossProfileType = checkNotNull(crossProfileType);
  }

  void generate() {
    if (generated) {
      throw new IllegalStateException("OtherProfileGenerator#generate can only be called once");
    }
    generated = true;

    generateOtherProfileClass();
  }

  private void generateOtherProfileClass() {
    ClassName className = getOtherProfileClassName(generatorContext, crossProfileType);

    ClassName singleSenderCanThrowInterface =
        InterfaceGenerator.getSingleSenderCanThrowInterfaceClassName(
            generatorContext, crossProfileType);

    TypeSpec.Builder classBuilder =
        TypeSpec.classBuilder(className)
            .addJavadoc(
                "Implementation of {@link $T} used when interacting with the other profile.\n",
                singleSenderCanThrowInterface)
            .addModifiers(Modifier.PUBLIC, Modifier.FINAL)
            .addSuperinterface(singleSenderCanThrowInterface);

    ClassName connectorClassName =
        crossProfileType.profileConnector().isPresent()
            ? crossProfileType.profileConnector().get().connectorClassName()
            : PROFILE_CONNECTOR_CLASSNAME;

    classBuilder.addField(connectorClassName, "connector", Modifier.PRIVATE, Modifier.FINAL);

    classBuilder.addMethod(
        MethodSpec.constructorBuilder()
            .addModifiers(Modifier.PUBLIC)
            .addParameter(connectorClassName, "connector")
            .beginControlFlow("if (connector == null)")
            .addStatement("throw new $T()", NullPointerException.class)
            .endControlFlow()
            .addStatement("this.connector = connector")
            .build());

    classBuilder.addField(
        FieldSpec.builder(long.class, "timeout")
            .addAnnotation(
                AnnotationSpec.builder(SuppressWarnings.class)
                    .addMember("value", "$S", "GoodTime")
                    .build())
            .addModifiers(Modifier.PRIVATE)
            .initializer("$L", CrossProfileAnnotation.TIMEOUT_MILLIS_NOT_SET)
            .build());

    classBuilder.addMethod(
        MethodSpec.methodBuilder("timeout")
            .addAnnotation(Override.class)
            .addAnnotation(
                AnnotationSpec.builder(SuppressWarnings.class)
                    .addMember("value", "$S", "GoodTime")
                    .build())
            .addModifiers(Modifier.PUBLIC)
            .returns(className)
            .addParameter(long.class, "timeout")
            .addStatement("this.timeout = timeout")
            .addStatement("return this")
            .build());

    ClassName ifAvailableClass =
        IfAvailableGenerator.getIfAvailableClassName(generatorContext, crossProfileType);

    classBuilder.addMethod(
        MethodSpec.methodBuilder("ifAvailable")
            .addAnnotation(Override.class)
            .addModifiers(Modifier.PUBLIC)
            .returns(ifAvailableClass)
            .addStatement("return new $T(this)", ifAvailableClass)
            .build());

    for (CrossProfileMethodInfo method : crossProfileType.crossProfileMethods()) {
      if (method.isBlocking(generatorContext, crossProfileType)) {
        generateBlockingMethodOnOtherProfileClass(classBuilder, method, crossProfileType);
      } else if (method.isCrossProfileCallback(generatorContext)) {
        generateCrossProfileCallbackMethodOnOtherProfileClass(
            classBuilder, method, crossProfileType);
      } else if (method.isFuture(crossProfileType)) {
        generateFutureMethodOnOtherProfileClass(classBuilder, method, crossProfileType);
      } else {
        throw new IllegalStateException("Unknown method type: " + method);
      }
    }

    generatorUtilities.writeClassToFile(className.packageName(), classBuilder);
  }

  private void generateBlockingMethodOnOtherProfileClass(
      TypeSpec.Builder classBuilder,
      CrossProfileMethodInfo method,
      CrossProfileTypeInfo crossProfileType) {

    MethodSpec.Builder methodBuilder =
        MethodSpec.methodBuilder(method.simpleName())
            .addAnnotation(Override.class)
            .addModifiers(Modifier.PUBLIC)
            .addExceptions(method.thrownExceptions())
            .addException(UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME)
            .returns(method.returnTypeTypeName())
            .addParameters(
                GeneratorUtilities.extractParametersFromMethod(
                    crossProfileType.supportedTypes(),
                    method.methodElement(),
                    REMOVE_AUTOMATICALLY_RESOLVED_PARAMETERS));

    methodBuilder.addStatement(
        "$1T internalCrossProfileClass = $1T.instance()",
        InternalCrossProfileClassGenerator.getInternalCrossProfileClassName(
            generatorContext, crossProfileType));

    // parcel is recycled in this method
    methodBuilder.addStatement("$1T params = $1T.obtain()", PARCEL_CLASSNAME);
    for (VariableElement param : method.methodElement().getParameters()) {
      if (crossProfileType.supportedTypes().isAutomaticallyResolved(param.asType())) {
        continue;
      }
      methodBuilder.addStatement(
          "internalCrossProfileClass.bundler().writeToParcel(params, $1L, $2L, /* flags= */ 0)",
          param.getSimpleName(),
          TypeUtils.generateBundlerType(param.asType()));
    }

    if (method.thrownExceptions().isEmpty()) {
      methodBuilder.addStatement(
          "$1T returnParcel = connector.crossProfileSender().call($2LL, $3L, params)",
          PARCEL_CLASSNAME,
          crossProfileType.identifier(),
          method.identifier());
    } else {
      methodBuilder.addStatement("$1T returnParcel", PARCEL_CLASSNAME);
      methodBuilder.beginControlFlow("try");
      methodBuilder.addStatement(
          "returnParcel = connector.crossProfileSender().callWithExceptions($1LL, $2L, params)",
          crossProfileType.identifier(),
          method.identifier());
      methodBuilder.nextControlFlow("catch ($T e)", UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME);
      methodBuilder.addStatement("throw e");

      for (TypeName exception : method.thrownExceptions()) {
        methodBuilder.nextControlFlow("catch ($T e)", exception);
        methodBuilder.addStatement("throw e");
      }

      methodBuilder.nextControlFlow("catch ($T e)", Throwable.class);
      methodBuilder.addStatement(
          "throw new $T($S)", IllegalStateException.class, "Unexpected exception thrown");
      methodBuilder.endControlFlow();
    }

    methodBuilder.addStatement("params.recycle()");

    if (!method.returnType().getKind().equals(TypeKind.VOID)) {
      methodBuilder.addStatement(
          CodeBlock.of(
              "@SuppressWarnings(\"unchecked\") $1T returnValue = ($1T)"
                  + " internalCrossProfileClass.bundler().readFromParcel(returnParcel,"
                  + " $2L)",
              method.returnType(),
              TypeUtils.generateBundlerType(method.returnType())));
      methodBuilder.addStatement("returnParcel.recycle()");
      methodBuilder.addStatement("return returnValue");
    } else {
      methodBuilder.addStatement("returnParcel.recycle()");
    }

    classBuilder.addMethod(methodBuilder.build());
  }

  private void generateCrossProfileCallbackMethodOnOtherProfileClass(
      TypeSpec.Builder classBuilder,
      CrossProfileMethodInfo method,
      CrossProfileTypeInfo crossProfileType) {
    VariableElement callbackParameter = method.getCrossProfileCallbackParam(generatorContext).get();
    TypeElement callbackType =
        generatorContext.elements().getTypeElement(callbackParameter.asType().toString());
    CrossProfileCallbackInterfaceInfo callbackInterface =
        CrossProfileCallbackInterfaceInfo.create(callbackType);

    MethodSpec.Builder methodBuilder =
        MethodSpec.methodBuilder(method.simpleName())
            .addAnnotation(Override.class)
            .addModifiers(Modifier.PUBLIC)
            .returns(method.returnTypeTypeName())
            .addParameters(
                GeneratorUtilities.extractParametersFromMethod(
                    crossProfileType.supportedTypes(),
                    method.methodElement(),
                    REMOVE_AUTOMATICALLY_RESOLVED_PARAMETERS))
            .addParameter(EXCEPTION_CALLBACK_CLASSNAME, "exceptionCallback");

    methodBuilder.addStatement(
        "$1T internalCrossProfileClass = $1T.instance()",
        InternalCrossProfileClassGenerator.getInternalCrossProfileClassName(
            generatorContext, crossProfileType));

    // parcel is passed into callAsync where it will be cached and recycled afterwards
    methodBuilder.addStatement("$1T params = $1T.obtain()", PARCEL_CLASSNAME);

    for (VariableElement param : method.methodElement().getParameters()) {
      if (crossProfileType.supportedTypes().isAutomaticallyResolved(param.asType())) {
        continue;
      }
      if (param.getSimpleName().equals(callbackParameter.getSimpleName())) {
        continue;
      }
      methodBuilder.addStatement(
          "internalCrossProfileClass.bundler().writeToParcel(params, $1L, $2L, /* flags= */ 0)",
          param.getSimpleName(),
          TypeUtils.generateBundlerType(param.asType()));
    }

    methodBuilder.addStatement(
        "$1T sender = new $2T($3L, exceptionCallback, internalCrossProfileClass.bundler())",
        LOCAL_CALLBACK_CLASSNAME,
        CrossProfileCallbackCodeGenerator.getCrossProfileCallbackSenderClassName(
            generatorContext, callbackInterface),
        callbackParameter.getSimpleName());

    // Suppress GoodTime warning for unboxing Duration.
    methodBuilder.addAnnotation(
        AnnotationSpec.builder(SuppressWarnings.class)
            .addMember("value", "$S", "GoodTime")
            .build());
    methodBuilder.addStatement(
        "connector.crossProfileSender().callAsync($1LL, $2L, params, sender, timeout =="
            + " $3L ? $4L : timeout)",
        crossProfileType.identifier(),
        method.identifier(),
        CrossProfileAnnotation.TIMEOUT_MILLIS_NOT_SET,
        method.timeoutMillis());

    methodBuilder.addComment(
        "We don't recycle the params as they will be stored for the async call and recycled"
            + " afterwards");

    classBuilder.addMethod(methodBuilder.build());
  }

  private void generateFutureMethodOnOtherProfileClass(
      TypeSpec.Builder classBuilder,
      CrossProfileMethodInfo method,
      CrossProfileTypeInfo crossProfileType) {

    MethodSpec.Builder methodBuilder =
        MethodSpec.methodBuilder(method.simpleName())
            .addAnnotation(Override.class)
            .addModifiers(Modifier.PUBLIC)
            .returns(method.returnTypeTypeName())
            .addParameters(
                GeneratorUtilities.extractParametersFromMethod(
                    crossProfileType.supportedTypes(),
                    method.methodElement(),
                    REMOVE_AUTOMATICALLY_RESOLVED_PARAMETERS));

    methodBuilder.addStatement(
        "$1T internalCrossProfileClass = $1T.instance()",
        InternalCrossProfileClassGenerator.getInternalCrossProfileClassName(
            generatorContext, crossProfileType));

    // parcel is passed into callAsync where it will be cached and recycled afterwards
    methodBuilder.addStatement("$1T params = $1T.obtain()", PARCEL_CLASSNAME);
    for (VariableElement param : method.methodElement().getParameters()) {
      if (crossProfileType.supportedTypes().isAutomaticallyResolved(param.asType())) {
        continue;
      }
      methodBuilder.addStatement(
          "internalCrossProfileClass.bundler().writeToParcel(params, $1L, $2L, /* flags= */ 0)",
          param.getSimpleName(),
          TypeUtils.generateBundlerType(param.asType()));
    }

    TypeMirror rawFutureType = TypeUtils.removeTypeArguments(method.returnType());

    FutureWrapper futureWrapper =
        crossProfileType.supportedTypes().getType(rawFutureType).getFutureWrapper().get();
    // This assumes every Future is generic with one type argument
    TypeMirror wrappedReturnType =
        TypeUtils.extractTypeArguments(method.returnType()).iterator().next();
    methodBuilder.addStatement(
        "$1T<$2T> futureWrapper = $1T.create(internalCrossProfileClass.bundler(), $3L)",
        futureWrapper.wrapperClassName(),
        wrappedReturnType,
        TypeUtils.generateBundlerType(wrappedReturnType));

    methodBuilder.addAnnotation(
        AnnotationSpec.builder(SuppressWarnings.class)
            .addMember("value", "$S", "GoodTime")
            .build());
    methodBuilder.addStatement(
        "connector.crossProfileSender().callAsync($1LL, $2L, params, futureWrapper,"
            + " timeout == $3L ? $4L : timeout)",
        crossProfileType.identifier(),
        method.identifier(),
        CrossProfileAnnotation.TIMEOUT_MILLIS_NOT_SET,
        method.timeoutMillis());

    methodBuilder.addComment(
        "We don't recycle the params as they will be stored for the async call and recycled"
            + " afterwards");

    methodBuilder.addStatement("return futureWrapper.getFuture()");

    classBuilder.addMethod(methodBuilder.build());
  }

  static ClassName getOtherProfileClassName(
      GeneratorContext generatorContext, CrossProfileTypeInfo crossProfileType) {
    return GeneratorUtilities.appendToClassName(
        crossProfileType.profileClassName(), "_OtherProfile");
  }
}
