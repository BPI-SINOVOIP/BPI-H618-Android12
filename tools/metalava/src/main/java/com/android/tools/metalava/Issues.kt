/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.tools.metalava

import com.android.sdklib.SdkVersionInfo
import java.util.ArrayList
import java.util.HashMap
import java.util.Locale
import kotlin.reflect.full.declaredMemberProperties

object Issues {
    private val allIssues: MutableList<Issue> = ArrayList(300)
    private val nameToIssue: MutableMap<String, Issue> = HashMap(300)

    val PARSE_ERROR = Issue(Severity.ERROR)
    // Compatibility issues
    val ADDED_PACKAGE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_CLASS = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_METHOD = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_FIELD = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_INTERFACE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_PACKAGE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_CLASS = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_METHOD = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_FIELD = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_INTERFACE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_STATIC = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_FINAL = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_TRANSIENT = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_VOLATILE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_TYPE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_VALUE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_SUPERCLASS = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_SCOPE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_ABSTRACT = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_THROWS = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_NATIVE = Issue(Severity.HIDDEN, Category.COMPATIBILITY)
    val CHANGED_CLASS = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_DEPRECATED = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val CHANGED_SYNCHRONIZED = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val ADDED_FINAL_UNINSTANTIABLE = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_FINAL = Issue(Severity.WARNING, Category.COMPATIBILITY)
    val REMOVED_DEPRECATED_CLASS = Issue(REMOVED_CLASS, Category.COMPATIBILITY)
    val REMOVED_DEPRECATED_METHOD = Issue(REMOVED_METHOD, Category.COMPATIBILITY)
    val REMOVED_DEPRECATED_FIELD = Issue(REMOVED_FIELD, Category.COMPATIBILITY)
    val ADDED_ABSTRACT_METHOD = Issue(ADDED_METHOD, Category.COMPATIBILITY)
    val ADDED_REIFIED = Issue(Severity.WARNING, Category.COMPATIBILITY)

    // Issues in javadoc generation
    val UNRESOLVED_LINK = Issue(Severity.LINT, Category.DOCUMENTATION)
    val BAD_INCLUDE_TAG = Issue(Severity.LINT, Category.DOCUMENTATION)
    val UNKNOWN_TAG = Issue(Severity.LINT, Category.DOCUMENTATION)
    val UNKNOWN_PARAM_TAG_NAME = Issue(Severity.LINT, Category.DOCUMENTATION)
    val UNDOCUMENTED_PARAMETER = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val BAD_ATTR_TAG = Issue(Severity.LINT, Category.DOCUMENTATION)
    val BAD_INHERITDOC = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val HIDDEN_LINK = Issue(Severity.LINT, Category.DOCUMENTATION)
    val HIDDEN_CONSTRUCTOR = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val UNAVAILABLE_SYMBOL = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val HIDDEN_SUPERCLASS = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val DEPRECATED = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val DEPRECATION_MISMATCH = Issue(Severity.ERROR, Category.DOCUMENTATION)
    val MISSING_COMMENT = Issue(Severity.LINT, Category.DOCUMENTATION)
    val IO_ERROR = Issue(Severity.ERROR)
    val NO_SINCE_DATA = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val NO_FEDERATION_DATA = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val BROKEN_SINCE_FILE = Issue(Severity.ERROR, Category.DOCUMENTATION)
    val INVALID_CONTENT_TYPE = Issue(Severity.ERROR, Category.DOCUMENTATION)
    val INVALID_SAMPLE_INDEX = Issue(Severity.ERROR, Category.DOCUMENTATION)
    val HIDDEN_TYPE_PARAMETER = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val PRIVATE_SUPERCLASS = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val NULLABLE = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val INT_DEF = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val REQUIRES_PERMISSION = Issue(Severity.LINT, Category.DOCUMENTATION)
    val BROADCAST_BEHAVIOR = Issue(Severity.LINT, Category.DOCUMENTATION)
    val SDK_CONSTANT = Issue(Severity.LINT, Category.DOCUMENTATION)
    val TODO = Issue(Severity.LINT, Category.DOCUMENTATION)
    val NO_ARTIFACT_DATA = Issue(Severity.HIDDEN, Category.DOCUMENTATION)
    val BROKEN_ARTIFACT_FILE = Issue(Severity.ERROR, Category.DOCUMENTATION)

