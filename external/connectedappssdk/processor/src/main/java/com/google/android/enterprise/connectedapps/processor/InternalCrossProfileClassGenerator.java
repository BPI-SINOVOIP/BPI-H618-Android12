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

import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.BUNDLER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CONTEXT_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CROSS_PROFILE_CALLBACK_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CROSS_PROFILE_FUTURE_RESULT_WRITER;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.METHOD_RUNNER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_UTILITIES_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.containers.CrossProfileMethodInfo.AutomaticallyResolvedParameterFilterBehaviour.REPLACE_AUTOMATICALLY_RESOLVED_PARAMETERS;
import static com.google.common.base.Preconditions.checkNotNull;
import static java.util.stream.Collectors.joining;

import com.google.android.enterprise.connectedapps.annotations.CrossProfile;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileCallbackInterfaceInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileMethodInfo;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileTypeInfo;
import com.google.android.enterprise.connectedapps.processor.containers.FutureWrapper;
import com.google.android.enterprise.connectedapps.processor.containers.GeneratorContext;
import com.google.android.enterprise.connectedapps.processor.containers.ProviderClassInfo;
import com.squareup.javapoet.ArrayTypeName;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.FieldSpec;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import java.util.Optional;
import java.util.stream.IntStream;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;

/**
 * Generate the {@code Profile_*_Internal} class for a single {@link CrossProfile} type.
 *
 * <p>This must only be used once. It should be used after {@link EarlyValidator} has been used to
 * validate that the annotated code is correct.
 */
final class InternalCrossProfileClassGenerator {

  private boolean generated = false;
  private final GeneratorContext generatorContext;
  private final GeneratorUtilities generatorUtilities;
  private final ProviderClassInfo providerClass;
  private final CrossProfileTypeInfo crossProfileType;

  InternalCrossProfileClassGenerator(
      GeneratorContext generatorContext,
      ProviderClassInfo providerClass,
      CrossProfileTypeInfo crossProfileType) {
    this.generatorContext = checkNotNull(generatorContext);
    this.generatorUtilities = new GeneratorUtilities(generatorContext);
    this.providerClass = checkNotNull(providerClass);
    this.crossProfileType = checkNotNull(crossProfileType);
  }

  void generate() {
    if (generated) {
      throw new IllegalStateException(
          "InternalCrossProfileClassGenerator#generate can only be called once");
    }
    generated = true;

    generateInternalCrossProfileClass();
  }

  private void generateInternalCrossProfileClass() {
    ClassName className = getInternalCrossProfileClassName(generatorContext, crossProfileType);

    TypeSpec.Builder classBuilder =
        TypeSpec.classBuilder(className).addModifiers(Modifier.PUBLIC, Modifier.FINAL);

    classBuilder.addJavadoc(
        "Internal class for {@link $T}.\n\n"
            + "<p>This is used by the Connected Apps SDK to dispatch cross-profile calls.\n\n"
            + "<p>Cross-profile type identifier: $L.\n",
        crossProfileType.crossProfileTypeElement().asType(),
        crossProfileType.identifier());

    classBuilder.addField(
        FieldSpec.builder(className, "instance")
            .addModifiers(Modifier.PRIVATE, Modifier.STATIC, Modifier.FINAL)
            .initializer("new $T()", className)
            .build());

    classBuilder.addField(
        FieldSpec.builder(BUNDLER_CLASSNAME, "bundler")
            .addModifiers(Modifier.PRIVATE, Modifier.STATIC, Modifier.FINAL)
            .initializer(
                "new $T()",
                BundlerGenerator.getBundlerClassName(generatorContext, crossProfileType))
            .build());

    if (!crossProfileType.isStatic()) {
      ExecutableElement providerMethod =
          providerClass.findProviderMethodFor(generatorContext, crossProfileType);
      String paramsString = providerMethod.getParameters().isEmpty() ? "()" : "(context)";
      CodeBlock providerMethodCall =
          CodeBlock.of("$L$L", providerMethod.getSimpleName(), paramsString);

      classBuilder.addMethod(
          MethodSpec.methodBuilder("crossProfileType")
              .addParameter(CONTEXT_CLASSNAME, "context")
              .returns(crossProfileType.className())
              .addStatement(
                  "return $T.instance().providerClass(context).$L",
                  InternalProviderClassGenerator.getInternalProviderClassName(
                      generatorContext, providerClass),
                  providerMethodCall)
              .build());
    }

    classBuilder.addMethod(
        MethodSpec.methodBuilder("bundler")
            .returns(BUNDLER_CLASSNAME)
            .addStatement("return bundler")
            .build());

    classBuilder.addMethod(
        MethodSpec.methodBuilder("instance")
            .addModifiers(Modifier.STATIC)
            .returns(className)
            .addStatement("return instance")
            .build());

    classBuilder.addMethod(MethodSpec.constructorBuilder().addModifiers(Modifier.PRIVATE).build());

    addMethodsField(classBuilder, crossProfileType);
    addCrossProfileTypeMethods(classBuilder, crossProfileType);
    addCallMethod(classBuilder);

    generatorUtilities.writeClassToFile(className.packageName(), classBuilder);
  }

