/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.tools.metalava.stub

import com.android.tools.metalava.model.AnnotationTarget
import com.android.tools.metalava.model.ClassItem
import com.android.tools.metalava.model.Item
import com.android.tools.metalava.model.Language
import com.android.tools.metalava.model.MemberItem
import com.android.tools.metalava.model.MethodItem
import com.android.tools.metalava.model.ModifierList
import com.android.tools.metalava.model.PackageItem
import com.android.tools.metalava.model.TypeItem
import com.android.tools.metalava.model.TypeParameterList
import com.android.tools.metalava.model.psi.PsiClassItem
import com.android.tools.metalava.model.visitors.ItemVisitor
import java.io.PrintWriter
import java.util.function.Predicate

class KotlinStubWriter(
    private val writer: PrintWriter,
    private val filterEmit: Predicate<Item>,
    private val filterReference: Predicate<Item>,
    private val generateAnnotations: Boolean = false,
    private val preFiltered: Boolean = true,
    private val docStubs: Boolean
) : ItemVisitor() {
    private val annotationTarget = if (docStubs) AnnotationTarget.DOC_STUBS_FILE else AnnotationTarget.SDK_STUBS_FILE

    override fun visitClass(cls: ClassItem) {
        if (cls.isTopLevelClass()) {
            val qualifiedName = cls.containingPackage().qualifiedName()
            if (qualifiedName.isNotBlank()) {
                writer.println("package $qualifiedName")
                writer.println()
            }
            val compilationUnit = cls.getCompilationUnit()
            compilationUnit?.getImportStatements(filterReference)?.let {
                for (item in it) {
                    when (item) {
                        is PackageItem ->
                            writer.println("import ${item.qualifiedName()}.*")
                        is ClassItem ->
                            writer.println("import ${item.qualifiedName()}")
                        is MemberItem ->
                            writer.println("import static ${item.containingClass().qualifiedName()}.${item.name()}")
                    }
                }
                writer.println()
            }
        }
        appendDocumentation(cls, writer, docStubs)

        writer.println("@file:Suppress(\"ALL\")")

        // Need to filter out abstract from the modifiers list and turn it
        // into a concrete method to make the stub compile
        val removeAbstract = cls.modifiers.isAbstract() && (cls.isEnum() || cls.isAnnotationType())

        appendModifiers(cls, cls.modifiers, removeAbstract)

        when {
            cls.isAnnotationType() -> writer.print("annotation class")
            cls.isInterface() -> writer.print("interface")
            cls.isEnum() -> writer.print("enum class")
            else -> writer.print("class")
        }

        writer.print(" ")
        writer.print(cls.simpleName())

        generateTypeParameterList(typeList = cls.typeParameterList(), addSpace = false)
        val printedSuperClass = generateSuperClassDeclaration(cls)
        generateInterfaceList(cls, printedSuperClass)
        writer.print(" {\n")
    }

    private fun generateTypeParameterList(
        typeList: TypeParameterList,
        addSpace: Boolean
    ) {
        val typeListString = typeList.toString()
        if (typeListString.isNotEmpty()) {
            writer.print(typeListString)

            if (addSpace) {
                writer.print(' ')
            }
        }
    }

    private fun appendModifiers(
        item: Item,
        modifiers: ModifierList,
        removeAbstract: Boolean,
        removeFinal: Boolean = false,
        addPublic: Boolean = false
    ) {
        val separateLines = item is ClassItem || item is MethodItem

        ModifierList.write(
            writer, modifiers, item,
            target = annotationTarget,
            includeAnnotations = true,
            skipNullnessAnnotations = true,
            includeDeprecated = true,
            runtimeAnnotationsOnly = !generateAnnotations,
            removeAbstract = removeAbstract,
            removeFinal = removeFinal,
            addPublic = addPublic,
            separateLines = separateLines,
            language = Language.KOTLIN
        )
    }

    private fun generateSuperClassDeclaration(cls: ClassItem): Boolean {
        if (cls.isEnum() || cls.isAnnotationType()) {
            // No extends statement for enums and annotations; it's implied by the "enum" and "@interface" keywords
            return false
        }

        val superClass = if (preFiltered)
            cls.superClassType()
        else cls.filteredSuperClassType(filterReference)

        if (superClass != null && !superClass.isJavaLangObject()) {
            val qualifiedName = superClass.toTypeString() // TODO start passing language = Language.KOTLIN
            writer.print(" : ")

            if (qualifiedName.contains("<")) {
                // TODO: push this into the model at filter-time such that clients don't need
                // to remember to do this!!
                val s = superClass.asClass()
                if (s != null) {
                    val map = cls.mapTypeVariables(s)
                    val replaced = superClass.convertTypeString(map)
                    writer.print(replaced)
                    return true
                }
            }
            (cls as PsiClassItem).psiClass.superClassType
            writer.print(qualifiedName)
            // TODO: print out arguments to the parent constructor
            writer.print("()")
            return true
        }
        return false
    }

    private fun generateInterfaceList(cls: ClassItem, printedSuperClass: Boolean) {
        if (cls.isAnnotationType()) {
            // No extends statement for annotations; it's implied by the "@interface" keyword
            return
        }

        val interfaces = if (preFiltered)
            cls.interfaceTypes().asSequence()
        else cls.filteredInterfaceTypes(filterReference).asSequence()

        if (interfaces.any()) {
            if (printedSuperClass) {
                writer.print(",")
            } else {
                writer.print(" :")
            }
            interfaces.forEachIndexed { index, type ->
                if (index > 0) {
                    writer.print(",")
                }
                writer.print(" ")
                writer.print(type.toTypeString()) // TODO start passing language = Language.KOTLIN
            }
        }
    }

    private fun writeType(
        item: Item,
        type: TypeItem?
    ) {
        type ?: return

        val typeString = type.toTypeString(
            outerAnnotations = false,
            innerAnnotations = generateAnnotations,
            erased = false,
            kotlinStyleNulls = true,
            context = item,
            filter = filterReference
            // TODO pass in language = Language.KOTLIN
        )

        writer.print(typeString)
    }

    override fun visitMethod(method: MethodItem) {
        if (method.isKotlinProperty()) return // will be handled by visitProperty
        val containingClass = method.containingClass()
        val modifiers = method.modifiers
        val isEnum = containingClass.isEnum()
        val isAnnotation = containingClass.isAnnotationType()

        writer.println()
        appendDocumentation(method, writer, docStubs)

        // TODO: Should be an annotation
        generateThrowsList(method)

        // Need to filter out abstract from the modifiers list and turn it
        // into a concrete method to make the stub compile
        val removeAbstract = modifiers.isAbstract() && (isEnum || isAnnotation)

        appendModifiers(method, modifiers, removeAbstract, false)
        generateTypeParameterList(typeList = method.typeParameterList(), addSpace = true)

        writer.print("fun ")
        writer.print(method.name())
        generateParameterList(method)

        writer.print(": ")
        val returnType = method.returnType()
        writeType(method, returnType)

        if (isAnnotation) {
            val default = method.defaultValue()
            if (default.isNotEmpty()) {
                writer.print(" default ")
                writer.print(default)
            }
        }

        if (modifiers.isAbstract() && !isEnum || isAnnotation || modifiers.isNative()) {
            // do nothing
        } else {
            writer.print(" = ")
            writeThrowStub()
        }
        writer.println()
    }

    override fun afterVisitClass(cls: ClassItem) {
        writer.println("}\n\n")
    }

    private fun writeThrowStub() {
        writer.write("error(\"Stub!\")")
    }

    private fun generateParameterList(method: MethodItem) {
        writer.print("(")
        method.parameters().asSequence().forEachIndexed { i, parameter ->
            if (i > 0) {
                writer.print(", ")
            }
            appendModifiers(
                parameter,
                parameter.modifiers,
                removeAbstract = false,
                removeFinal = false,
                addPublic = false
            )
            val name = parameter.publicName() ?: parameter.name()
            writer.print(name)
            writer.print(": ")
            writeType(method, parameter.type())
        }
        writer.print(")")
    }

    private fun generateThrowsList(method: MethodItem) {
        // Note that throws types are already sorted internally to help comparison matching
        val throws = if (preFiltered) {
            method.throwsTypes().asSequence()
        } else {
            method.filteredThrowsTypes(filterReference).asSequence()
        }
        if (throws.any()) {
            writer.print("@Throws(")
            throws.asSequence().sortedWith(ClassItem.fullNameComparator).forEachIndexed { i, type ->
                if (i > 0) {
                    writer.print(",")
                }
                writer.print(type.qualifiedName())
                writer.print("::class")
            }
            writer.print(")")
        }
    }
}