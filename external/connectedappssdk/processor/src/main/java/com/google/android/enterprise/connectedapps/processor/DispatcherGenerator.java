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

import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.BACKGROUND_EXCEPTION_THROWER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.BINDER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CONTEXT_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CROSS_PROFILE_CALLBACK_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.CROSS_PROFILE_SENDER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_CALL_RECEIVER_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.CommonClassNames.PARCEL_UTILITIES_CLASSNAME;
import static com.google.android.enterprise.connectedapps.processor.ServiceGenerator.getConnectedAppsServiceClassName;
import static com.google.common.base.Preconditions.checkNotNull;
import static java.util.stream.Collectors.joining;

import com.google.android.enterprise.connectedapps.annotations.CrossProfileConfiguration;
import com.google.android.enterprise.connectedapps.processor.containers.CrossProfileConfigurationInfo;
import com.google.android.enterprise.connectedapps.processor.containers.GeneratorContext;
import com.google.android.enterprise.connectedapps.processor.containers.ProviderClassInfo;
import com.squareup.javapoet.AnnotationSpec;
import com.squareup.javapoet.ArrayTypeName;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.FieldSpec;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.TypeSpec;
import java.util.List;
import javax.lang.model.element.Modifier;

/**
 * Generate the {@code *_Dispatcher} class for a single {@link CrossProfileConfiguration} annotated
 * class.
 *
 * <p>This class includes the dispatch of calls to providers.
 *
 * <p>This must only be used once. It should be used after {@link EarlyValidator} has been used to
 * validate that the annotated code is correct.
 */
final class DispatcherGenerator {

  private boolean generated = false;
  private final GeneratorContext generatorContext;
  private final GeneratorUtilities generatorUtilities;
  private final CrossProfileConfigurationInfo configuration;

  DispatcherGenerator(
      GeneratorContext generatorContext, CrossProfileConfigurationInfo configuration) {
    this.generatorContext = checkNotNull(generatorContext);
    this.generatorUtilities = new GeneratorUtilities(generatorContext);
    this.configuration = checkNotNull(configuration);
  }

  void generate() {
    if (generated) {
      throw new IllegalStateException("DispatcherGenerator#generate can only be called once");
    }
    generated = true;

    generateDispatcherClass();
  }

  private void generateDispatcherClass() {
    ClassName className = getDispatcherClassName(generatorContext, configuration);

    TypeSpec.Builder classBuilder =
        TypeSpec.classBuilder(className)
            .addModifiers(Modifier.PUBLIC, Modifier.FINAL)
            .addJavadoc(
                "Class for dispatching calls to appropriate providers.\n\n"
                    + "<p>This uses a {@link $T} to construct calls before passing the completed"
                    + " call\n"
                    + "to a provider.\n",
                PARCEL_CALL_RECEIVER_CLASSNAME);

    classBuilder.addField(
        FieldSpec.builder(PARCEL_CALL_RECEIVER_CLASSNAME, "parcelCallReceiver")
            .addModifiers(Modifier.PRIVATE, Modifier.FINAL)
            .initializer("new $T()", PARCEL_CALL_RECEIVER_CLASSNAME)
            .build());

    addCallMethod(classBuilder);
    addPrepareCallMethod(classBuilder);
    addFetchResponseMethod(classBuilder);

    generatorUtilities.writeClassToFile(className.packageName(), classBuilder);
  }

  private static void addPrepareCallMethod(TypeSpec.Builder classBuilder) {
    MethodSpec prepareCallMethod =
        MethodSpec.methodBuilder("prepareCall")
            .addModifiers(Modifier.PUBLIC)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(long.class, "callId")
            .addParameter(int.class, "blockId")
            .addParameter(int.class, "numBytes")
            .addParameter(ArrayTypeName.of(byte.class), "paramBytes")
            .addStatement("parcelCallReceiver.prepareCall(callId, blockId, numBytes, paramBytes)")
            .addJavadoc(
                "Store a block of bytes to be part of a future call to\n"
                    + "{@link #call(Context, long, int, long, int, byte[], ICrossProfileCallback)}."
                    + "\n\n"
                    + "@param callId Arbitrary identifier used to link together\n"
                    + "    {@link #prepareCall(Context, long, int, int, byte[])} and\n    "
                    + "{@link #call(Context, long, int, long, int, byte[], ICrossProfileCallback)}"
                    + " calls.\n"
                    + "@param blockId The (zero indexed) number of this block. Each block should"
                    + " be\n    {@link $1T#MAX_BYTES_PER_BLOCK} bytes so the total number of blocks"
                    + " is\n    {@code numBytes / $1T#MAX_BYTES_PER_BLOCK}.\n"
                    + "@param numBytes The total number of bytes being transferred (across all"
                    + " blocks for this call,\n    including the final {@link"
                    + " #call(Context, long, int, long, int, byte[], ICrossProfileCallback)}.\n"
                    + "@param paramBytes The bytes for this block. Should contain\n    {@link"
                    + " $1T#MAX_BYTES_PER_BLOCK} bytes.\n\n"
                    + "@see $2T#prepareCall(long, int, int, byte[])",
                CROSS_PROFILE_SENDER_CLASSNAME,
                PARCEL_CALL_RECEIVER_CLASSNAME)
            .build();
    classBuilder.addMethod(prepareCallMethod);
  }

