//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type-related AST definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <source_location.hpp>
#include <string>
#include <types.hpp>
#include <variant>

//****************************************************************************
namespace bust::ast {
//****************************************************************************

using bust::PrimitiveType;
using core::HasLocation;

struct PrimitiveTypeIdentifier : public HasLocation {
  PrimitiveType m_type;
};

struct DefinedType : public HasLocation {
  std::string m_type;
};

using TypeIdentifier = std::variant<PrimitiveTypeIdentifier, DefinedType>;

inline std::string type_identifier_to_string(const TypeIdentifier &tid) {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeIdentifier>) {
          switch (v.m_type) {
          case PrimitiveType::UNIT:
            return "()";
          case PrimitiveType::BOOL:
            return "bool";
          case PrimitiveType::I64:
            return "i64";
          }
        } else {
          return v.m_type;
        }
      },
      tid);
}

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
