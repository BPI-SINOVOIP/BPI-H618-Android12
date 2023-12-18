//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/HashNames.h"

#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/Symbol.h"

namespace sh
{

namespace
{
constexpr const ImmutableString kHashedNamePrefix("webgl_");

ImmutableString HashName(const ImmutableString &name, ShHashFunction64 hashFunction)
{
    ASSERT(!name.empty());
    ASSERT(hashFunction);
    khronos_uint64_t number = (*hashFunction)(name.data(), name.length());

    // Build the hashed name in place.
    static const unsigned int kHexStrMaxLength = sizeof(number) * 2;
    static const size_t kHashedNameMaxLength   = kHashedNamePrefix.length() + kHexStrMaxLength;

    ImmutableStringBuilder hashedName(kHashedNameMaxLength);
    hashedName << kHashedNamePrefix;

    hashedName.appendHex(number);

    return hashedName;
}

void AddToNameMapIfNotMapped(const ImmutableString &name,
                             const ImmutableString &hashedName,
                             NameMap *nameMap)
{
    if (nameMap)
    {
        NameMap::const_iterator it = nameMap->find(name.data());
        if (it != nameMap->end())
        {
            // (How bout returning?)
            return;
        }
        (*nameMap)[name.data()] = hashedName.data();
    }
}

}  // anonymous namespace

ImmutableString HashName(const ImmutableString &name,
                         ShHashFunction64 hashFunction,
                         NameMap *nameMap)
{
    const ImmutableString kUnhashedNamePrefix(kUserDefinedNamePrefix);

    if (hashFunction == nullptr)
    {
        if (name.length() + kUnhashedNamePrefix.length() > kESSLMaxIdentifierLength)
        {
            // If the identifier length is already close to the limit, we can't prefix it. This is
            // not a problem since there are no builtins or ANGLE's internal variables that would
            // have as long names and could conflict.
            return name;
        }
        if (name == "gl_ClipDistance")
        {
            // NOTE(hqle): When gl_ClipDistance is re-declared, it will become an UserDefined
            // symbol. Normally, UserDefined symbols will have "_u" prefix added to their names by
            // ANGLE. However, gl_ClipDistance is an exception. If we add "_u" to its name, the
            // backend won't be able to handle it properly. So for gl_ClipDistance, we won't add
            // "_u" prefix, instead we return it original name.
            //
            // The other way is treating gl_ClipDistance as an AngleInternal symbol when a
            // re-declaration occurs. AngleInternal symbols will have their name intact. However,
            // the issue is that the current code put a lot of restrictions on AngleInternal
            // symbols. For examples:
            //  - CollectVariables.cpp will not consider AngleInternal as varying output variales.
            //  - SymbolTable.cpp will throw an exception if AngleInternal symbols are declared by
            //  users. In this case, it would be gl_ClipDistance. This is because
            //  TSymbolTable::declare() only accepts an UserDefined symbol.
            //  - And potentially many other places that have some assumptions that haven't been
            //  discovered yet.
            //
            // If re-declared gl_ClipDistance was to be an AngleInternal symbol, a special "if (name
            // == "gl_ClipDistance")" handling would have to be put into all the above mentioned
            // cases. TParseContext::declareVariable() function would also have to be modified in
            // order to assign AngleInternal symbol type to the re-declared gl_ClipDistance
            // variable.
            // Compare to only this place has to be handled if re-declared gl_ClipDistance is
            // treated as an UserDefined symbol.
            //
            return name;
        }

        static const char* coreBuiltinConflictNames[] = {
            // Keywords in GLSL ES 1.00 but not in GLSL ES 3.00
            "texture",
            "textureProj",
            "textureLod",
            "textureProjLod",
            "textureGrad",
            "textureProjGrad",
            // Keywords in GLSL ES 3.00 but not in GLSL ES 1.00
            "resource",
            "patch",
            "sample",
            "subroutine",
            "common",
            "partition",
            "active",

            "filter",
            "image1D",
            "iimage1D",
            "uimage1D",
            "image1DArray",
            "iimage1DArray",
            "uimage1DArray",
            "image1DShadow",
            "image2DShadow",
            "image1DArrayShadow",
            "image2DArrayShadow",
            "imageBuffer",
            "iimageBuffer",
            "uimageBuffer",

            "sampler1DArray",
            "sampler1DArrayShadow",
            "isampler1D",
            "isampler1DArray",
            "usampler1D",
            "usampler1DArray",
            "isampler2DRect",
            "usampler2DRect",
            "samplerBuffer",
            "isamplerBuffer",
            "usamplerBuffer",
        };

        // TODO(lfy): hack that removes the prefixes until emulator can deal with it properly

        for (uint32_t i = 0; i < sizeof(coreBuiltinConflictNames) / sizeof(const char*); ++i) {
            if (name == coreBuiltinConflictNames[i]) {
                ImmutableStringBuilder prefixedName(kUnhashedNamePrefix.length() + name.length());
                prefixedName << kUnhashedNamePrefix << name;
                ImmutableString res = prefixedName;
                AddToNameMapIfNotMapped(name, res, nameMap);
                return res;
            }
        }

        return name;
    }

    // Has a hash function
    ImmutableString hashedName = HashName(name, hashFunction);
    AddToNameMapIfNotMapped(name, hashedName, nameMap);
    return hashedName;
}

ImmutableString HashName(const TSymbol *symbol, ShHashFunction64 hashFunction, NameMap *nameMap)
{
    if (symbol->symbolType() == SymbolType::Empty)
    {
        return kEmptyImmutableString;
    }
    if (symbol->symbolType() == SymbolType::AngleInternal ||
        symbol->symbolType() == SymbolType::BuiltIn)
    {
        return symbol->name();
    }
    return HashName(symbol->name(), hashFunction, nameMap);
}

}  // namespace sh