    // Metalava warnings (not from doclava)

    val TYPO = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val MISSING_PERMISSION = Issue(Severity.LINT, Category.DOCUMENTATION)
    val MULTIPLE_THREAD_ANNOTATIONS = Issue(Severity.LINT, Category.DOCUMENTATION)
    val UNRESOLVED_CLASS = Issue(Severity.LINT, Category.DOCUMENTATION)
    val INVALID_NULL_CONVERSION = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val PARAMETER_NAME_CHANGE = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val OPERATOR_REMOVAL = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val INFIX_REMOVAL = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val VARARG_REMOVAL = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val ADD_SEALED = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val FUN_REMOVAL = Issue(Severity.ERROR, Category.COMPATIBILITY)
    val ANNOTATION_EXTRACTION = Issue(Severity.ERROR)
    val SUPERFLUOUS_PREFIX = Issue(Severity.WARNING)
    val HIDDEN_TYPEDEF_CONSTANT = Issue(Severity.ERROR)
    val EXPECTED_PLATFORM_TYPE = Issue(Severity.HIDDEN)
    val INTERNAL_ERROR = Issue(Severity.ERROR)
    val RETURNING_UNEXPECTED_CONSTANT = Issue(Severity.WARNING)
    val DEPRECATED_OPTION = Issue(Severity.WARNING)
    val BOTH_PACKAGE_INFO_AND_HTML = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val UNMATCHED_MERGE_ANNOTATION = Issue(Severity.WARNING)
    // The plan is for this to be set as an error once (1) existing code is marked as @deprecated
    // and (2) the principle is adopted by the API council
    val REFERENCES_DEPRECATED = Issue(Severity.HIDDEN)
    val UNHIDDEN_SYSTEM_API = Issue(Severity.ERROR)
    val SHOWING_MEMBER_IN_HIDDEN_CLASS = Issue(Severity.ERROR)
    val INVALID_NULLABILITY_ANNOTATION = Issue(Severity.ERROR)
    val REFERENCES_HIDDEN = Issue(Severity.ERROR)
    val IGNORING_SYMLINK = Issue(Severity.INFO)
    val INVALID_NULLABILITY_ANNOTATION_WARNING = Issue(Severity.WARNING)
    // The plan is for this to be set as an error once (1) existing code is marked as @deprecated
    // and (2) the principle is adopted by the API council
    val EXTENDS_DEPRECATED = Issue(Severity.HIDDEN)
    val FORBIDDEN_TAG = Issue(Severity.ERROR)
    val MISSING_COLUMN = Issue(Severity.WARNING, Category.DOCUMENTATION)
    val INVALID_SYNTAX = Issue(Severity.ERROR)
    val UNRESOLVED_IMPORT = Issue(Severity.INFO)
    val HIDDEN_ABSTRACT_METHOD = Issue(Severity.ERROR)