  private void addMethodsField(
      TypeSpec.Builder classBuilder, CrossProfileTypeInfo crossProfileType) {
    int totalMethods = crossProfileType.crossProfileMethods().size();

    classBuilder.addField(
        FieldSpec.builder(ArrayTypeName.of(METHOD_RUNNER_CLASSNAME), "methods")
            .addModifiers(Modifier.PRIVATE)
            .initializer(
                "new $T[]{$L}",
                METHOD_RUNNER_CLASSNAME,
                IntStream.range(0, totalMethods)
                    .mapToObj(n -> "this::method" + n)
                    .collect(joining(",")))
            .build());
  }

  private void addCrossProfileTypeMethods(
      TypeSpec.Builder classBuilder, CrossProfileTypeInfo crossProfileType) {
    for (CrossProfileMethodInfo method : crossProfileType.crossProfileMethods()) {
      if (method.isBlocking(generatorContext, crossProfileType)) {
        addBlockingCrossProfileTypeMethod(classBuilder, method);
      } else if (method.isCrossProfileCallback(generatorContext)) {
        addCrossProfileCallbackCrossProfileTypeMethod(classBuilder, method);
      } else if (method.isFuture(crossProfileType)) {
        addFutureCrossProfileTypeMethod(classBuilder, method);
      } else {
        throw new IllegalStateException("Unknown method type: " + method);
      }
    }
  }

  private void addBlockingCrossProfileTypeMethod(
      TypeSpec.Builder classBuilder, CrossProfileMethodInfo method) {
    CodeBlock.Builder methodCode = CodeBlock.builder();

    // parcle is recycled by caller
    methodCode.addStatement("$1T returnParcel = $1T.obtain()", PARCEL_CLASSNAME);

    addExtractParametersCode(methodCode, method);

    CodeBlock methodCall =
        CodeBlock.of(
            "$L.$L($L)",
            getCrossProfileTypeReference(method),
            method.simpleName(),
            method.commaSeparatedParameters(
                crossProfileType.supportedTypes(), REPLACE_AUTOMATICALLY_RESOLVED_PARAMETERS));

    if (!method.thrownExceptions().isEmpty()) {
      methodCode.beginControlFlow("try");
    }

    if (isPrimitiveOrObjectVoid(method.returnType())) {
      methodCode.addStatement(methodCall);
    } else {
      methodCall = CodeBlock.of("$T returnValue = $L", method.returnType(), methodCall);
      methodCode.addStatement(methodCall);
      methodCode.add("returnParcel.writeInt(0); // No errors\n");
      methodCode.addStatement(
          "bundler.writeToParcel(returnParcel, returnValue, $L, /* flags= */ 0)",
          TypeUtils.generateBundlerType(method.returnType()));
    }

    if (!method.thrownExceptions().isEmpty()) {
      for (TypeName exceptionType : method.thrownExceptions()) {
        methodCode.nextControlFlow("catch ($L e)", exceptionType);
        methodCode.add("returnParcel.writeInt(1); // Errors\n");
        methodCode.addStatement(
            "$T.writeThrowableToParcel(returnParcel, e)", PARCEL_UTILITIES_CLASSNAME);
      }
      methodCode.endControlFlow();
    }

    methodCode.addStatement("return returnParcel");

    classBuilder.addMethod(
        MethodSpec.methodBuilder("method" + method.identifier())
            .addModifiers(Modifier.PRIVATE)
            .returns(PARCEL_CLASSNAME)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(PARCEL_CLASSNAME, "params")
            .addParameter(CROSS_PROFILE_CALLBACK_CLASSNAME, "callback")
            .addCode(methodCode.build())
            .addJavadoc(
                "Call $1L and return a {@link $2T} containing the return value.\n\n"
                    + "<p>The {@link $2T} must be recycled after use.\n",
                GeneratorUtilities.methodJavadocReference(method.methodElement()),
                PARCEL_CLASSNAME)
            .build());
  }

