//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the ZIR type arena.
//*
//*
//****************************************************************************

#include "exceptions.hpp"
#include <zir/arena.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

TypeId TypeArena::intern(const Type &type) const {
  auto iter = m_type_to_type_id.find(type);
  if (iter == m_type_to_type_id.end()) {
    throw core::InternalCompilerError(
        "Couldn't find type through const lookup!");
  }
  return iter->second;
}

TypeId TypeArena::intern(const Type &type) {
  auto iter = m_type_to_type_id.find(type);
  if (iter == m_type_to_type_id.end()) {
    auto new_id = m_type_id_to_type.size();
    m_type_to_type_id.emplace(type, new_id);
    m_type_id_to_type.emplace_back(type);
    return {new_id};
  }
  return iter->second;
}

const Type &TypeArena::get(TypeId type_id) const {
  if (type_id.m_id >= m_type_id_to_type.size()) {
    throw core::InternalCompilerError("Failed to call get on type_id: " +
                                      std::to_string(type_id.m_id));
  }
  return m_type_id_to_type[type_id.m_id];
}

TypeId TypeArena::convert(hir::TypeId type_id, const hir::TypeRegistry &reg) {
  return convert(reg.get(type_id), reg);
}

TypeId TypeArena::convert(const hir::TypeKind &type_kind,
                          const hir::TypeRegistry &reg) {
  // Translate concrete hir TypeKind into zir Type and intern
  auto new_type = std::visit(
      [&](const auto &tk) -> Type {
        using T = std::decay_t<decltype(tk)>;
        if constexpr (std::is_same_v<T, hir::PrimitiveTypeValue>) {
          switch (tk.m_type) {
          case PrimitiveType::UNIT:
            return UnitType{};
          case PrimitiveType::BOOL:
            return BoolType{};
          case PrimitiveType::CHAR:
            return CharType{};
          case PrimitiveType::I8:
            return I8Type{};
          case PrimitiveType::I32:
            return I32Type{};
          case PrimitiveType::I64:
            return I64Type{};
          }
        } else if constexpr (std::is_same_v<T, hir::FunctionType>) {
          std::vector<TypeId> parameters;
          parameters.reserve(tk.m_parameters.size());
          for (const auto &parameter : tk.m_parameters) {
            parameters.emplace_back(convert(parameter, reg));
          }
          auto return_type = convert(tk.m_return_type, reg);
          return FunctionType{.m_parameters = std::move(parameters),
                              .m_return_type = return_type};
        } else if constexpr (std::is_same_v<T, hir::NeverType>) {
          return NeverType{};
        } else {
          // TypeVariable shouldn't be passed here?
          std::unreachable();
        }
      },
      type_kind);

  return intern(new_type);
}

std::string TypeArena::to_string(TypeId) const { return {}; }

std::string TypeArena::to_string(const Type &) const { return {}; }

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
