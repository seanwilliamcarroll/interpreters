//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Instruction and terminator types for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/types.hpp"
#include <string>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct LiteralHandle {
  std::string m_handle;
};

struct TemporaryHandle {
  static size_t m_unique_id;
  size_t m_id = m_unique_id++;
};

struct LocalHandle {
  std::string m_handle;
};

struct GlobalHandle {
  std::string m_handle;
};

using Handle =
    std::variant<LiteralHandle, TemporaryHandle, LocalHandle, GlobalHandle>;

// inline std::string handle_to_string(const Handle &handle) {
//   return std::visit(
//       [](const auto &handle) -> std::string {
//         using T = std::decay_t<decltype(handle)>;
//         if constexpr (std::is_same_v<T, LiteralHandle>) {
//           return handle.m_handle;
//         } else if constexpr (std::is_same_v<T, TemporaryHandle>) {
//           return "%" + std::to_string(handle.m_id);
//         } else if constexpr (std::is_same_v<T, LocalHandle>) {
//           return "%" + handle.m_handle;
//         } else if constexpr (std::is_same_v<T, GlobalHandle>) {
//           return "@" + handle.m_handle;
//         }
//       },
//       handle);
// }

inline std::string get_raw_handle(const Handle &handle) {
  return std::visit(
      [](const auto &handle) -> std::string {
        using T = std::decay_t<decltype(handle)>;
        if constexpr (std::is_same_v<T, TemporaryHandle>) {
          return std::to_string(handle.m_id);
        } else {
          return handle.m_handle;
        }
      },
      handle);
}

// inline std::ostream &operator<<(std::ostream &out, const Handle &handle) {
//   return out << handle_to_string(handle);
// }

struct BinaryInstruction {
  Handle m_result;
  Handle m_lhs;
  Handle m_rhs;
  LLVMBinaryOperator m_operator;
  LLVMType m_type;
};

struct BranchInstruction {
  Handle m_condition;
  Handle m_iftrue;
  Handle m_iffalse;
};

struct JumpInstruction {
  Handle m_target;
};

struct LoadInstruction {
  Handle m_destination;
  Handle m_source;
  LLVMType m_type;
};

struct StoreInstruction {
  Handle m_destination;
  Handle m_source;
  LLVMType m_type;
};

struct AllocaInstruction {
  Handle m_handle;
  LLVMType m_type;
};

struct ReturnInstruction {
  Handle m_value;
  LLVMType m_type;
};

using Instruction = std::variant<BinaryInstruction, LoadInstruction,
                                 StoreInstruction, AllocaInstruction>;

using Terminator =
    std::variant<BranchInstruction, JumpInstruction, ReturnInstruction>;

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
