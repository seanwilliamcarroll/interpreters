//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type definitions for bust HIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <ostream>
#include <source_location.hpp>
#include <string>
#include <types.hpp>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
using bust::PrimitiveType;
using core::HasLocation;
//****************************************************************************

struct PrimitiveTypeValue : public HasLocation {
  PrimitiveType m_type;
};

struct FunctionType;

struct NeverType : public HasLocation {};

struct UnknownType : public HasLocation {};

// TODO: Do we want to have an InferredType and ExplicitType, where Explicit has
// a location?
// TODO: Unknown type?
// TODO: User defined types of some kind
using Type = std::variant<PrimitiveTypeValue, std::unique_ptr<FunctionType>,
                          NeverType, UnknownType>;

struct FunctionType : public HasLocation {
  // I think this should have a location, inferred types may not
  std::vector<Type> m_argument_types;
  Type m_return_type;
};

inline core::SourceLocation type_location(const Type &type) {
  return std::visit(
      [](const auto &t) -> core::SourceLocation {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<FunctionType>>) {
          return t->m_location;
        } else {
          return t.m_location;
        }
      },
      type);
}

inline bool types_equal(const Type &lhs, const Type &rhs);

inline bool operator==(const Type &lhs, const Type &rhs) {
  return types_equal(lhs, rhs);
}

inline bool types_equal(const Type &lhs, const Type &rhs) {
  if (lhs.index() != rhs.index()) {
    return false;
  }
  return std::visit(
      [&rhs](const auto &l) -> bool {
        using T = std::decay_t<decltype(l)>;
        const auto &r = std::get<T>(rhs);
        if constexpr (std::is_same_v<T, PrimitiveTypeValue>) {
          return l.m_type == r.m_type;
        } else if constexpr (std::is_same_v<T, std::unique_ptr<FunctionType>>) {
          if (l->m_argument_types.size() != r->m_argument_types.size()) {
            return false;
          }
          for (size_t i = 0; i < l->m_argument_types.size(); ++i) {
            if (l->m_argument_types[i] != r->m_argument_types[i]) {
              return false;
            }
          }
          return l->m_return_type == r->m_return_type;
        } else {
          // NeverType, UnknownType — same variant alternative means equal
          return true;
        }
      },
      lhs);
}

inline Type clone_type(const Type &type) {
  return std::visit(
      [](const auto &t) -> Type {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<FunctionType>>) {
          std::vector<Type> args;
          args.reserve(t->m_argument_types.size());
          for (const auto &arg : t->m_argument_types) {
            args.push_back(clone_type(arg));
          }
          return std::make_unique<FunctionType>(FunctionType{
              {t->m_location}, std::move(args), clone_type(t->m_return_type)});
        } else {
          return t;
        }
      },
      type);
}

inline std::string type_to_string(const Type &type) {
  return std::visit(
      [](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeValue>) {
          switch (t.m_type) {
          case PrimitiveType::UNIT:
            return "()";
          case PrimitiveType::BOOL:
            return "bool";
          case PrimitiveType::I64:
            return "i64";
          }
        } else if constexpr (std::is_same_v<T, std::unique_ptr<FunctionType>>) {
          std::string result = "fn(";
          for (size_t i = 0; i < t->m_argument_types.size(); ++i) {
            if (i > 0) {
              result += ", ";
            }
            result += type_to_string(t->m_argument_types[i]);
          }
          result += ") -> ";
          result += type_to_string(t->m_return_type);
          return result;
        } else if constexpr (std::is_same_v<T, NeverType>) {
          return "!";
        } else {
          return "?";
        }
      },
      type);
}

inline std::ostream &operator<<(std::ostream &out, const Type &type) {
  return out << type_to_string(type);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
