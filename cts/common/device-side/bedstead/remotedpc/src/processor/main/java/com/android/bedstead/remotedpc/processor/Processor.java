/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.bedstead.remotedpc.processor;

import com.android.bedstead.remotedpc.processor.annotations.RemoteDpcAutomaticAdmin;
import com.android.bedstead.remotedpc.processor.annotations.RemoteDpcManager;

import com.google.android.enterprise.connectedapps.annotations.CrossProfile;
import com.google.android.enterprise.connectedapps.annotations.CrossProfileConfiguration;
import com.google.android.enterprise.connectedapps.annotations.CrossProfileProvider;
import com.google.auto.service.AutoService;
import com.squareup.javapoet.AnnotationSpec;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterSpec;
import com.squareup.javapoet.TypeSpec;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedAnnotationTypes;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.PackageElement;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.MirroredTypeException;
import javax.lang.model.type.TypeKind;
import javax.tools.Diagnostic;
import javax.tools.JavaFileObject;

/** Processor for generating RemoteDPC API for framework manager classes. */
@SupportedAnnotationTypes({
        "com.android.bedstead.remotedpc.processor.annotations.RemoteDpcManager",
})
@AutoService(javax.annotation.processing.Processor.class)
public final class Processor extends AbstractProcessor {
    // TODO(scottjonathan): Add more verification before generating - and add processor tests
    private static final ClassName CONTEXT_CLASSNAME =
            ClassName.get("android.content", "Context");
    private static final ClassName CONFIGURATION_CLASSNAME =
            ClassName.get("com.android.bedstead.remotedpc", "Configuration");
    private static final ClassName CROSS_PROFILE_CONNECTOR_CLASSNAME =
            ClassName.get("com.google.android.enterprise.connectedapps", "CrossProfileConnector");
    private static final ClassName UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME =
            ClassName.get(
                    "com.google.android.enterprise.connectedapps.exceptions",
                    "UnavailableProfileException");
    private static final ClassName PROFILE_RUNTIME_EXCEPTION_CLASSNAME =
            ClassName.get(
                    "com.google.android.enterprise.connectedapps.exceptions",
                    "ProfileRuntimeException");
    private static final ClassName NENE_EXCEPTION_CLASSNAME =
            ClassName.get(
                    "com.android.bedstead.nene.exceptions", "NeneException");
    public static final String MANAGER_PACKAGE_NAME = "com.android.bedstead.remotedpc.managers";

    @Override
    public SourceVersion getSupportedSourceVersion() {
        return SourceVersion.latest();
    }

    @Override
    public boolean process(Set<? extends TypeElement> annotations,
            RoundEnvironment roundEnv) {

        Set<TypeElement> interfaces = new HashSet<>();

        for (Element e : roundEnv.getElementsAnnotatedWith(RemoteDpcManager.class)) {
            TypeElement interfaceClass = (TypeElement) e;
            interfaces.add(interfaceClass);
            processRemoteDpcManagerInterface(interfaceClass);
        }

        if (interfaces.isEmpty()) {
            // We only want to generate the provider and configuration once, not on every iteration
            return true;
        }

        generateProvider(interfaces);
        generateConfiguration();

        return true;
    }

    private void processRemoteDpcManagerInterface(TypeElement interfaceClass) {
        RemoteDpcManager r = interfaceClass.getAnnotation(RemoteDpcManager.class);
        TypeElement managerClass = extractClassFromAnnotation(r::managerClass);

        if (!interfaceClass.getKind().isInterface()) {
            showError("@RemoteDpcManager can only be applied to interfaces", interfaceClass);
        }

        generateCrossProfileInterface(interfaceClass);
        generateImplClass(interfaceClass, managerClass);
        generateWrapperClass(interfaceClass);
    }

