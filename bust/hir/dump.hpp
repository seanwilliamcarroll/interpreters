//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : HIR dump utility for debugging.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <operators.hpp>

#include <sstream>
#include <string>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

class Dumper {
public:
  static std::string dump(const Program &program) {
    return Dumper::dump(program.m_type_arena, program.m_top_items);
  }

  static std::string dump(const TypeArena &type_arena,
                          const std::vector<TopItem> &top_items) {
    Dumper d(type_arena, top_items);
    d.dump_program();
    return d.m_out.str();
  }

private:
  const TypeArena &m_type_arena;
  const std::vector<TopItem> &m_top_items;
  std::ostringstream m_out;
  int m_indent = 0;

  Dumper(const TypeArena &type_arena, const std::vector<TopItem> &top_items)
      : m_type_arena(type_arena), m_top_items(top_items) {}

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

  void dump_program() {
    line("Program");
    IndentGuard g(*this);
    for (const auto &item : m_top_items) {
      dump_top_item(item);
    }
  }

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

  void dump_identifier(const Identifier &id) {
    m_out << id.m_name << ": " << m_type_arena.to_string(id.m_type);
  }

  void dump_func_declaration(const FunctionDeclaration &f) {
    m_out << f.m_function_id << ": " << m_type_arena.to_string(f.m_type)
          << "\n";
    IndentGuard g(*this);
    indent();
    m_out << "params(";
    for (size_t i = 0; i < f.m_parameters.size(); ++i) {
      if (i > 0) {
        m_out << ", ";
      }
      dump_identifier(f.m_parameters[i]);
    }
    m_out << ")";
  }

  void dump_func_def(const FunctionDef &f) {
    indent();
    m_out << "FunctionDef ";
    dump_func_declaration(f.m_signature);
    m_out << "\n";
    dump_block(f.m_body);
  }

  void dump_extern_function_declaration(const ExternFunctionDeclaration &f) {
    indent();
    m_out << "ExternFunctionDeclaration ";
    dump_func_declaration(f.m_signature);
    m_out << "\n";
  }

  void dump_let_binding(const LetBinding &lb) {
    indent();
    m_out << "Let ";
    dump_identifier(lb.m_variable);
    m_out << " =\n";
    IndentGuard g(*this);
    dump_expression(lb.m_expression);
  }

  void dump_block(const Block &b) {
    line("Block");
    IndentGuard g(*this);
    for (const auto &stmt : b.m_statements) {
      dump_statement(stmt);
    }
    if (b.m_final_expression.has_value()) {
      line("=> (final)");
      IndentGuard g2(*this);
      dump_expression(*b.m_final_expression);
    }
  }

  void dump_statement(const Statement &s) {
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, LetBinding>) {
            dump_let_binding(v);
          } else if constexpr (std::is_same_v<T, Expression>) {
            dump_expression(v);
          }
        },
        s);
  }

  void dump_expression(const Expression &e) {
    indent();
    m_out << "[" << m_type_arena.to_string(e.m_type) << "] ";
    // Reset indent for the inner content since we already indented
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, Identifier>) {
            m_out << "Ident(" << v.m_name << ")\n";
          } else if constexpr (std::is_same_v<T, LiteralI64>) {
            m_out << "Int(" << v.m_value << ")\n";
          } else if constexpr (std::is_same_v<T, LiteralBool>) {
            m_out << "Bool(" << (v.m_value ? "true" : "false") << ")\n";
          } else if constexpr (std::is_same_v<T, LiteralUnit>) {
            m_out << "Unit\n";
          } else if constexpr (std::is_same_v<T, std::unique_ptr<BinaryExpr>>) {
            m_out << "Binary(" << v->m_operator << ")\n";
            IndentGuard g(*this);
            dump_expression(v->m_lhs);
            dump_expression(v->m_rhs);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<UnaryExpr>>) {
            m_out << "Unary(" << v->m_operator << ")\n";
            IndentGuard g(*this);
            dump_expression(v->m_expression);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<CallExpr>>) {
            m_out << "Call\n";
            IndentGuard g(*this);
            line("callee:");
            {
              IndentGuard g2(*this);
              dump_expression(v->m_callee);
            }
            if (!v->m_arguments.empty()) {
              line("args:");
              IndentGuard g2(*this);
              for (const auto &arg : v->m_arguments) {
                dump_expression(arg);
              }
            }
          } else if constexpr (std::is_same_v<T, std::unique_ptr<IfExpr>>) {
            m_out << "If\n";
            IndentGuard g(*this);
            line("cond:");
            {
              IndentGuard g2(*this);
              dump_expression(v->m_condition);
            }
            line("then:");
            {
              IndentGuard g2(*this);
              dump_block(v->m_then_block);
            }
            if (v->m_else_block.has_value()) {
              line("else:");
              IndentGuard g2(*this);
              dump_block(*v->m_else_block);
            }
          } else if constexpr (std::is_same_v<T, std::unique_ptr<Block>>) {
            m_out << "Block\n";
            IndentGuard g(*this);
            dump_block(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<ReturnExpr>>) {
            m_out << "Return\n";
            IndentGuard g(*this);
            dump_expression(v->m_expression);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<CastExpr>>) {
            m_out << "Cast\n";
            IndentGuard g(*this);
            dump_expression(v->m_expression);
            m_out << " AS " << m_type_arena.to_string(v->m_new_type);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<LambdaExpr>>) {
            m_out << "Lambda(";
            for (size_t i = 0; i < v->m_parameters.size(); ++i) {
              if (i > 0) {
                m_out << ", ";
              }
              dump_identifier(v->m_parameters[i]);
            }
            m_out << ")\n";
            IndentGuard g(*this);
            dump_block(v->m_body);
          }
        },
        e.m_expression);
  }
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
