//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Interning registry for HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "exceptions.hpp"
#include <cstddef>
#include <hir/types.hpp>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeRegistry {
  TypeRegistry()
      : m_unit(intern(PrimitiveTypeValue{PrimitiveType::UNIT})),
        m_i8(intern(PrimitiveTypeValue{PrimitiveType::I8})),
        m_i32(intern(PrimitiveTypeValue{PrimitiveType::I32})),
        m_i64(intern(PrimitiveTypeValue{PrimitiveType::I64})),
        m_char(intern(PrimitiveTypeValue{PrimitiveType::CHAR})),
        m_bool(intern(PrimitiveTypeValue{PrimitiveType::BOOL})),
        m_never(intern(NeverType{})) {}

  TypeId intern(const TypeKind &kind) {
    if (auto iter = m_mapping.find(kind); iter != m_mapping.end()) {
      return iter->second;
    }

    auto next_type_id = TypeId{m_types.size()};
    m_types.push_back(kind);
    m_mapping.emplace(m_types.back(), next_type_id);
    return next_type_id;
  }

  const TypeKind &get(TypeId id) const {
    if (id.m_id >= m_types.size()) {
      throw core::InternalCompilerError(
          "TypeRegistry::get() out of bounds: id " + std::to_string(id.m_id) +
          ", size " + std::to_string(m_types.size()));
    }
    return m_types[id.m_id];
  }

  std::string to_string(const TypeKind &) const;
  std::string to_string(TypeId) const;

  template <typename VariantType>
  const VariantType &as(TypeId type_id, const char *function) const {
    const auto &type_kind = get(type_id);
    if (!std::holds_alternative<VariantType>(type_kind)) {
      throw core::InternalCompilerError(std::string(function) +
                                        "Bad access to registry with " +
                                        to_string(type_id));
    }
    return std::get<VariantType>(type_kind);
  }

  const FunctionType &as_function(TypeId type_id) const {
    return as<FunctionType>(type_id, __PRETTY_FUNCTION__);
  }

  const PrimitiveTypeValue &as_primitive(TypeId type_id) const {
    return as<PrimitiveTypeValue>(type_id, __PRETTY_FUNCTION__);
  }

  const TypeVariable &as_type_variable(TypeId type_id) const {
    return as<TypeVariable>(type_id, __PRETTY_FUNCTION__);
  }

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