  private static void addFetchResponseMethod(TypeSpec.Builder classBuilder) {
    MethodSpec prepareCallMethod =
        MethodSpec.methodBuilder("fetchResponse")
            .addModifiers(Modifier.PUBLIC)
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(long.class, "callId")
            .addParameter(int.class, "blockId")
            .returns(ArrayTypeName.of(byte.class))
            .addStatement("return parcelCallReceiver.getPreparedResponse(callId, blockId)")
            .addJavadoc(
                "Fetch a response block if a previous call to\n {@link #call(Context, long, int,"
                    + " long, int, byte[], ICrossProfileCallback)} returned a\n byte array with"
                    + " 1 as the first byte.\n\n"
                    + "@param callId should be the same callId used with\n    {@link #call(Context,"
                    + " long, int, long, int, byte[], ICrossProfileCallback)}\n"
                    + "@param blockId The (zero indexed) number of the block to fetch.\n\n"
                    + "@see $1T#getPreparedResponse(long, int)\n",
                PARCEL_CALL_RECEIVER_CLASSNAME)
            .build();
    classBuilder.addMethod(prepareCallMethod);
  }

  private void addCallMethod(TypeSpec.Builder classBuilder) {
    CodeBlock.Builder methodCode = CodeBlock.builder();

    methodCode.beginControlFlow("try");

    methodCode.addStatement(
        "$1T parcel = parcelCallReceiver.getPreparedCall(callId, blockId, paramBytes)",
        PARCEL_CLASSNAME);

    List<ProviderClassInfo> providers = configuration.providers().asList();

    if (!providers.isEmpty()) {
      addProviderDispatch(methodCode, providers);
    }

    methodCode.addStatement(
        "throw new $T(\"Unknown type identifier \" + crossProfileTypeIdentifier)",
        IllegalArgumentException.class);

    methodCode.nextControlFlow("catch ($T e)", RuntimeException.class);
    // parcel is recycled in this method
    methodCode.addStatement("$1T throwableParcel = $1T.obtain()", PARCEL_CLASSNAME);
    methodCode.add("throwableParcel.writeInt(1); //errors\n");
    methodCode.addStatement(
        "$T.writeThrowableToParcel(throwableParcel, e)", PARCEL_UTILITIES_CLASSNAME);
    methodCode.addStatement(
        "$1T throwableBytes = parcelCallReceiver.prepareResponse(callId, throwableParcel)",
        ArrayTypeName.of(byte.class));
    methodCode.addStatement("throwableParcel.recycle()");

    methodCode.addStatement("$T.throwInBackground(e)", BACKGROUND_EXCEPTION_THROWER_CLASSNAME);

    methodCode.addStatement("return throwableBytes");
    methodCode.endControlFlow();

    MethodSpec callMethod =
        MethodSpec.methodBuilder("call")
            .addModifiers(Modifier.PUBLIC)
            .returns(ArrayTypeName.of(byte.class))
            .addAnnotation(AnnotationSpec.builder(SuppressWarnings.class)
                // Allow catching of RuntimeException
                .addMember("value", "\"CatchSpecificExceptionsChecker\"")
                .build())
            .addParameter(CONTEXT_CLASSNAME, "context")
            .addParameter(long.class, "callId")
            .addParameter(int.class, "blockId")
            .addParameter(long.class, "crossProfileTypeIdentifier")
            .addParameter(int.class, "methodIdentifier")
            .addParameter(ArrayTypeName.of(byte.class), "paramBytes")
            .addParameter(CROSS_PROFILE_CALLBACK_CLASSNAME, "callback")
            .addCode(methodCode.build())
            .addJavadoc(
                "Make a call, which will execute some annotated method and return a response.\n\n"
                    + "<p>The parameters to the call should be contained in a {@link $1T}"
                    + " marshalled into\n"
                    + "a byte array. If the byte array is larger than {@link"
                    + " $2T#MAX_BYTES_PER_BLOCK},\n"
                    + "then it should be separated into blocks of {@link"
                    + " $2T#MAX_BYTES_PER_BLOCK}\n"
                    + "bytes, and {@link #prepareCall(Context, long, int, int, byte[])} used to"
                    + " set all but the final\n"
                    + "block, before calling this method with the final block.\n\n"
                    + "<p>The response will be an array of bytes. If the response is complete (it"
                    + " fits into a single\n"
                    + "block), then the first byte will be 0, otherwise the first byte will be 1"
                    + " and the next 4 bytes\n"
                    + "will be an int representing the total size of the return value. The rest of"
                    + " the bytes are the\n"
                    + "first block of the return value. {@link #fetchResponse(Context, long, int)"
                    + " should be used to\n"
                    + "fetch further blocks.\n\n"
                    + "@param callId Arbitrary identifier used to link together\n"
                    + "    {@link #prepareCall(Context, long, int, int, byte[])} and\n"
                    + "    {@link #call(Context, long, int, long, int, byte[],"
                    + " ICrossProfileCallback)} calls.\n"
                    + "@param blockId The (zero indexed) number of this block. Each block should"
                    + " be\n    {@link CrossProfileSender#MAX_BYTES_PER_BLOCK} bytes so the total"
                    + " number of blocks is\n    {@code numBytes /"
                    + " CrossProfileSender#MAX_BYTES_PER_BLOCK}.\n"
                    + "@param crossProfileTypeIdentifier The generated identifier for the type"
                    + " which contains the\n    method being called.\n"
                    + "@param methodIdentifier The index of the method being called on the cross"
                    + " profile type.\n"
                    + "@param paramBytes The bytes for the final block, this will be merged with"
                    + " any blocks\n    previously set by a call to"
                    + " {@link #prepareCall(Context, long, int, int, byte[])}.\n"
                    + "@param callback A callback to be used if this is an asynchronous call."
                    + " Otherwise this should be\n    {@code null}.\n\n"
                    + "@see $3T#getPreparedCall(long, int, byte[])\n",
                PARCEL_CLASSNAME,
                CROSS_PROFILE_SENDER_CLASSNAME,
                PARCEL_CALL_RECEIVER_CLASSNAME)
            .build();

    classBuilder.addMethod(callMethod);
  }