  private void addCrossProfileCallbackCrossProfileTypeMethod(
      TypeSpec.Builder classBuilder, CrossProfileMethodInfo method) {
    CodeBlock.Builder methodCode = CodeBlock.builder();

    // parcel is recycled by caller
    methodCode.addStatement("$1T returnParcel = $1T.obtain()", PARCEL_CLASSNAME);

    addExtractParametersCode(methodCode, method);

    createCrossProfileCallbackParameter(methodCode, method);

    CodeBlock methodCall =
        CodeBlock.of(
            "$L.$L($L)",
            getCrossProfileTypeReference(method),
            method.simpleName(),
            method.commaSeparatedParameters(
                crossProfileType.supportedTypes(), REPLACE_AUTOMATICALLY_RESOLVED_PARAMETERS));

    if (isPrimitiveOrObjectVoid(method.returnType())) {
      methodCode.addStatement(methodCall);
    } else {
      methodCall = CodeBlock.of("$T returnValue = $L", method.returnType(), methodCall);
      methodCode.addStatement(methodCall);
      methodCode.add("returnParcel.writeInt(0); // No errors\n");
      methodCode.addStatement(
          "bundler.writeToParcel(returnParcel, returnValue, $L, /* flags= */ 0)",
          TypeUtils.generateBundlerType(method.returnType()));
    }

    methodCode.addStatement("return returnParcel");

    classBuilder.addMethod(
        MethodSpec.methodBuilder("method" + method.identifier())
            .addModifiers(Modifier.PRIVATE)
            .returns(PARCEL_CLASSNAME)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(PARCEL_CLASSNAME, "params")
            // TODO: This should be renamed to "callback" once we prefix unpacked parameter names
            //  (without doing this, a param named "callback" will cause a compile error)
            .addParameter(CROSS_PROFILE_CALLBACK_CLASSNAME, "crossProfileCallback")
            .addCode(methodCode.build())
            .addJavadoc(
                "Call $1L, and link the callback to {@code crossProfileCallback}.\n\n"
                    + "@return An empty parcel. This must be recycled after use.\n",
                GeneratorUtilities.methodJavadocReference(method.methodElement()))
            .build());
  }

  private void addFutureCrossProfileTypeMethod(
      TypeSpec.Builder classBuilder, CrossProfileMethodInfo method) {
    CodeBlock.Builder methodCode = CodeBlock.builder();

    // parcel is recycled by caller
    methodCode.addStatement("$1T returnParcel = $1T.obtain()", PARCEL_CLASSNAME);

    addExtractParametersCode(methodCode, method);

    CodeBlock methodCall =
        CodeBlock.of(
            "$L.$L($L)",
            getCrossProfileTypeReference(method),
            method.simpleName(),
            method.commaSeparatedParameters(
                crossProfileType.supportedTypes(), REPLACE_AUTOMATICALLY_RESOLVED_PARAMETERS));

    methodCode.addStatement("$T future = $L", method.returnType(), methodCall);

    TypeMirror rawFutureType = TypeUtils.removeTypeArguments(method.returnType());

    FutureWrapper futureWrapper =
        crossProfileType.supportedTypes().getType(rawFutureType).getFutureWrapper().get();
    // This assumes every Future is generic with one type argument
    TypeMirror wrappedReturnType =
        TypeUtils.extractTypeArguments(method.returnType()).iterator().next();
    methodCode.addStatement(
        "$T.writeFutureResult(future, new $T<>(callback, bundler, $L))",
        futureWrapper.wrapperClassName(),
        CROSS_PROFILE_FUTURE_RESULT_WRITER,
        TypeUtils.generateBundlerType(wrappedReturnType));

    // TODO: Can this just return null? where does it go? that'd avoid having to obtain/recycle
    methodCode.addStatement("return returnParcel");

    classBuilder.addMethod(
        MethodSpec.methodBuilder("method" + method.identifier())
            .addModifiers(Modifier.PRIVATE)
            .returns(PARCEL_CLASSNAME)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(PARCEL_CLASSNAME, "params")
            .addParameter(CROSS_PROFILE_CALLBACK_CLASSNAME, "callback")
            .addCode(methodCode.build())
            .addJavadoc(
                "Call $1L, and link the returned future to {@code crossProfileCallback}.\n\n"
                    + "@return An empty parcel. This must be recycled after use.\n",
                GeneratorUtilities.methodJavadocReference(method.methodElement()))
            .build());
  }

