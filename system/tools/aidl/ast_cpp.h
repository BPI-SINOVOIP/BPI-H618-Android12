/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace android {
namespace aidl {
class CodeWriter;
}  // namespace aidl
}  // namespace android

namespace android {
namespace aidl {
namespace cpp {

class AstNode {
 public:
  AstNode() = default;
  virtual ~AstNode() = default;

  // All ast nodes are non-copyable and non-movable
  AstNode(const AstNode&) = delete;
  AstNode(AstNode&&) = delete;
  AstNode& operator=(const AstNode&) = delete;
  AstNode& operator=(AstNode&&) = delete;

  virtual void Write(CodeWriter* to) const = 0;
  std::string ToString();
};  // class AstNode

class Declaration : public AstNode {
 public:
  Declaration() = default;
  virtual ~Declaration() = default;
};  // class Declaration

class LiteralDecl : public Declaration {
 public:
  explicit LiteralDecl(const std::string& expression);
  ~LiteralDecl() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string expression_;
};  // class LiteralDecl

class ClassDecl : public Declaration {
 public:
  ClassDecl(const std::string& name, const std::string& parent,
            const std::vector<std::string>& template_params, const std::string& attributes = "");
  ClassDecl(const std::string& name, const std::string& parent,
            const std::vector<std::string>& template_params,
            std::vector<std::unique_ptr<Declaration>> public_members,
            std::vector<std::unique_ptr<Declaration>> private_members,
            const std::string& attributes = "");
  virtual ~ClassDecl() = default;

  void Write(CodeWriter* to) const override;

  void AddPublic(std::unique_ptr<Declaration> member);
  void AddPrivate(std::unique_ptr<Declaration> member);

 private:
  std::string name_;
  std::string parent_;
  std::string attributes_;
  std::vector<std::string> template_params_;
  std::vector<std::unique_ptr<Declaration>> public_members_;
  std::vector<std::unique_ptr<Declaration>> private_members_;
};  // class ClassDecl

class Enum : public Declaration {
 public:
  Enum(const std::string& name, const std::string& base_type, bool is_class,
       const std::string& attributes = "");
  virtual ~Enum() = default;

  bool HasValues() const { return !fields_.empty(); }
  void Write(CodeWriter* to) const override;

  void AddValue(const std::string& key, const std::string& value,
                const std::string& attribute = "");

 private:
  struct EnumField {
    EnumField(const std::string& k, const std::string& v, const std::string& a);
    const std::string key;
    const std::string value;
    const std::string attribute;
  };

  std::string enum_name_;
  std::string underlying_type_;
  std::string attributes_;
  bool is_class_;
  std::vector<EnumField> fields_;
};  // class Enum

class ArgList : public AstNode {
 public:
  ArgList() = default;
  explicit ArgList(const std::string& single_argument);
  explicit ArgList(const std::vector<std::string>& arg_list);
  explicit ArgList(std::vector<std::unique_ptr<AstNode>> arg_list);
  ArgList(ArgList&& arg_list) noexcept;
  virtual ~ArgList() = default;

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::unique_ptr<AstNode>> arguments_;
};  // class ArgList

class ConstructorDecl : public Declaration {
 public:
  enum Modifiers {
    IS_VIRTUAL = 1 << 0,
    IS_DEFAULT = 1 << 1,
    IS_EXPLICIT = 1 << 2,
  };

  ConstructorDecl(const std::string& name,
                  ArgList&& arg_list);
  ConstructorDecl(const std::string& name,
                  ArgList&& arg_list,
                  uint32_t modifiers);

  virtual ~ConstructorDecl() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string name_;
  const ArgList arguments_;
  const uint32_t modifiers_;
};  // class ConstructorDecl

class MacroDecl : public Declaration {
 public:
  MacroDecl(const std::string& name, ArgList&& arg_list);
  virtual ~MacroDecl() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string name_;
  const ArgList arguments_;
};  // class MacroDecl

class MethodDecl : public Declaration {
 public:
  enum Modifiers {
    IS_CONST = 1 << 0,
    IS_VIRTUAL = 1 << 1,
    IS_OVERRIDE = 1 << 2,
    IS_PURE_VIRTUAL = 1 << 3,
    IS_STATIC = 1 << 4,
    IS_FINAL = 1 << 5,
  };

  MethodDecl(const std::string& return_type, const std::string& name, ArgList&& arg_list,
             const std::string& attributes = "");
  MethodDecl(const std::string& return_type, const std::string& name, ArgList&& arg_list,
             uint32_t modifiers, const std::string& attributes = "");
  virtual ~MethodDecl() = default;

  void Write(CodeWriter* to) const override;

 private:
  const std::string return_type_;
  const std::string name_;
  const std::string attributes_;
  const ArgList arguments_;
  bool is_const_ = false;
  bool is_virtual_ = false;
  bool is_override_ = false;
  bool is_pure_virtual_ = false;
  bool is_static_ = true;
  bool is_final_ = false;
};  // class MethodDecl

class StatementBlock : public Declaration {
 public:
  StatementBlock() = default;
  virtual ~StatementBlock() = default;

