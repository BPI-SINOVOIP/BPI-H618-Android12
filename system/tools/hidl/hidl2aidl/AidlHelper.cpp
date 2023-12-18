/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <hidl-util/FQName.h>
#include <hidl-util/Formatter.h>
#include <hidl-util/StringHelper.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "AidlHelper.h"
#include "ArrayType.h"
#include "CompoundType.h"
#include "Coordinator.h"
#include "Interface.h"
#include "Method.h"
#include "NamedType.h"
#include "Reference.h"
#include "Scope.h"

namespace android {

Formatter* AidlHelper::notesFormatter = nullptr;
std::string AidlHelper::fileHeader = "";
bool AidlHelper::expandExtended = false;

Formatter& AidlHelper::notes() {
    CHECK(notesFormatter != nullptr);
    return *notesFormatter;
}

void AidlHelper::setNotes(Formatter* formatter) {
    CHECK(formatter != nullptr);
    notesFormatter = formatter;
}

std::string AidlHelper::getAidlName(const FQName& fqName) {
    std::vector<std::string> names;
    for (const std::string& name : fqName.names()) {
        names.push_back(StringHelper::Capitalize(name));
    }
    return StringHelper::JoinStrings(names, "");
}

std::string AidlHelper::getAidlPackage(const FQName& fqName) {
    std::string aidlPackage = fqName.package();
    if (fqName.getPackageMajorVersion() != 1) {
        aidlPackage += std::to_string(fqName.getPackageMajorVersion());
    }

    return aidlPackage;
}

std::string AidlHelper::getAidlPackagePath(const FQName& fqName) {
    return base::Join(base::Split(AidlHelper::getAidlPackage(fqName), "."), "/");
}

std::optional<std::string> AidlHelper::getAidlFQName(const FQName& fqName) {
    std::optional<const ReplacedTypeInfo> type = getAidlReplacedType(fqName);
    if (type) {
        return type.value().aidlReplacedFQName;
    }
    return getAidlPackage(fqName) + "." + getAidlName(fqName);
}

void AidlHelper::importLocallyReferencedType(const Type& type, std::set<std::string>* imports) {
    if (type.isArray()) {
        return importLocallyReferencedType(*static_cast<const ArrayType*>(&type)->getElementType(),
                                           imports);
    }
    if (type.isTemplatedType()) {
        return importLocallyReferencedType(
                *static_cast<const TemplatedType*>(&type)->getElementType(), imports);
    }

    if (!type.isNamedType()) return;
    const NamedType& namedType = *static_cast<const NamedType*>(&type);

    std::optional<std::string> import = AidlHelper::getAidlFQName(namedType.fqName());
    if (import) {
        imports->insert(import.value());
    }
}

// This tries iterating over the HIDL AST which is a bit messy because
// it has to encode the logic in the rest of hidl2aidl. It would be better
// if we could iterate over the AIDL structure which has already been
// processed.
void AidlHelper::emitFileHeader(
        Formatter& out, const NamedType& type,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    AidlHelper::emitFileHeader(out);
    out << "package " << getAidlPackage(type.fqName()) << ";\n\n";

    std::set<std::string> imports;

    // Import all the defined types since they will now be in a different file
    if (type.isScope()) {
        const Scope& scope = static_cast<const Scope&>(type);
        for (const NamedType* namedType : scope.getSubTypes()) {
            importLocallyReferencedType(*namedType, &imports);
        }
    }

    // Import all the referenced types
    if (type.isInterface()) {
        // This is a separate case because getReferences doesn't traverse all the superTypes and
        // sometimes includes references to types that would not exist on AIDL
        const std::vector<const Method*>& methods =
                getUserDefinedMethods(out, static_cast<const Interface&>(type));
        for (const Method* method : methods) {
            for (const Reference<Type>* ref : method->getReferences()) {
                importLocallyReferencedType(*ref->get(), &imports);
            }
        }
    } else if (type.isCompoundType()) {
        // Get all of the imports for the flattened compound type that may
        // include additional fields and subtypes from older versions
        const auto& it = processedTypes.find(&type);
        CHECK(it != processedTypes.end()) << "Failed to find " << type.fullName();
        const ProcessedCompoundType& processedType = it->second;

        for (const auto& field : processedType.fields) {
            importLocallyReferencedType(*field.field->get(), &imports);
        }
        for (const auto& subType : processedType.subTypes) {
            importLocallyReferencedType(*subType, &imports);
        }
    } else {
        for (const Reference<Type>* ref : type.getReferences()) {
            if (ref->get()->definedName() == type.fqName().name()) {
                // Don't import the referenced type if this is referencing itself
                continue;
            }
            importLocallyReferencedType(*ref->get(), &imports);
        }
    }

    for (const std::string& import : imports) {
        out << "import " << import << ";\n";
    }

    if (imports.size() > 0) {
        out << "\n";
    }
}

Formatter AidlHelper::getFileWithHeader(
        const NamedType& namedType, const Coordinator& coordinator,
        const std::map<const NamedType*, const ProcessedCompoundType>& processedTypes) {
    Formatter out =
            coordinator.getFormatter(namedType.fqName(), Coordinator::Location::DIRECT,
                                     AidlHelper::getAidlPackagePath(namedType.fqName()) + "/" +
                                             getAidlName(namedType.fqName()) + ".aidl");
    emitFileHeader(out, namedType, processedTypes);
    return out;
}

void AidlHelper::processCompoundType(const CompoundType& compoundType,
                                     ProcessedCompoundType* processedType,
                                     const std::string& fieldNamePrefix) {
    // Gather all of the subtypes defined in this type
    for (const NamedType* subType : compoundType.getSubTypes()) {
        processedType->subTypes.insert(subType);
    }
    std::pair<size_t, size_t> version = compoundType.fqName().hasVersion()
                                                ? compoundType.fqName().getVersion()
                                                : std::pair<size_t, size_t>{0, 0};
    for (const NamedReference<Type>* field : compoundType.getFields()) {
        // Check for references to another version of itself
        if (field->get()->typeName() == compoundType.typeName()) {
            if (AidlHelper::shouldBeExpanded(
                        static_cast<const CompoundType&>(*field->get()).fqName(),
                        compoundType.fqName())) {
                processCompoundType(static_cast<const CompoundType&>(*field->get()), processedType,
                                    fieldNamePrefix + field->name() + ".");
            } else {
                // Keep this field as is
                processedType->fields.push_back({field, fieldNamePrefix + field->name(), version});
            }
        } else {
            // Handle duplicate field names. Keep only the most recent definitions.
            auto it = std::find_if(processedType->fields.begin(), processedType->fields.end(),
                                   [field](auto& processedField) {
                                       return processedField.field->name() == field->name();
                                   });
            if (it != processedType->fields.end()) {
                AidlHelper::notes()
                        << "Found conflicting field name \"" << field->name()
                        << "\" in different versions of " << compoundType.fqName().name() << ". ";

                if (version.first > it->version.first ||
                    (version.first == it->version.first && version.second > it->version.second)) {
                    AidlHelper::notes()
                            << "Keeping " << field->get()->typeName() << " from " << version.first
                            << "." << version.second << " and discarding "
                            << (it->field)->get()->typeName() << " from " << it->version.first
                            << "." << it->version.second << ".\n";
                    it->fullName = fieldNamePrefix + field->name();
                    it->field = field;
                    it->version = version;
                } else {
                    AidlHelper::notes()
                            << "Keeping " << (it->field)->get()->typeName() << " from "
                            << it->version.first << "." << it->version.second << " and discarding "
                            << field->get()->typeName() << " from " << version.first << "."
                            << version.second << ".\n";
                }
            } else {
                processedType->fields.push_back({field, fieldNamePrefix + field->name(), version});
            }
        }
    }
}

void AidlHelper::setFileHeader(const std::string& file) {
    if (!file.empty()) {
        if (!android::base::ReadFileToString(file, &fileHeader)) {
            std::cerr << "ERROR: Failed to find license file: " << file << "\n";
            exit(1);
        }
    }
}

void AidlHelper::emitFileHeader(Formatter& out) {
    if (fileHeader.empty()) {
        out << "// FIXME: license file, or use the -l option to generate the files with the "
               "header.\n\n";
    } else {
        out << fileHeader << "\n";
    }
}

bool AidlHelper::shouldBeExpanded(const FQName& a, const FQName& b) {
    return a.package() == b.package() || expandExtended;
}

}  // namespace android
