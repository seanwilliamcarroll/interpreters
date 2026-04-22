//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type-related AST definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <source_location.hpp>
#include <types.hpp>

#include <memory>
#include <string>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::ast {
//****************************************************************************

struct PrimitiveTypeIdentifier : public core::HasLocation {
  PrimitiveType m_type;
};

struct DefinedType : public core::HasLocation {
  std::string m_type;
};

struct FunctionTypeIdentifier;

struct TupleTypeIdentifier;

using TypeIdentifier = std::variant<PrimitiveTypeIdentifier, DefinedType,
                                    std::unique_ptr<TupleTypeIdentifier>,
                                    std::unique_ptr<FunctionTypeIdentifier>>;

struct FunctionTypeIdentifier : public core::HasLocation {
  std::vector<TypeIdentifier> m_parameter_types;
  TypeIdentifier m_return_type;
};

struct TupleTypeIdentifier : public core::HasLocation {
  std::vector<TypeIdentifier> m_field_types;
};

inline TypeIdentifier clone_type_identifier(const TypeIdentifier &tid) {
  return std::visit(
      [](const auto &v) -> TypeIdentifier {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T,
                                     std::unique_ptr<FunctionTypeIdentifier>>) {
          std::vector<TypeIdentifier> params;
          params.reserve(v->m_parameter_types.size());
          for (const auto &p : v->m_parameter_types) {
            params.push_back(clone_type_identifier(p));
          }
          return std::make_unique<FunctionTypeIdentifier>(
              FunctionTypeIdentifier{{v->m_location},
                                     std::move(params),
                                     clone_type_identifier(v->m_return_type)});
        } else if constexpr (std::is_same_v<
                                 T, std::unique_ptr<TupleTypeIdentifier>>) {
          std::vector<TypeIdentifier> field_types;
          field_types.reserve(v->m_field_types.size());
          for (const auto &type : v->m_field_types) {
            field_types.emplace_back(clone_type_identifier(type));
          }
          return std::make_unique<TupleTypeIdentifier>(TupleTypeIdentifier{
              .m_field_types = std::move(field_types),
          });
        } else {
          return v;
        }
      },
      tid);
}

inline std::string type_identifier_to_string(const TypeIdentifier &tid) {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeIdentifier>) {
          return to_string(v.m_type);
        } else if constexpr (std::is_same_v<
                                 T, std::unique_ptr<FunctionTypeIdentifier>>) {
          std::string result = "fn(";
          for (size_t i = 0; i < v->m_parameter_types.size(); ++i) {
            if (i > 0) {
              result += ", ";
            }
            result += type_identifier_to_string(v->m_parameter_types[i]);
          }
          result += ") -> ";
          result += type_identifier_to_string(v->m_return_type);
          return result;
        } else if constexpr (std::is_same_v<
                                 T, std::unique_ptr<TupleTypeIdentifier>>) {
          std::string result = "(";
          for (size_t i = 0; i < v->m_field_types.size(); ++i) {
            if (i > 0) {
              result += " ";
            }
            result += type_identifier_to_string(v->m_field_types[i]);
            result += ",";
          }
          result += ")";
          return result;
        } else {
          return v.m_type;
        }
      },
      tid);
}

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
