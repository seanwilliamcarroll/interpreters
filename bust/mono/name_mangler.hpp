//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Produces unique mangled names for monomorphized lambda
//*            specializations. Given a base name and a concrete type
//*            substitution, emits an identifier valid for codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <types.hpp>

#include <sstream>
#include <string>
#include <variant>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct Mangler {

  explicit Mangler(hir::TypeArena &type_arena) : m_type_arena(type_arena) {}

  // Entry point
  std::string mangle(const std::string &original_identifier,
                     const hir::BindingId &id, const hir::TypeId &type_id) {
    m_out.str("");
    m_out << original_identifier << "__bi" << std::to_string(id.m_id) << "_";
    mangle(type_id);
    return m_out.str();
  }

  void mangle(const hir::TypeId &type_id) {
    std::visit(
        [&](const auto &type) {
          using T = std::decay_t<decltype(type)>;
          if constexpr (std::is_same_v<T, hir::PrimitiveTypeValue>) {
            m_out << bust::to_sanitized_string(type.m_type);
          } else if constexpr (std::is_same_v<T, hir::FunctionType>) {
            m_out << "fn_p_";
            if (!type.m_parameters.empty()) {
              for (size_t index = 0; index < type.m_parameters.size() - 1;
                   ++index) {
                const auto &parameter = type.m_parameters[index];
                mangle(parameter);
                m_out << "_";
              }
              mangle(type.m_parameters.back());
            }

            m_out << "_p_ret_";
            mangle(type.m_return_type);
          } else if constexpr (std::is_same_v<T, hir::TupleType>) {
            m_out << "tup_p_";
            for (size_t index = 0; index < type.m_fields.size() - 1; ++index) {
              const auto &field = type.m_fields[index];
              mangle(field);
              m_out << "_";
            }
            mangle(type.m_fields.back());
            m_out << "_p";
          } else {
            throw core::InternalCompilerError(
                "Cannot mangle names with NeverType or TypeVariables");
          }
        },
        m_type_arena.get(type_id));
  }

  const hir::TypeArena &m_type_arena;
  std::stringstream m_out;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
