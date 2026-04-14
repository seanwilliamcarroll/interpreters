//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared type definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstdint>
#include <string>
#include <type_traits>

//****************************************************************************
namespace bust {
//****************************************************************************

enum class PrimitiveTypeClass : uint8_t {
  NUMERIC,
  COMPARABLE,
  BOOL,
};

enum class PrimitiveType : uint8_t {
  UNIT,
  BOOL,
  CHAR,
  I8,
  I32,
  I64,
};

inline bool is_type_in_type_class(PrimitiveTypeClass type_class,
                                  PrimitiveType type) {
  switch (type_class) {
  case PrimitiveTypeClass::BOOL:
    return type == PrimitiveType::BOOL;
  case PrimitiveTypeClass::NUMERIC:
    return type == PrimitiveType::I8 || type == PrimitiveType::I32 ||
           type == PrimitiveType::I64;
  case PrimitiveTypeClass::COMPARABLE:
    return type != PrimitiveType::UNIT;
  }
}

inline bool resolves(PrimitiveTypeClass sub_class,
                     PrimitiveTypeClass super_class) {
  switch (super_class) {
  case PrimitiveTypeClass::BOOL:
    // Bool is disjoint from Numeric, and is not a superset of Comparable
    return sub_class == PrimitiveTypeClass::BOOL;
  case PrimitiveTypeClass::NUMERIC:
    // Numeric is disjoint from Bool, and is not a superset of Comparable
    return sub_class == PrimitiveTypeClass::NUMERIC;
  case PrimitiveTypeClass::COMPARABLE:
    // Comparable is a superset of Bool and Numeric
    return true;
  }
}

template <PrimitiveType AbstractType> struct ToConcrete : std::false_type {};

template <> struct ToConcrete<PrimitiveType::BOOL> {
  using type = bool;
  constexpr static PrimitiveTypeClass type_class = PrimitiveTypeClass::BOOL;
};

template <> struct ToConcrete<PrimitiveType::CHAR> {
  using type = char;
  constexpr static PrimitiveTypeClass type_class =
      PrimitiveTypeClass::COMPARABLE;
};

template <> struct ToConcrete<PrimitiveType::I8> {
  using type = int8_t;
  constexpr static PrimitiveTypeClass type_class = PrimitiveTypeClass::NUMERIC;
};

template <> struct ToConcrete<PrimitiveType::I32> {
  using type = int32_t;
  constexpr static PrimitiveTypeClass type_class = PrimitiveTypeClass::NUMERIC;
};

template <> struct ToConcrete<PrimitiveType::I64> {
  using type = int64_t;
  constexpr static PrimitiveTypeClass type_class = PrimitiveTypeClass::NUMERIC;
};

inline std::string to_string(const PrimitiveTypeClass &type_class) {
  switch (type_class) {
  case PrimitiveTypeClass::BOOL:
    return "Boolean";
  case PrimitiveTypeClass::NUMERIC:
    return "Numeric";
  case PrimitiveTypeClass::COMPARABLE:
    return "Comparable";
  }
}

inline std::string operator+(const std::string &lhs, PrimitiveTypeClass rhs) {
  return lhs + to_string(rhs);
}

inline std::string operator+(PrimitiveTypeClass lhs, const std::string &rhs) {
  return to_string(lhs) + rhs;
}

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

inline std::string to_sanitized_string(const PrimitiveType &type) {
  switch (type) {
  case PrimitiveType::UNIT:
    return "unit";
  default:
    return to_string(type);
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
