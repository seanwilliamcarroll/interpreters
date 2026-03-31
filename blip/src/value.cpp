//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Runtime value implementation for blip evaluation
//*
//*
//****************************************************************************

#include <value.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::string value_to_string(const Value &value) {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
          return std::to_string(v);
        } else if constexpr (std::is_same_v<T, bool>) {
          return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
          return v;
        } else if constexpr (std::is_same_v<T, Unit>) {
          return "<unit>";
        } else if constexpr (std::is_same_v<T, Function>) {
          return "<function " + v.m_name + ">";
        } else if constexpr (std::is_same_v<T, BuiltInFunction>) {
          return "<builtinfunction " + v.m_name + ">";
        }
      },
      value);
}

//****************************************************************************
} // namespace blip
//****************************************************************************
