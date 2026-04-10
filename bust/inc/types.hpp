//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Shared type definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>

//****************************************************************************
namespace bust {
//****************************************************************************

enum class PrimitiveType : uint8_t {
  UNIT,
  BOOL,
  CHAR,
  I8,
  I32,
  I64,
};

template <PrimitiveType AbstractType> struct ToConcrete : std::false_type {};

template <> struct ToConcrete<PrimitiveType::BOOL> {
  using type = bool;
};

template <> struct ToConcrete<PrimitiveType::CHAR> {
  using type = char;
};

template <> struct ToConcrete<PrimitiveType::I8> {
  using type = int8_t;
};

template <> struct ToConcrete<PrimitiveType::I32> {
  using type = int32_t;
};

template <> struct ToConcrete<PrimitiveType::I64> {
  using type = int64_t;
};

inline std::string to_string(const PrimitiveType &type) {
  switch (type) {
  case PrimitiveType::UNIT:
    return "()";
  case PrimitiveType::BOOL:
    return "bool";
  case PrimitiveType::CHAR:
    return "char";
  case PrimitiveType::I8:
    return "i8";
  case PrimitiveType::I32:
    return "i32";
  case PrimitiveType::I64:
    return "i64";
  }
}

inline bool can_cast(PrimitiveType from, PrimitiveType to) {
  switch (from) {
  case PrimitiveType::UNIT: {
    // Can't cast Unit to or from anything
    return false;
  }
  case PrimitiveType::BOOL: {
    switch (to) {
    case PrimitiveType::BOOL:
    case PrimitiveType::I8:
    case PrimitiveType::I32:
    case PrimitiveType::I64:
      return true;
    case PrimitiveType::UNIT:
    case PrimitiveType::CHAR:
      return false;
    }
  }
  case PrimitiveType::CHAR:
  case PrimitiveType::I8:
  case PrimitiveType::I32:
  case PrimitiveType::I64:
    // Can always cast these 4 to one another, we allow narrowing and expanding
    switch (to) {
    case PrimitiveType::CHAR:
    case PrimitiveType::I8:
    case PrimitiveType::I32:
    case PrimitiveType::I64:
      return true;
    case PrimitiveType::BOOL:
    case PrimitiveType::UNIT:
      return false;
    }
  }
}

//****************************************************************************
} // namespace bust
//****************************************************************************