    // API lint
    val START_WITH_LOWER = Issue(Severity.ERROR, Category.API_LINT, "style-conventions")
    val START_WITH_UPPER = Issue(Severity.ERROR, Category.API_LINT, "style-conventions")
    val ALL_UPPER = Issue(Severity.ERROR, Category.API_LINT, "constant-naming")
    val ACRONYM_NAME = Issue(Severity.WARNING, Category.API_LINT, "acronyms-in-method-name")
    val ENUM = Issue(Severity.ERROR, Category.API_LINT, "avoid-enum")
    val ENDS_WITH_IMPL = Issue(Severity.ERROR, Category.API_LINT, "dont-end-with-impl")
    val MIN_MAX_CONSTANT = Issue(Severity.WARNING, Category.API_LINT, "min-max-constants")
    val COMPILE_TIME_CONSTANT = Issue(Severity.ERROR, Category.API_LINT)
    val SINGULAR_CALLBACK = Issue(Severity.ERROR, Category.API_LINT, "callback-class-singular")
    val CALLBACK_NAME = Issue(Severity.WARNING, Category.API_LINT, "observer-should-be-callback")
    val CALLBACK_INTERFACE =
        Issue(Severity.ERROR, Category.API_LINT, "callback-abstract-instead-of-interface")
    val CALLBACK_METHOD_NAME = Issue(Severity.ERROR, Category.API_LINT, "callback-method-naming")
    val LISTENER_INTERFACE = Issue(Severity.ERROR, Category.API_LINT, "callbacks-listener")
    val SINGLE_METHOD_INTERFACE = Issue(Severity.ERROR, Category.API_LINT, "callbacks-listener")
    val INTENT_NAME =
        Issue(Severity.ERROR, Category.API_LINT, "use-standard-prefixes-for-constants")
    val ACTION_VALUE = Issue(Severity.ERROR, Category.API_LINT)
    val EQUALS_AND_HASH_CODE = Issue(Severity.ERROR, Category.API_LINT, "equals-and-hashcode")
    val PARCEL_CREATOR = Issue(Severity.ERROR, Category.API_LINT, "parcelable-creator")
    val PARCEL_NOT_FINAL = Issue(Severity.ERROR, Category.API_LINT, "parcelable-final")
    val PARCEL_CONSTRUCTOR = Issue(Severity.ERROR, Category.API_LINT, "parcelable-creator")
    val PROTECTED_MEMBER = Issue(Severity.ERROR, Category.API_LINT, "avoid-protected")
    val PAIRED_REGISTRATION = Issue(Severity.ERROR, Category.API_LINT, "callbacks-symmetry")
    val REGISTRATION_NAME = Issue(Severity.ERROR, Category.API_LINT, "callbacks-accessors")
    val VISIBLY_SYNCHRONIZED = Issue(Severity.ERROR, Category.API_LINT, "avoid-synchronized")
    val INTENT_BUILDER_NAME =
        Issue(Severity.WARNING, Category.API_LINT, "intent-builder-createintent")
    val CONTEXT_NAME_SUFFIX = Issue(Severity.ERROR, Category.API_LINT, "classes-subclass-naming")
    val INTERFACE_CONSTANT = Issue(Severity.ERROR, Category.API_LINT, "services-intents")
    val ON_NAME_EXPECTED = Issue(Severity.WARNING, Category.API_LINT, "callback-method-naming")
    val TOP_LEVEL_BUILDER = Issue(Severity.WARNING, Category.API_LINT, "builders-static-inner")
    val MISSING_BUILD_METHOD =
        Issue(Severity.WARNING, Category.API_LINT, "builder-must-declare-build")
    val BUILDER_SET_STYLE = Issue(Severity.WARNING, Category.API_LINT, "builder-method-naming")
    val SETTER_RETURNS_THIS = Issue(Severity.WARNING, Category.API_LINT)
    val RAW_AIDL = Issue(Severity.ERROR, Category.API_LINT, "no-public-binder")
    val INTERNAL_CLASSES = Issue(Severity.ERROR, Category.API_LINT)
    val PACKAGE_LAYERING = Issue(Severity.WARNING, Category.API_LINT)
    val GETTER_SETTER_NAMES = Issue(Severity.ERROR, Category.API_LINT)
    val CONCRETE_COLLECTION = Issue(Severity.ERROR, Category.API_LINT, "classes-collections")
    val OVERLAPPING_CONSTANTS = Issue(Severity.WARNING, Category.API_LINT, "overlapping-constants")
    val GENERIC_EXCEPTION = Issue(Severity.ERROR, Category.API_LINT, "appropriate-exception")
    val ILLEGAL_STATE_EXCEPTION =
        Issue(Severity.WARNING, Category.API_LINT, "appropriate-exception")
    val RETHROW_REMOTE_EXCEPTION = Issue(Severity.ERROR, Category.API_LINT, "appropriate-exception")
    val MENTIONS_GOOGLE = Issue(Severity.ERROR, Category.API_LINT, "mentions-google")
    val HEAVY_BIT_SET = Issue(Severity.ERROR, Category.API_LINT, "avoid-bitset")
    val MANAGER_CONSTRUCTOR = Issue(Severity.ERROR, Category.API_LINT)
    val MANAGER_LOOKUP = Issue(Severity.ERROR, Category.API_LINT)
    val AUTO_BOXING = Issue(Severity.ERROR, Category.API_LINT, "auto-boxing")
    val STATIC_UTILS = Issue(Severity.ERROR, Category.API_LINT)
    val CONTEXT_FIRST = Issue(Severity.ERROR, Category.API_LINT)
    val LISTENER_LAST = Issue(Severity.WARNING, Category.API_LINT, "placement-of-sam-parameters")
    val EXECUTOR_REGISTRATION = Issue(Severity.WARNING, Category.API_LINT, "callbacks-listener")
    val CONFIG_FIELD_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val RESOURCE_FIELD_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val RESOURCE_VALUE_FIELD_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val RESOURCE_STYLE_FIELD_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val STREAM_FILES = Issue(Severity.WARNING, Category.API_LINT)
    val PARCELABLE_LIST = Issue(Severity.WARNING, Category.API_LINT)
    val ABSTRACT_INNER = Issue(Severity.WARNING, Category.API_LINT)
    val BANNED_THROW = Issue(Severity.ERROR, Category.API_LINT)
    val EXTENDS_ERROR = Issue(Severity.ERROR, Category.API_LINT)
    val EXCEPTION_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val METHOD_NAME_UNITS = Issue(Severity.ERROR, Category.API_LINT, "unit-names")
    val FRACTION_FLOAT = Issue(Severity.ERROR, Category.API_LINT)
    val PERCENTAGE_INT = Issue(Severity.ERROR, Category.API_LINT)
    val NOT_CLOSEABLE = Issue(Severity.WARNING, Category.API_LINT)
    val KOTLIN_OPERATOR = Issue(Severity.INFO, Category.API_LINT)
    val ARRAY_RETURN =
        Issue(Severity.WARNING, Category.API_LINT, "methods-prefer-collection-over-array")
    val USER_HANDLE = Issue(Severity.WARNING, Category.API_LINT)
    val USER_HANDLE_NAME = Issue(Severity.WARNING, Category.API_LINT)
    val SERVICE_NAME = Issue(Severity.ERROR, Category.API_LINT)
    val METHOD_NAME_TENSE = Issue(Severity.WARNING, Category.API_LINT)
    val NO_CLONE = Issue(Severity.ERROR, Category.API_LINT, "avoid-clone")
    val USE_ICU = Issue(Severity.WARNING, Category.API_LINT)
    val USE_PARCEL_FILE_DESCRIPTOR =
        Issue(Severity.ERROR, Category.API_LINT, "prefer-parcelfiledescriptor")
    val NO_BYTE_OR_SHORT = Issue(Severity.WARNING, Category.API_LINT, "avoid-short-byte")
    val SINGLETON_CONSTRUCTOR = Issue(Severity.ERROR, Category.API_LINT, "singleton-class")
    val COMMON_ARGS_FIRST = Issue(Severity.WARNING, Category.API_LINT, "optional-params-last")
    val CONSISTENT_ARGUMENT_ORDER = Issue(Severity.ERROR, Category.API_LINT, "optional-params-last")
    val KOTLIN_KEYWORD = Issue(Severity.ERROR, Category.API_LINT)
    val UNIQUE_KOTLIN_OPERATOR = Issue(Severity.ERROR, Category.API_LINT)
    val SAM_SHOULD_BE_LAST =
        Issue(Severity.WARNING, Category.API_LINT, "placement-of-sam-parameters")
    val MISSING_JVMSTATIC = Issue(Severity.WARNING, Category.API_LINT)
    val DEFAULT_VALUE_CHANGE = Issue(Severity.ERROR, Category.API_LINT, "default-value-removal")
    val DOCUMENT_EXCEPTIONS = Issue(Severity.ERROR, Category.API_LINT, "docs-throws")
    val FORBIDDEN_SUPER_CLASS = Issue(Severity.ERROR, Category.API_LINT)
    val MISSING_NULLABILITY = Issue(Severity.ERROR, Category.API_LINT, "annotations")
    val MUTABLE_BARE_FIELD = Issue(Severity.ERROR, Category.API_LINT, "mutable-bare-field")
    val INTERNAL_FIELD = Issue(Severity.ERROR, Category.API_LINT, "internal-fields")
    val PUBLIC_TYPEDEF = Issue(Severity.ERROR, Category.API_LINT, "no-public-typedefs")
    val ANDROID_URI = Issue(Severity.ERROR, Category.API_LINT, "android-uri")
    val BAD_FUTURE = Issue(Severity.ERROR, Category.API_LINT, "bad-future")
    val STATIC_FINAL_BUILDER = Issue(Severity.WARNING, Category.API_LINT, "builders-static-inner")
    val GETTER_ON_BUILDER = Issue(Severity.WARNING, Category.API_LINT, "getter-on-builder")
    val MISSING_GETTER_MATCHING_BUILDER =
        Issue(Severity.WARNING, Category.API_LINT, "builders-symmetric-setters")
    val OPTIONAL_BUILDER_CONSTRUCTOR_ARGUMENT =
        Issue(Severity.WARNING, Category.API_LINT, "builders-nonnull-constructors")
    val NO_SETTINGS_PROVIDER = Issue(Severity.HIDDEN, Category.API_LINT, "no-settings-provider")
    val NULLABLE_COLLECTION = Issue(Severity.WARNING, Category.API_LINT, "methods-prefer-non-null-collections")

