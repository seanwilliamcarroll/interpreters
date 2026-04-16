//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the HIR type interning registry.
//*
//*
//****************************************************************************

#include "hir/types.hpp"
#include <hir/type_registry.hpp>
#include <string>
#include <types.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

std::string TypeRegistry::to_string(const TypeKind &type_kind) const {
  return std::visit(
      [&](const auto &tk) -> std::string {
        using T = std::decay_t<decltype(tk)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeValue>) {
          const PrimitiveType primitive_type = tk.m_type;
          return bust::to_string(primitive_type);
        } else if constexpr (std::is_same_v<T, TypeVariable>) {
          return "?T<" + std::to_string(tk.m_id) + ">";
        } else if constexpr (std::is_same_v<T, FunctionType>) {
          std::string out = "fn (";
          if (!tk.m_parameters.empty()) {
            out += "(";
            for (size_t index = 0; index < tk.m_parameters.size() - 1;
                 ++index) {
              out += to_string(get(tk.m_parameters[index]));
              out += ", ";
            }
            out += to_string(get(tk.m_parameters.back()));
            out += ")";
          }
          out += " -> ";
          out += to_string(get(tk.m_return_type));
          out += ")";
          return out;
        } else if constexpr (std::is_same_v<T, NeverType>) {
          return "!";
        }
      },
      type_kind);
}

std::string TypeRegistry::to_string(TypeId type_id) const {
  return to_string(m_types.at(type_id.m_id));
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
