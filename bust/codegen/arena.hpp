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
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <exceptions.hpp>

#include <optional>

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
            // Do all structs have names?
            auto struct_name = expect_get_struct_name(intern(t));
            return "%" + struct_name;
          }
        },
        type);
  }

  TypeId intern_global_struct(const std::string &name, StructType struct_type) {
    // Don't uniquify the name, trust it is correct
    auto type_id = intern(std::move(struct_type));
    if (!m_struct_names.emplace(type_id, name).second) {
      throw core::InternalCompilerError("Already interned global struct: " +
                                        name);
    }
    return type_id;
  }

  TypeId intern_struct(const std::string &name, StructType struct_type) {
    auto type_id = intern(std::move(struct_type));
    auto iter = m_struct_names.find(type_id);
    if (iter == m_struct_names.end()) {
      // Deduplicate
      m_struct_names.emplace(type_id, m_name_tracker.uniquify(name));
    }
    return type_id;
  }

  [[nodiscard]] std::optional<std::string>
  get_struct_name(TypeId type_id) const {
    auto iter = m_struct_names.find(type_id);
    if (iter == m_struct_names.end()) {
      return {};
    }
    return std::make_optional(iter->second);
  }

  [[nodiscard]] std::string expect_get_struct_name(TypeId type_id) const {
    auto struct_name = get_struct_name(type_id);
    if (!struct_name.has_value()) {
      throw core::InternalCompilerError(
          "Expected to find name for struct with type id " +
          std::to_string(type_id.m_id));
    }
    return struct_name.value();
  }

  [[nodiscard]]
  const std::unordered_map<TypeId, std::string> &struct_names() const {
    return m_struct_names;
  }

private:
  std::unordered_map<TypeId, std::string> m_struct_names;
  UniqueNameTracker m_name_tracker;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
