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

#include <functional>
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

struct TypeVariable : public HasLocation {
  // Not sure on needing a location
  size_t m_id;
};

struct FunctionType;

struct NeverType : public HasLocation {};

// TODO: User defined types of some kind
using Type = std::variant<PrimitiveTypeValue, TypeVariable,
                          std::unique_ptr<FunctionType>, NeverType>;

struct FunctionType : public HasLocation {
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
        } else if constexpr (std::is_same_v<T, TypeVariable>) {
          return l.m_id == r.m_id;
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
          // NeverType — same variant alternative means equal
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

struct TypeToStringConverter {
  std::string operator()(const PrimitiveTypeValue &type) {
    switch (type.m_type) {
    case PrimitiveType::UNIT:
      return "()";
    case PrimitiveType::BOOL:
      return "bool";
    case PrimitiveType::I64:
      return "i64";
    }
  }

  std::string operator()(const TypeVariable &type) {
    return "?T" + std::to_string(type.m_id);
  }

  std::string operator()(const std::unique_ptr<FunctionType> &type) {
    std::string result = "fn(";
    for (size_t i = 0; i < type->m_argument_types.size(); ++i) {
      if (i > 0) {
        result += ", ";
      }
      result += std::visit(*this, type->m_argument_types[i]);
    }
    result += ") -> ";
    result += std::visit(*this, type->m_return_type);
    return result;
  }

  std::string operator()(const NeverType &) { return "!"; }
};

inline std::string type_to_string(const Type &type) {
  return std::visit(TypeToStringConverter{}, type);
}

inline std::ostream &operator<<(std::ostream &out, const Type &type) {
  return out << type_to_string(type);
}

inline std::string operator+(const std::string &lhs, const Type &type) {
  return lhs + type_to_string(type);
}

inline std::string operator+(const Type &type, const std::string &rhs) {
  return type_to_string(type) + rhs;
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************

namespace std {

template <> struct hash<bust::hir::TypeVariable> {
  size_t operator()(const bust::hir::TypeVariable &type) const {
    return std::hash<size_t>{}(type.m_id);
  }
};

} // namespace std
