//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : AST dump utility for debugging.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>

#include <sstream>
#include <string>
#include <utility>

//****************************************************************************
namespace bust::ast {
//****************************************************************************

class Dumper {
public:
  static std::string dump(const Program &program) {
    Dumper d;
    d.dump_program(program);
    return d.m_out.str();
  }

private:
  std::ostringstream m_out;
  int m_indent = 0;

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

  void dump_program(const Program &p) {
    line("Program");
    IndentGuard g(*this);
    for (const auto &item : p.m_items) {
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
            dump_extern_func_declaration(v);
          } else if constexpr (std::is_same_v<T, LetBinding>) {
            dump_let_binding(v);
          }
        },
        item);
  }

  void dump_type_id(const TypeIdentifier &tid) {
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, PrimitiveTypeIdentifier>) {
            m_out << to_string(v.m_type);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<
                                                     FunctionTypeIdentifier>>) {
            m_out << "fn(";
            for (size_t i = 0; i < v->m_parameter_types.size(); ++i) {
              if (i > 0) {
                m_out << ", ";
              }
              dump_type_id(v->m_parameter_types[i]);
            }
            m_out << ") -> ";
            dump_type_id(v->m_return_type);
          } else if constexpr (std::is_same_v<
                                   T, std::unique_ptr<TupleTypeIdentifier>>) {
            m_out << "(";
            for (size_t i = 0; i < v->m_field_types.size(); ++i) {
              if (i > 0) {
                m_out << " ";
              }
              dump_type_id(v->m_field_types[i]);
              m_out << ",";
            }
            m_out << ")";
          } else if constexpr (std::is_same_v<T, DefinedType>) {
            m_out << v.m_type;
          }
        },
        tid);
  }

  void dump_identifier(const Identifier &id) {
    m_out << id.m_name;
    if (id.m_type.has_value()) {
      m_out << ": ";
      dump_type_id(*id.m_type);
    }
  }

  void dump_func_declaration(const FunctionDeclaration &function_declaration) {
    m_out << function_declaration.m_id.m_name << "(";
    for (size_t i = 0; i < function_declaration.m_parameters.size(); ++i) {
      if (i > 0) {
        m_out << ", ";
      }
      dump_identifier(function_declaration.m_parameters[i]);
    }
    m_out << ") -> ";
    dump_type_id(function_declaration.m_return_type);
  }

  void dump_func_def(const FunctionDef &f) {
    indent();
    m_out << "FunctionDef ";
    dump_func_declaration(f.m_signature);
    m_out << "\n";
    IndentGuard g(*this);
    dump_block(f.m_body);
  }

  void dump_extern_func_declaration(const ExternFunctionDeclaration &f) {
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
    std::visit(
        [this](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, Identifier>) {
            indent();
            m_out << "Ident(" << v.m_name << ")\n";
          } else if constexpr (std::is_same_v<T, I64>) {
            indent();
            m_out << "Int(" << v.m_value << ")\n";
          } else if constexpr (std::is_same_v<T, Bool>) {
            indent();
            m_out << "Bool(" << (v.m_value ? "true" : "false") << ")\n";
          } else if constexpr (std::is_same_v<T, Char>) {
            indent();
            m_out << "Char('" << (v.m_value) << "')\n";
          } else if constexpr (std::is_same_v<T, Unit>) {
            line("Unit");
          } else if constexpr (std::is_same_v<T, std::unique_ptr<BinaryExpr>>) {
            dump_binary(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<UnaryExpr>>) {
            dump_unary(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<CallExpr>>) {
            dump_call(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<IfExpr>>) {
            dump_if(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<Block>>) {
            dump_block(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<CastExpr>>) {
            dump_cast(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<ReturnExpr>>) {
            dump_return(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<LambdaExpr>>) {
            dump_lambda(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<TupleExpr>>) {
            dump_tuple(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<DotExpr>>) {
            dump_dot(*v);
          } else if constexpr (std::is_same_v<T, std::unique_ptr<WhileExpr>>) {
            line("While(TODO)");
          } else if constexpr (std::is_same_v<T, std::unique_ptr<ForExpr>>) {
            line("For(TODO)");
          } else {
            throw core::InternalCompilerError("Unreachable!!");
          }
        },
        e.m_expression);
  }

  static const char *binary_op_str(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::PLUS:
      return "+";
    case BinaryOperator::MINUS:
      return "-";
    case BinaryOperator::MULTIPLIES:
      return "*";
    case BinaryOperator::DIVIDES:
      return "/";
    case BinaryOperator::MODULUS:
      return "%";
    case BinaryOperator::EQ:
      return "==";
    case BinaryOperator::NOT_EQ:
      return "!=";
    case BinaryOperator::LT:
      return "<";
    case BinaryOperator::LT_EQ:
      return "<=";
    case BinaryOperator::GT:
      return ">";
    case BinaryOperator::GT_EQ:
      return ">=";
    case BinaryOperator::LOGICAL_AND:
      return "&&";
    case BinaryOperator::LOGICAL_OR:
      return "||";
    }
  }

  static const char *unary_op_str(UnaryOperator op) {
    switch (op) {
    case UnaryOperator::MINUS:
      return "-";
    case UnaryOperator::NOT:
      return "!";
    }
  }

  void dump_binary(const BinaryExpr &b) {
    indent();
    m_out << "Binary(" << binary_op_str(b.m_operator) << ")\n";
    IndentGuard g(*this);
    dump_expression(b.m_lhs);
    dump_expression(b.m_rhs);
  }

  void dump_unary(const UnaryExpr &u) {
    indent();
    m_out << "Unary(" << unary_op_str(u.m_operator) << ")\n";
    IndentGuard g(*this);
    dump_expression(u.m_expression);
  }

  void dump_call(const CallExpr &c) {
    line("Call");
    IndentGuard g(*this);
    line("callee:");
    {
      IndentGuard g2(*this);
      dump_expression(c.m_callee);
    }
    if (!c.m_arguments.empty()) {
      line("args:");
      IndentGuard g2(*this);
      for (const auto &arg : c.m_arguments) {
        dump_expression(arg);
      }
    }
  }

  void dump_if(const IfExpr &i) {
    line("If");
    IndentGuard g(*this);
    line("cond:");
    {
      IndentGuard g2(*this);
      dump_expression(i.m_condition);
    }
    line("then:");
    {
      IndentGuard g2(*this);
      dump_block(i.m_then_block);
    }
    if (i.m_else_block.has_value()) {
      line("else:");
      IndentGuard g2(*this);
      dump_block(*i.m_else_block);
    }
  }

  void dump_cast(const CastExpr &c) {
    line("Cast");
    IndentGuard g(*this);
    dump_expression(c.m_expression);
    m_out << " AS ";
    dump_type_id(c.m_new_type);
  }

  void dump_return(const ReturnExpr &r) {
    line("Return");
    IndentGuard g(*this);
    dump_expression(r.m_expression);
  }

  void dump_tuple(const TupleExpr &t) {
    line("Tuple");
    IndentGuard g(*this);
    for (const auto &field : t.m_fields) {
      dump_expression(field);
    }
  }

  void dump_dot(const DotExpr &d) {
    indent();
    m_out << "Dot(" << d.m_tuple_index << ")\n";
    IndentGuard g(*this);
    dump_expression(d.m_expression);
  }

  void dump_lambda(const LambdaExpr &l) {
    indent();
    m_out << "Lambda(";
    for (size_t i = 0; i < l.m_parameters.size(); ++i) {
      if (i > 0) {
        m_out << ", ";
      }
      dump_identifier(l.m_parameters[i]);
    }
    m_out << ")";
    if (l.m_return_type.has_value()) {
      m_out << " -> ";
      dump_type_id(*l.m_return_type);
    }
    m_out << "\n";
    IndentGuard g(*this);
    dump_block(l.m_body);
  }
};

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
