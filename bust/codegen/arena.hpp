//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Arena-based type system for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include <arena.hpp>
#include <codegen/types.hpp>

#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct TypeArena : public AbstractInternArena<TypeId, LLVMType> {
  [[nodiscard]] std::string to_string(TypeId type_id) const override {
    return to_string(get(type_id));
  }

  [[nodiscard]] std::string to_string(const LLVMType &type) const override {
    return std::visit(
        [&](const auto &t) -> std::string {
          using T = std::decay_t<decltype(t)>;
          if constexpr (std::is_same_v<T, VoidType>) {
            return "void";
          } else if constexpr (std::is_same_v<T, I1Type>) {
            return "i1";
          } else if constexpr (std::is_same_v<T, I8Type>) {
            return "i8";
          } else if constexpr (std::is_same_v<T, I32Type>) {
            return "i32";
          } else if constexpr (std::is_same_v<T, I64Type>) {
            return "i64";
          } else if constexpr (std::is_same_v<T, PtrType>) {
            return "ptr";
          } else if constexpr (std::is_same_v<T, StructType>) {
            if (t.m_name.has_value()) {
              return "%" + t.m_name.value();
            }
            // Anonymous struct
            std::string out = "{ ";
            for (size_t index = 0; index < t.m_fields.size() - 1; ++index) {
              out += to_string(t.m_fields[index]);
              out += ", ";
            }
            out += to_string(t.m_fields.back());
            out += " }";
            return out;
          }
        },
        type);
  }

  [[nodiscard]] const StructType &as_struct(TypeId type_id) const {
    return as<StructType>(type_id);
  }

  [[nodiscard]] std::vector<StructType> named_struct_types() const {
    std::vector<StructType> struct_types;

    for (const auto &type : actual_types()) {
      if (!std::holds_alternative<StructType>(type)) {
        continue;
      }
      const auto &struct_type = std::get<StructType>(type);
      if (struct_type.m_name.has_value()) {
        struct_types.emplace_back(struct_type);
      }
    }

    return struct_types;
  }
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