    /** Generate Impl which wraps the manager class. */
    private void generateImplClass(TypeElement interfaceClass, TypeElement managerClass) {
        ClassName managerClassName = ClassName.get(managerClass);

        TypeSpec.Builder classBuilder = TypeSpec.classBuilder(implName(interfaceClass))
                .addSuperinterface(crossProfileInterfaceName(interfaceClass))
                .addModifiers(Modifier.PUBLIC, Modifier.FINAL);

        classBuilder.addAnnotation(AnnotationSpec.builder(SuppressWarnings.class)
                .addMember("value", "{\"NewApi\", \"OldTargetApi\"}")
                .build());

        classBuilder.addField(managerClassName,
                "mManager", Modifier.PRIVATE, Modifier.FINAL);

        classBuilder.addMethod(
                MethodSpec.constructorBuilder()
                        .addParameter(CONTEXT_CLASSNAME, "context")
                        .addCode("mManager = context.getSystemService($T.class);",
                                managerClassName)
                .build()
        );

        for (ExecutableElement method : getMethods(interfaceClass)) {
            MethodSpec.Builder methodBuilder = MethodSpec.methodBuilder(
                    method.getSimpleName().toString())
                    .returns(ClassName.get(method.getReturnType()))
                    .addModifiers(Modifier.PUBLIC)
                    .addAnnotation(Override.class);

            for (VariableElement param : method.getParameters()) {
                ParameterSpec parameterSpec =
                        ParameterSpec.builder(ClassName.get(param.asType()),
                                param.getSimpleName().toString()).build();

                methodBuilder.addParameter(parameterSpec);
            }

            String parametersString = method.getParameters().stream()
                    .map(VariableElement::getSimpleName)
                    .collect(Collectors.joining(", "));
            CodeBlock methodCall;

            if (method.getAnnotation(RemoteDpcAutomaticAdmin.class) != null) {
                // We just redirect to the other method, adding in the component
                if (parametersString.isEmpty()) {
                    methodCall = CodeBlock.of("$L($T.REMOTE_DPC_COMPONENT_NAME);",
                            method.getSimpleName(), CONFIGURATION_CLASSNAME);
                } else {
                    methodCall = CodeBlock.of("$L($T.REMOTE_DPC_COMPONENT_NAME, $L);",
                            method.getSimpleName(), CONFIGURATION_CLASSNAME, parametersString);
                }
            } else {
                // We call through to the wrapped manager class
                methodCall = CodeBlock.of("mManager.$L($L);",
                        method.getSimpleName(), parametersString);
            }

            if (!method.getReturnType().getKind().equals(TypeKind.VOID)) {
                methodCall = CodeBlock.of("return $L", methodCall);
            }
            methodBuilder.addCode(methodCall);

            classBuilder.addMethod(methodBuilder.build());
        }

        PackageElement packageElement = (PackageElement) interfaceClass.getEnclosingElement();

        writeClassToFile(packageElement.getQualifiedName().toString(), classBuilder.build());
    }

