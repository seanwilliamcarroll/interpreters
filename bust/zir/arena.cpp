//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the ZIR type arena.
//*
//*
//****************************************************************************

#include <zir/arena.hpp>
#include <zir/types.hpp>

#include <cstddef>
#include <type_traits>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

std::string TypeArena::to_string(TypeId type_id) const {
  return to_string(get(type_id));
}

std::string TypeArena::to_string(const Type &type) const {
  return std::visit(
      [this](const auto &t) -> std::string {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, UnitType>) {
          return "()";
        } else if constexpr (std::is_same_v<T, BoolType>) {
          return "bool";
        } else if constexpr (std::is_same_v<T, CharType>) {
          return "char";
        } else if constexpr (std::is_same_v<T, I8Type>) {
          return "i8";
        } else if constexpr (std::is_same_v<T, I32Type>) {
          return "i32";
        } else if constexpr (std::is_same_v<T, I64Type>) {
          return "i64";
        } else if constexpr (std::is_same_v<T, NeverType>) {
          return "!";
        } else if constexpr (std::is_same_v<T, FunctionType>) {
          std::string result = "fn(";
          for (size_t i = 0; i < t.m_parameters.size(); ++i) {
            if (i > 0) {
              result += ", ";
            }
            result += to_string(t.m_parameters[i]);
          }
          result += ") -> ";
          result += to_string(t.m_return_type);
          return result;
        } else if constexpr (std::is_same_v<T, TupleType>) {
          std::string out = "(";
          for (size_t index = 0; index < t.m_fields.size() - 1; ++index) {
            out += to_string(get(t.m_fields[index]));
            out += ", ";
          }
          out += to_string(get(t.m_fields.back()));
          out += ",)";
          return out;
        }
      },
      type);
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
