//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Interning arena for HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <arena.hpp>
#include <hir/types.hpp>
#include <types.hpp>

#include <string>
#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeArena : public AbstractInternArena<TypeId, TypeKind> {
  TypeArena()
      : m_unit(intern(PrimitiveTypeValue{PrimitiveType::UNIT})),
        m_i8(intern(PrimitiveTypeValue{PrimitiveType::I8})),
        m_i32(intern(PrimitiveTypeValue{PrimitiveType::I32})),
        m_i64(intern(PrimitiveTypeValue{PrimitiveType::I64})),
        m_char(intern(PrimitiveTypeValue{PrimitiveType::CHAR})),
        m_bool(intern(PrimitiveTypeValue{PrimitiveType::BOOL})),
        m_never(intern(NeverType{})) {}

  [[nodiscard]] std::string to_string(const TypeKind &type_kind) const override;
  [[nodiscard]] std::string to_string(TypeId type_id) const override;

  [[nodiscard]] const FunctionType &as_function(TypeId type_id) const {
    return as<FunctionType>(type_id);
  }

  [[nodiscard]] const TupleType &as_tuple(TypeId type_id) const {
    return as<TupleType>(type_id);
  }

  [[nodiscard]] const PrimitiveTypeValue &as_primitive(TypeId type_id) const {
    return as<PrimitiveTypeValue>(type_id);
  }

  [[nodiscard]] const TypeVariable &as_type_variable(TypeId type_id) const {
    return as<TypeVariable>(type_id);
  }

  [[nodiscard]] bool is_function(TypeId type_id) const {
    return is<FunctionType>(type_id);
  }

  [[nodiscard]] bool is_tuple(TypeId type_id) const {
    return is<TupleType>(type_id);
  }

  [[nodiscard]] bool is_primitive(TypeId type_id) const {
    return is<PrimitiveTypeValue>(type_id);
  }

  [[nodiscard]] bool is_type_variable(TypeId type_id) const {
    return is<TypeVariable>(type_id);
  }

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