    /** Generate wrapper which wraps the cross-profile class. */
    private void generateWrapperClass(TypeElement interfaceClass) {
        TypeSpec.Builder classBuilder =
                TypeSpec.classBuilder(
                        wrapperName(interfaceClass))
                        .addSuperinterface(crossProfileInterfaceName(interfaceClass))
                        .addModifiers(Modifier.PUBLIC, Modifier.FINAL);


        classBuilder.addField(CROSS_PROFILE_CONNECTOR_CLASSNAME,
                "mConnector", Modifier.PRIVATE, Modifier.FINAL);
        classBuilder.addField(profileTypeName(interfaceClass),
                "mProfileType", Modifier.PRIVATE, Modifier.FINAL);

        classBuilder.addMethod(
                MethodSpec.constructorBuilder()
                        .addModifiers(Modifier.PUBLIC)
                        .addParameter(CROSS_PROFILE_CONNECTOR_CLASSNAME, "connector")
                        .addCode("mConnector = connector;")
                        .addCode("mProfileType = $T.create(connector);",
                                profileTypeName(interfaceClass))
                        .build()
        );

        classBuilder.addMethod(
                MethodSpec.methodBuilder("tryConnect")
                        .addModifiers(Modifier.PRIVATE)
                        .addException(UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME)
                        .addCode("$T retries = 300;", int.class)
                        .beginControlFlow("while (true)")
                            .beginControlFlow("try")
                                .addCode("mConnector.connect();")
                                .addCode("return;")
                            .nextControlFlow("catch ($T e)", UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME)
                                .addCode("retries -= 1;")
                                .beginControlFlow("if (retries <= 0)")
                                    .addCode("throw e;")
                                .endControlFlow()
                                .beginControlFlow("try")
                                    .addCode("$T.sleep(100);", Thread.class)
                                 .nextControlFlow("catch ($T e2)", InterruptedException.class)
                                .endControlFlow()
                            .endControlFlow()
                        .endControlFlow()
                        .build()
        );

        for (ExecutableElement method : getMethods(interfaceClass)) {
            MethodSpec.Builder methodBuilder =
                    MethodSpec.methodBuilder(method.getSimpleName().toString())
                    .returns(ClassName.get(method.getReturnType()))
                    .addModifiers(Modifier.PUBLIC)
                    .addAnnotation(Override.class);

            for (VariableElement param : method.getParameters()) {
                ParameterSpec parameterSpec =
                        ParameterSpec.builder(ClassName.get(param.asType()),
                                param.getSimpleName().toString()).build();

                methodBuilder.addParameter(parameterSpec);
            }

            String parametersString = method.getParameters().stream()
                    .map(VariableElement::getSimpleName)
                    .collect(Collectors.joining(", "));

            CodeBlock methodCall = CodeBlock.of("mProfileType.other().$L($L);",
                    method.getSimpleName().toString(), parametersString);
            if (!method.getReturnType().getKind().equals(TypeKind.VOID)) {
                methodCall = CodeBlock.of("return $L", methodCall);
            }

            methodBuilder.beginControlFlow("try")
                    .addCode("tryConnect();")
                    .addCode(methodCall)
                    .nextControlFlow("catch ($T e)",
                            UNAVAILABLE_PROFILE_EXCEPTION_CLASSNAME)
                    .addCode("throw new $T(\"Error connecting\", e);", NENE_EXCEPTION_CLASSNAME)
                    .nextControlFlow("catch ($T e)",
                            PROFILE_RUNTIME_EXCEPTION_CLASSNAME)
                    .addCode("throw ($T) e.getCause();", RuntimeException.class)
                    .nextControlFlow("finally")
                    .addCode("mConnector.stopManualConnectionManagement();")
                    .endControlFlow();

            classBuilder.addMethod(methodBuilder.build());
        }

        PackageElement packageElement = (PackageElement) interfaceClass.getEnclosingElement();
        writeClassToFile(packageElement.getQualifiedName().toString(), classBuilder.build());
    }

    /** Generate sub-interface which is annotated @CrossProfile. */
    private void generateCrossProfileInterface(TypeElement interfaceClass) {
        TypeSpec.Builder classBuilder =
                TypeSpec.interfaceBuilder(
                        crossProfileInterfaceName(interfaceClass))
                        .addSuperinterface(ClassName.get(interfaceClass))
                        .addModifiers(Modifier.PUBLIC);

        for (ExecutableElement method : getMethods(interfaceClass)) {
            MethodSpec.Builder methodBuilder =
                    MethodSpec.methodBuilder(method.getSimpleName().toString())
                    .returns(ClassName.get(method.getReturnType()))
                    .addModifiers(Modifier.PUBLIC, Modifier.ABSTRACT)
                    .addAnnotation(CrossProfile.class)
                    .addAnnotation(Override.class);

            for (VariableElement param : method.getParameters()) {
                ParameterSpec parameterSpec =
                        ParameterSpec.builder(ClassName.get(param.asType()),
                                param.getSimpleName().toString()).build();

                methodBuilder.addParameter(parameterSpec);
            }

            classBuilder.addMethod(methodBuilder.build());
        }

        PackageElement packageElement = (PackageElement) interfaceClass.getEnclosingElement();
        writeClassToFile(packageElement.getQualifiedName().toString(), classBuilder.build());
    }

