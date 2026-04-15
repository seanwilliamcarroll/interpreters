//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the evaluator pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <eval/environment.hpp>
#include <hir/type_registry.hpp>
#include <hir/types.hpp>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct Context {
  const hir::FunctionType &as_function(hir::TypeId type_id) const {
    return m_type_registry.as_function(type_id);
  }

  const hir::PrimitiveTypeValue &as_primitive(hir::TypeId type_id) const {
    return m_type_registry.as_primitive(type_id);
  }

  const hir::TypeVariable &as_type_variable(hir::TypeId type_id) const {
    return m_type_registry.as_type_variable(type_id);
  }

  bool is_function(hir::TypeId type_id) const {
    return m_type_registry.is_function(type_id);
  }

  bool is_primitive(hir::TypeId type_id) const {
    return m_type_registry.is_primitive(type_id);
  }

  bool is_type_variable(hir::TypeId type_id) const {
    return m_type_registry.is_type_variable(type_id);
  }

  std::string to_string(const auto &type) const {
    return m_type_registry.to_string(type);
  }

  Environment m_env{};
  const hir::TypeRegistry &m_type_registry;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
