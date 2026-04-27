//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR dump utility for debugging.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <operators.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/program.hpp>

#include <sstream>
#include <string>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

class Dumper {
public:
  static std::string dump(const Program &program) {
    Dumper d(program);
    d.dump_program();
    return d.m_out.str();
  }

private:
  const Program &m_program;
  std::ostringstream m_out;
  int m_indent = 0;

  Dumper(const Program &program) : m_program(program) {}

  const Arena &arena() const { return m_program.m_arena; }

  void indent() {
    for (int i = 0; i < m_indent; ++i) {
      m_out << "  ";
    }
  }

  void line(const std::string &text) {
    indent();
    m_out << text << "\n";
  }

  struct IndentGuard {
    Dumper &d;
    IndentGuard(Dumper &d) : d(d) { ++d.m_indent; }
    ~IndentGuard() { --d.m_indent; }
  };

  std::string type_str(TypeId type_id) const {
    return arena().to_string(type_id);
  }

  const Binding &binding(BindingId id) const { return arena().get(id); }

  const Expression &expression(ExprId id) const { return arena().get(id); }

  // -- Program ---------------------------------------------------------------

  void dump_program() {
    line("Program");
    IndentGuard g(*this);
    for (const auto &item : m_program.m_top_items) {
      dump_top_item(item);
    }
  }

  // -- Top items -------------------------------------------------------------