  private void addProviderDispatch(
      CodeBlock.Builder methodCode, List<ProviderClassInfo> providers) {
    for (ProviderClassInfo provider : providers) {
      addProviderDispatchInner(methodCode, provider);
    }
  }

  private void addProviderDispatchInner(CodeBlock.Builder methodCode, ProviderClassInfo provider) {
    String condition =
        provider.allCrossProfileTypes().stream()
            .map(
                h ->
                    "crossProfileTypeIdentifier == "
                        + h.identifier()
                        + "L // "
                        + h.crossProfileTypeElement().getQualifiedName()
                        + "\n")
            .collect(joining(" || "));

    methodCode.beginControlFlow("if ($L)", condition);
    methodCode.addStatement(
        "$1T returnParcel = $2T.instance().call(context.getApplicationContext(),"
            + " crossProfileTypeIdentifier, methodIdentifier, parcel, callback)",
        PARCEL_CLASSNAME,
        InternalProviderClassGenerator.getInternalProviderClassName(generatorContext, provider));
    methodCode.addStatement(
        "$1T returnBytes = parcelCallReceiver.prepareResponse(callId, returnParcel)",
        ArrayTypeName.of(byte.class));
    methodCode.addStatement("parcel.recycle()");
    methodCode.addStatement("returnParcel.recycle()");
    methodCode.addStatement("return returnBytes");
    methodCode.endControlFlow();
  }

  static ClassName getDispatcherClassName(
      GeneratorContext generatorContext, CrossProfileConfigurationInfo configuration) {
    ClassName serviceName = getConnectedAppsServiceClassName(generatorContext, configuration);
    return ClassName.get(serviceName.packageName(), serviceName.simpleName() + "_Dispatcher");
  }
}
