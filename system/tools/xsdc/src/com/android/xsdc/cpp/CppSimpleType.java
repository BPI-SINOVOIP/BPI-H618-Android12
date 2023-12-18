/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.xsdc.cpp;

class CppSimpleType implements CppType {
    final private String name;
    final private String fullName;
    final private String rawParsingExpression;
    final private boolean list;
    final private boolean isEnum;

    CppSimpleType(String name, String rawParsingExpression, boolean list, boolean isEnum) {
        this.rawParsingExpression = rawParsingExpression;
        this.list = list;
        this.name = name;
        this.fullName = list ? String.format("std::vector<%s>", name) : name;
        this.isEnum = isEnum;
    }

    CppSimpleType(String name, String rawParsingExpression, boolean list) {
        this(name, rawParsingExpression, list, false);
    }

    boolean isList() {
        return list;
    }

    boolean isEnum() {
        return isEnum;
    }

    CppSimpleType newListType() throws CppCodeGeneratorException {
        if (list) throw new CppCodeGeneratorException("list of list is not supported");
        return new CppSimpleType(name, rawParsingExpression, true);
    }

    public String getTypeName() {
        return name;
    }

    @Override
    public String getName() {
        return fullName;
    }

    @Override
    public String getParsingExpression() {
        StringBuilder expression = new StringBuilder();
        if (list) {
            expression.append(
                    String.format("%s value;\n", getName()));
            expression.append(String.format("{\nint base = 0;\n"
                    + "int found;\n"
                    + "while(true) {\n"
                    + "found = raw.find_first_of(\" \", base);\n"
                    + "value.push_back(%s);\n"
                    + "if (found == raw.npos) break;\n"
                    + "base = found + 1;\n"
                    + "}\n",
                    String.format(rawParsingExpression, "raw.substr(base, found - base)")));
            expression.append("}\n");
        } else {
            expression.append(
                    String.format("%s %svalue = %s;\n", getName(),
                            this.name.equals("std::string") ? "&" : "",
                            String.format(rawParsingExpression, "raw")));
        }
        return expression.toString();
    }

    @Override
    public String getWritingExpression(String getValue, String name) {
        StringBuilder expression = new StringBuilder();
        if (list) {
            expression.append("{\nint count = 0;\n");
            expression.append(String.format("for (const auto& v : %s) {\n", getValue));
            String value;
            if (isEnum) {
                value = String.format("%sToString(v)", this.name);
            } else if (this.name.equals("char") || this.name.equals("unsigned char")) {
                value = "(int)v";
            } else if (this.name.equals("bool")) {
                value = "(v ? \"true\" : \"false\")";
            } else {
                value = "v";
            }
            expression.append("if (count != 0) {\n"
                    + "out << \" \";\n}\n"
                    + "++count;\n");
            expression.append(String.format("out << %s;\n}\n}\n", value));
        } else {
            if (isEnum) {
                expression.append(String.format("out << toString(%s);\n", getValue));
            } else if (this.name.equals("char") || this.name.equals("unsigned char")) {
                expression.append(String.format("out << (int)%s;\n", getValue));
            } else if (this.name.equals("bool")) {
                expression.append(String.format("out << (%s ? \"true\" : \"false\");\n", getValue));
            } else {
                expression.append(String.format("out << %s;\n", getValue));
            }
        }
        return expression.toString();
    }
}