    private void generateProvider(Set<TypeElement> interfaces) {
        TypeSpec.Builder classBuilder =
                TypeSpec.classBuilder(
                        "Provider")
                        .addModifiers(Modifier.PUBLIC, Modifier.FINAL);

        for (TypeElement i : interfaces) {
            MethodSpec.Builder methodBuilder =
                    MethodSpec.methodBuilder("provide_" + i.getSimpleName())
                    .returns(crossProfileInterfaceName(i))
                    .addModifiers(Modifier.PUBLIC)
                    .addParameter(CONTEXT_CLASSNAME, "context")
                    .addAnnotation(CrossProfileProvider.class)
                    .addCode("return new $T(context);", implName(i));

            classBuilder.addMethod(methodBuilder.build());
        }

        writeClassToFile(MANAGER_PACKAGE_NAME, classBuilder.build());
    }

    private void generateConfiguration() {
        TypeSpec.Builder classBuilder =
                TypeSpec.classBuilder(
                        "Configuration")
                        .addModifiers(Modifier.PUBLIC, Modifier.FINAL)
                .addAnnotation(AnnotationSpec.builder(CrossProfileConfiguration.class)
                        .addMember("providers", "Provider.class")
                        .build());

        writeClassToFile(MANAGER_PACKAGE_NAME, classBuilder.build());
    }

    private TypeElement extractClassFromAnnotation(Runnable runnable) {
        try {
            runnable.run();
        } catch (MirroredTypeException e) {
            return e.getTypeMirrors().stream()
                    .map(t -> (TypeElement) processingEnv.getTypeUtils().asElement(t))
                    .findFirst()
                    .get();
        }
        throw new AssertionError("Could not extract class from annotation");
    }

    private void writeClassToFile(String packageName, TypeSpec clazz) {
        String qualifiedClassName =
                packageName.isEmpty() ? clazz.name : packageName + "." + clazz.name;

        JavaFile javaFile = JavaFile.builder(packageName, clazz).build();
        try {
            JavaFileObject builderFile =
                    processingEnv.getFiler().createSourceFile(qualifiedClassName);
            try (PrintWriter out = new PrintWriter(builderFile.openWriter())) {
                javaFile.writeTo(out);
            }
        } catch (IOException e) {
            throw new IllegalStateException("Error writing " + qualifiedClassName + " to file", e);
        }
    }

    private ClassName crossProfileInterfaceName(TypeElement interfaceClass) {
        return ClassName.bestGuess(interfaceClass.getQualifiedName().toString() + "_Internal");
    }

    private ClassName implName(TypeElement interfaceClass) {
        return ClassName.bestGuess(interfaceClass.getQualifiedName().toString() + "_Impl");
    }

    private ClassName wrapperName(TypeElement interfaceClass) {
        return ClassName.bestGuess(interfaceClass.getQualifiedName().toString() + "_Wrapper");
    }

    private ClassName profileTypeName(TypeElement interfaceClass) {
        ClassName crossProfileInterfaceName = crossProfileInterfaceName(interfaceClass);
        return ClassName.get(crossProfileInterfaceName.packageName(),
                "Profile" + crossProfileInterfaceName.simpleName());
    }

    private Set<ExecutableElement> getMethods(TypeElement interfaceClass) {
        return interfaceClass.getEnclosedElements().stream()
                .filter(e -> e instanceof ExecutableElement)
                .map(e -> (ExecutableElement) e)
                .collect(Collectors.toSet());
    }

    private void showError(String errorText, Element errorElement) {
        processingEnv
                .getMessager()
                .printMessage(Diagnostic.Kind.ERROR, errorText, errorElement);
    }
}