  private void createCrossProfileCallbackParameter(
      CodeBlock.Builder methodCode, CrossProfileMethodInfo method) {
    VariableElement asyncCallbackParam =
        method.getCrossProfileCallbackParam(generatorContext).get();

    TypeElement callbackType =
        generatorContext.elements().getTypeElement(asyncCallbackParam.asType().toString());
    CrossProfileCallbackInterfaceInfo callbackInterface =
        CrossProfileCallbackInterfaceInfo.create(callbackType);

    methodCode.addStatement(
        "$T $L = new $L(crossProfileCallback, bundler)",
        asyncCallbackParam.asType(),
        asyncCallbackParam.getSimpleName(),
        CrossProfileCallbackCodeGenerator.getCrossProfileCallbackReceiverClassName(
            generatorContext, callbackInterface));
  }

  private static boolean isPrimitiveOrObjectVoid(TypeMirror typeMirror) {
    return typeMirror.getKind().equals(TypeKind.VOID)
        || typeMirror.toString().equals("java.lang.Void");
  }

  private void addExtractParametersCode(CodeBlock.Builder code, CrossProfileMethodInfo method) {
    Optional<VariableElement> callbackParameter =
        method.getCrossProfileCallbackParam(generatorContext);
    for (VariableElement parameter : method.methodElement().getParameters()) {
      if (callbackParameter.isPresent()
          && callbackParameter.get().getSimpleName().equals(parameter.getSimpleName())) {
        continue; // Don't extract a callback parameter
      }
      if (crossProfileType.supportedTypes().isAutomaticallyResolved(parameter.asType())) {
        continue;
      }
      code.addStatement(
          "@SuppressWarnings(\"unchecked\") $1T $2L = ($1T) bundler.readFromParcel(params, $3L)",
          parameter.asType(),
          parameter.getSimpleName().toString(),
          TypeUtils.generateBundlerType(parameter.asType()));
    }
  }

  private static void addCallMethod(TypeSpec.Builder classBuilder) {
    classBuilder.addMethod(
        MethodSpec.methodBuilder("call")
            .addModifiers(Modifier.PUBLIC)
            .returns(PARCEL_CLASSNAME)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(int.class, "methodIdentifier")
            .addParameter(PARCEL_CLASSNAME, "params")
            .addParameter(CROSS_PROFILE_CALLBACK_CLASSNAME, "callback")
            .beginControlFlow("if (methodIdentifier >= methods.length)")
            .addStatement(
                "throw new $T(\"Invalid method identifier\" + methodIdentifier)",
                IllegalArgumentException.class)
            .endControlFlow()
            .addStatement("return methods[methodIdentifier].call(context, params, callback)")
            .addJavadoc(
                "Call the method referenced by {@code methodIdentifier}.\n\n"
                    + "<p>If the method is synchronous, this will return a {@link $1T} containing"
                    + " the return value, otherwise it will return an empty {@link $1T}. The"
                    + " {@link $1T} must be recycled after use.\n",
                PARCEL_CLASSNAME)
            .build());
  }

  private CodeBlock getCrossProfileTypeReference(CrossProfileMethodInfo method) {
    if (method.isStatic()) {
      return CodeBlock.of("$1T", crossProfileType.className());
    }
    return CodeBlock.of("crossProfileType(context)");
  }

  static ClassName getInternalCrossProfileClassName(
      GeneratorContext generatorContext, CrossProfileTypeInfo crossProfileType) {
    return GeneratorUtilities.appendToClassName(crossProfileType.profileClassName(), "_Internal");
  }
}