    fun findIssueById(id: String?): Issue? {
        return nameToIssue[id]
    }

    fun findIssueByIdIgnoringCase(id: String): Issue? {
        for (e in allIssues) {
            if (id.equals(e.name, ignoreCase = true)) {
                return e
            }
        }
        return null
    }

    class Issue private constructor(
        val defaultLevel: Severity,
        /**
         * When `level` is set to [Severity.INHERIT], this is the parent from
         * which the issue will inherit its level.
         */
        val parent: Issue?,
        /** Applicable category  */
        val category: Category,
        /** Related rule, if any  */
        val rule: String?
    ) {
        /**
         * The name of this issue
         */
        lateinit var name: String
            internal set

        internal constructor(
            defaultLevel: Severity,
            category: Category = Category.UNKNOWN
        ) : this(defaultLevel, null, category, null)

        internal constructor(
            defaultLevel: Severity,
            category: Category,
            rule: String
        ) : this(defaultLevel, null, category, rule)

        internal constructor(
            parent: Issue,
            category: Category
        ) : this(Severity.INHERIT, parent, category, null)

        override fun toString(): String {
            return "Issue $name"
        }

        init {
            allIssues.add(this)
        }
    }

    enum class Category(val description: String, val ruleLink: String?) {
        COMPATIBILITY("Compatibility", null),
        DOCUMENTATION("Documentation", null),
        API_LINT("API Lint", "https://s.android.com/api-guidelines#"),
        UNKNOWN("Default", null)
    }

    init { // Initialize issue names based on the field names
        for (property in Issues::class.declaredMemberProperties) {
            if (property.returnType.classifier != Issue::class) continue
            val issue = property.getter.call(Issues) as Issue

            issue.name = SdkVersionInfo.underlinesToCamelCase(property.name.toLowerCase(Locale.US))
            nameToIssue[issue.name] = issue
        }
        for (issue in allIssues) {
            check(issue.name != "")
        }
    }
}