  void dump_top_item(const TopItem &item) {
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, FunctionDef>) {
            dump_func_def(v);
          } else if constexpr (std::is_same_v<T, ExternFunctionDeclaration>) {
            dump_extern_function_declaration(v);
          } else if constexpr (std::is_same_v<T, LetBinding>) {
            dump_let_binding(v);
          }
        },
        item);
  }

  void dump_func_def(const FunctionDef &f) {
    const auto &func_binding = binding(f.m_id);
    indent();
    m_out << "FunctionDef " << func_binding.m_name << ": "
          << type_str(func_binding.m_type) << "\n";
    {
      IndentGuard g(*this);
      indent();
      m_out << "params(";
      for (size_t i = 0; i < f.m_parameters.size(); ++i) {
        if (i > 0) {
          m_out << ", ";
        }
        dump_binding(f.m_parameters[i]);
      }
      m_out << ")\n";
    }
    dump_block(f.m_body);
  }

  void dump_extern_function_declaration(const ExternFunctionDeclaration &f) {
    const auto &func_binding = binding(f.m_id);
    indent();
    m_out << "ExternFunctionDeclaration " << func_binding.m_name << ": "
          << type_str(func_binding.m_type) << "\n";
  }

  // -- Bindings --------------------------------------------------------------

  void dump_binding(BindingId id) {
    const auto &b = binding(id);
    m_out << b.m_name << ": " << type_str(b.m_type);
  }

  // -- Let bindings ----------------------------------------------------------

  void dump_let_binding(const LetBinding &lb) {
    indent();
    m_out << "Let ";
    dump_binding(lb.m_identifier);
    m_out << " =\n";
    IndentGuard g(*this);
    dump_expr(lb.m_expression);
  }

  // -- Blocks ----------------------------------------------------------------

  void dump_block(const Block &b) {
    line("Block");
    IndentGuard g(*this);
    for (const auto &stmt : b.m_statements) {
      dump_statement(stmt);
    }
    if (b.m_final_expression.has_value()) {
      line("=> (final)");
      IndentGuard g2(*this);
      dump_expr(*b.m_final_expression);
    }
  }

  // -- Statements ------------------------------------------------------------

  void dump_statement(const Statement &s) {
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, LetBinding>) {
            dump_let_binding(v);
          } else if constexpr (std::is_same_v<T, ExpressionStatement>) {
            dump_expr(v.m_expression);
          }
        },
        s);
  }

  // -- Expressions -----------------------------------------------------------

  void dump_expr(ExprId id) {
    const auto &expr = expression(id);
    indent();
    m_out << "[" << type_str(expr.m_type_id) << "] ";
    std::visit([this](const auto &v) { dump_expr_kind(v); }, expr.m_expr_kind);
  }

  void dump_expr_kind(const Unit & /*unused*/) { m_out << "Unit\n"; }

  void dump_expr_kind(const Bool &v) {
    m_out << "Bool(" << (v.m_value ? "true" : "false") << ")\n";
  }

  void dump_expr_kind(const Char &v) {
    m_out << "Char('" << v.m_value << "')\n";
  }

  void dump_expr_kind(const I8 &v) {
    m_out << "I8(" << static_cast<int>(v.m_value) << ")\n";
  }

  void dump_expr_kind(const I32 &v) { m_out << "I32(" << v.m_value << ")\n"; }

  void dump_expr_kind(const I64 &v) { m_out << "I64(" << v.m_value << ")\n"; }

  void dump_expr_kind(const IdentifierExpr &v) {
    const auto &b = binding(v.m_id);
    m_out << "Ident(" << b.m_name << ")\n";
  }

  void dump_expr_kind(const BinaryExpr &v) {
    m_out << "Binary(" << to_cstring(v.m_operator) << ")\n";
    IndentGuard g(*this);
    dump_expr(v.m_lhs);
    dump_expr(v.m_rhs);
  }

  void dump_expr_kind(const UnaryExpr &v) {
    m_out << "Unary(" << to_cstring(v.m_operator) << ")\n";
    IndentGuard g(*this);
    dump_expr(v.m_expression);
  }

  void dump_expr_kind(const CallExpr &v) {
    m_out << "Call\n";
    IndentGuard g(*this);
    line("callee:");
    {
      IndentGuard g2(*this);
      dump_expr(v.m_callee);
    }
    if (!v.m_arguments.empty()) {
      line("args:");
      IndentGuard g2(*this);
      for (const auto &arg : v.m_arguments) {
        dump_expr(arg);
      }
    }
  }

  void dump_expr_kind(const IfExpr &v) {
    m_out << "If\n";
    IndentGuard g(*this);
    line("cond:");
    {
      IndentGuard g2(*this);
      dump_expr(v.m_condition);
    }
    line("then:");
    {
      IndentGuard g2(*this);
      dump_block(v.m_then_block);
    }
    if (v.m_else_block.has_value()) {
      line("else:");
      IndentGuard g2(*this);
      dump_block(*v.m_else_block);
    }
  }

  void dump_expr_kind(const ReturnExpr &v) {
    m_out << "Return\n";
    IndentGuard g(*this);
    dump_expr(v.m_expression);
  }

  void dump_expr_kind(const CastExpr &v) {
    m_out << "Cast -> " << type_str(v.m_new_type) << "\n";
    IndentGuard g(*this);
    dump_expr(v.m_expression);
  }

  void dump_expr_kind(const LambdaExpr &v) {
    m_out << "Lambda(";
    for (size_t i = 0; i < v.m_parameters.size(); ++i) {
      if (i > 0) {
        m_out << ", ";
      }
      dump_binding(v.m_parameters[i].m_id);
    }
    m_out << ") -> " << type_str(v.m_return_type) << "\n";
    IndentGuard g(*this);
    dump_block(v.m_body);
  }

  void dump_expr_kind(const Block &v) {
    m_out << "Block\n";
    IndentGuard g(*this);
    dump_block(v);
  }

  void dump_expr_kind(const TupleExpr &v) {
    m_out << "Tuple\n";
    IndentGuard g(*this);
    for (const auto &field : v.m_fields) {
      dump_expr(field);
    }
  }

  void dump_expr_kind(const DotExpr &v) {
    m_out << "Dot(" << v.m_tuple_index << ")\n";
    IndentGuard g(*this);
    dump_expr(v.m_expression);
  }
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
