//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Handle types for codegen SSA values and named storage.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/ir_literals.hpp>

#include <string>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct LiteralHandle {
  std::string m_handle;

  static LiteralHandle zero() { return {std::string{ir_literals::zero}}; }
  static LiteralHandle one() { return {std::string{ir_literals::one}}; }
  static LiteralHandle null() { return {std::string{ir_literals::null}}; }
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

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