  void AddStatement(std::unique_ptr<AstNode> statement);
  void AddStatement(AstNode* statement);  // Takes ownership
  void AddLiteral(const std::string& expression, bool add_semicolon = true);
  bool Empty() const { return statements_.empty(); }

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::unique_ptr<AstNode>> statements_;
};  // class StatementBlock

class ConstructorImpl : public Declaration {
 public:
  ConstructorImpl(const std::string& class_name,
                  ArgList&& arg_list,
                  const std::vector<std::string>& initializer_list);
  virtual ~ConstructorImpl() = default;

  // ConstructorImpl retains ownership of the statement block.
  StatementBlock* GetStatementBlock();

  void Write(CodeWriter* to) const override;

 private:
  std::string class_name_;
  ArgList arguments_;
  std::vector<std::string> initializer_list_;
  StatementBlock body_;
};  // class ConstructorImpl

class MethodImpl : public Declaration {
 public:
  // Passing an empty class name causes the method to be declared as a normal
  // function (ie. no ClassName:: qualifier).
  MethodImpl(const std::string& return_type, const std::string& class_name,
             const std::string& method_name, const std::vector<std::string>& template_params,
             ArgList&& arg_list, bool is_const_method = false);
  virtual ~MethodImpl() = default;

  // MethodImpl retains ownership of the statement block.
  StatementBlock* GetStatementBlock();

  void Write(CodeWriter* to) const override;

 private:
  std::string return_type_;
  std::string method_name_;
  const ArgList arguments_;
  StatementBlock statements_;
  bool is_const_method_ = false;
  std::vector<std::string> template_params_;
};  // class MethodImpl

class SwitchStatement : public AstNode {
 public:
  explicit SwitchStatement(const std::string& expression);
  virtual ~SwitchStatement() = default;

  // Add a case statement and return a pointer code block corresponding
  // to the case.  The switch statement will add a break statement
  // after the code block by default to prevent accidental fall-through.
  // Returns nullptr on duplicate value expressions (by strcmp, not value
  // equivalence).
  StatementBlock* AddCase(const std::string& value_expression);
  void Write(CodeWriter* to) const override;

 private:
  const std::string switch_expression_;
  std::vector<std::string> case_values_;
  std::vector<std::unique_ptr<StatementBlock>> case_logic_;
};  // class SwitchStatement

class Assignment : public AstNode {
 public:
  Assignment(const std::string& left, const std::string& right);
  Assignment(const std::string& left, AstNode* right);
  ~Assignment() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string lhs_;
  std::unique_ptr<AstNode> rhs_;
};  // class Assignment

class MethodCall : public AstNode {
 public:
  MethodCall(const std::string& method_name,
             const std::string& single_argument);
  MethodCall(const std::string& method_name, ArgList&& arg_list);
  ~MethodCall() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string method_name_;
  const ArgList arguments_;
};  // class MethodCall

class IfStatement : public AstNode {
 public:
  explicit IfStatement(AstNode* expression,
              bool invert_expression = false);
  virtual ~IfStatement() = default;
  StatementBlock* OnTrue() { return &on_true_; }
  StatementBlock* OnFalse() { return &on_false_; }
  void Write(CodeWriter* to) const override;

 private:
  std::unique_ptr<AstNode> expression_;
  bool invert_expression_ = false;
  StatementBlock on_true_;
  StatementBlock on_false_;
};  // class IfStatement

class Statement : public AstNode {
 public:
  explicit Statement(std::unique_ptr<AstNode> expression);
  explicit Statement(AstNode* expression);  // Takes possession.
  explicit Statement(const std::string& expression);
  ~Statement() = default;
  void Write(CodeWriter* to) const override;

 private:
  std::unique_ptr<AstNode> expression_;
};  // class Statement

class Comparison : public AstNode {
 public:
  Comparison(AstNode* lhs, const std::string& comparison, AstNode* rhs);
  ~Comparison() = default;
  void Write(CodeWriter* to) const override;

 private:
  std::unique_ptr<AstNode> left_;
  std::unique_ptr<AstNode> right_;
  const std::string operator_;
};  // class Comparison

class LiteralExpression : public AstNode {
 public:
  explicit LiteralExpression(const std::string& expression);
  ~LiteralExpression() = default;
  void Write(CodeWriter* to) const override;

 private:
  const std::string expression_;
};  // class LiteralExpression

class CppNamespace : public Declaration {
 public:
  CppNamespace(const std::string& name,
               std::vector<std::unique_ptr<Declaration>> declarations);
  CppNamespace(const std::string& name,
               std::unique_ptr<Declaration> declaration);
  explicit CppNamespace(const std::string& name);
  virtual ~CppNamespace() = default;

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::unique_ptr<Declaration>> declarations_;
  std::string name_;
};  // class CppNamespace

class Document : public AstNode {
 public:
  Document(const std::vector<std::string>& include_list,
           std::vector<std::unique_ptr<Declaration>> declarations);

  void Write(CodeWriter* to) const override;

 private:
  std::vector<std::string> include_list_;
  std::vector<std::unique_ptr<Declaration>> declarations_;
};  // class Document

class CppHeader final : public Document {
 public:
  CppHeader(const std::vector<std::string>& include_list,
            std::vector<std::unique_ptr<Declaration>> declarations);
  void Write(CodeWriter* to) const override;
};  // class CppHeader

class CppSource final : public Document {
 public:
  CppSource(const std::vector<std::string>& include_list,
            std::vector<std::unique_ptr<Declaration>> declarations);
};  // class CppSource

}  // namespace cpp
}  // namespace aidl
}  // namespace android
