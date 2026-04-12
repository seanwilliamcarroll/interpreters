//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Interning arena for HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "exceptions.hpp"
#include <cstddef>
#include <hir/types.hpp>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeArena {
  TypeArena()
      : m_unit(intern(PrimitiveTypeValue{PrimitiveType::UNIT})),
        m_i8(intern(PrimitiveTypeValue{PrimitiveType::I8})),
        m_i32(intern(PrimitiveTypeValue{PrimitiveType::I32})),
        m_i64(intern(PrimitiveTypeValue{PrimitiveType::I64})),
        m_char(intern(PrimitiveTypeValue{PrimitiveType::CHAR})),
        m_bool(intern(PrimitiveTypeValue{PrimitiveType::BOOL})),
        m_never(intern(NeverType{})) {}

  // TypeId intern(const TypeKind& kind) const {
  //   if (auto iter = m_mapping.find(kind); iter != m_mapping.end()) {
  //     return iter->second;
  //   }
  //   throw std::runtime_error("Bad Lookup!");
  // }

  TypeId intern(const TypeKind &kind) {
    if (auto iter = m_mapping.find(kind); iter != m_mapping.end()) {
      return iter->second;
    }

    auto next_type_id = TypeId{m_types.size()};
    m_types.push_back(kind);
    m_mapping.emplace(m_types.back(), next_type_id);
    return next_type_id;
  }

  const TypeKind &get(TypeId id) const { return m_types[id.m_id]; }

  std::string to_string(const TypeKind &);
  std::string to_string(TypeId);

private:
  std::vector<TypeKind> m_types{};
  std::unordered_map<TypeKind, TypeId> m_mapping{};

public:
  const TypeId m_unit;
  const TypeId m_i8;
  const TypeId m_i32;
  const TypeId m_i64;
  const TypeId m_char;
  const TypeId m_bool;
  const TypeId m_never;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
