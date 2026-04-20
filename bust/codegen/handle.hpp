//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Handle types for codegen SSA values and named storage.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct LiteralHandle {
  std::string m_handle;

  static LiteralHandle zero() { return {"0"}; }
  static LiteralHandle one() { return {"1"}; }
  static LiteralHandle null() { return {"null"}; }
};

struct TemporaryHandle {
  static size_t m_unique_id;
  size_t m_id = m_unique_id++;
};

struct LocalHandle {
  std::string m_handle;
};

struct ParameterHandle {
  std::string m_handle;
};

struct GlobalHandle {
  std::string m_handle;
};

using Handle = std::variant<LiteralHandle, TemporaryHandle, LocalHandle,
                            ParameterHandle, GlobalHandle>;

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
