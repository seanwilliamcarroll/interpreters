//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Operator enum definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

//****************************************************************************
namespace bust {
//****************************************************************************

enum class UnaryOperator : uint8_t {
  // Logic/Bits
  NOT,
  // Arithmetic
  MINUS,
};

enum class BinaryOperator : uint8_t {
  // Logic/Bits
  LOGICAL_AND,
  LOGICAL_OR,
  // BITWISE_AND, // Maybe
  // BITWISE_OR, // maybe
  // Arithmetic
  PLUS,
  MINUS,
  MULTIPLIES,
  DIVIDES,
  MODULUS,
  // Assignment
  // ASSIGNS, // Maybe at some point?
  // Comparison
  EQ,
  LT,
  LT_EQ,
  GT,
  GT_EQ,
  NOT_EQ,
  // DOT, // TODO
};

inline const char *to_cstring(UnaryOperator op) {
  switch (op) {
  case UnaryOperator::NOT:
    return "!";
  case UnaryOperator::MINUS:
    return "-";
  }
}

inline const char *to_cstring(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LOGICAL_AND:
    return "&&";
  case BinaryOperator::LOGICAL_OR:
    return "||";
  case BinaryOperator::PLUS:
    return "+";
  case BinaryOperator::MINUS:
    return "-";
  case BinaryOperator::MULTIPLIES:
    return "*";
  case BinaryOperator::DIVIDES:
    return "/";
  case BinaryOperator::MODULUS:
    return "%";
  case BinaryOperator::EQ:
    return "==";
  case BinaryOperator::LT:
    return "<";
  case BinaryOperator::LT_EQ:
    return "<=";
  case BinaryOperator::GT:
    return ">";
  case BinaryOperator::GT_EQ:
    return ">=";
  case BinaryOperator::NOT_EQ:
    return "!=";
  }
}

inline std::string to_string(UnaryOperator op) { return to_cstring(op); }
inline std::string to_string(BinaryOperator op) { return to_cstring(op); }

inline std::ostream &operator<<(std::ostream &out, UnaryOperator op) {
  return out << to_cstring(op);
}

inline std::ostream &operator<<(std::ostream &out, BinaryOperator op) {
  return out << to_cstring(op);
}

inline std::string operator+(const std::string &lhs, UnaryOperator op) {
  return lhs + to_cstring(op);
}

inline std::string operator+(UnaryOperator op, const std::string &rhs) {
  return to_cstring(op) + rhs;
}

inline std::string operator+(const std::string &lhs, BinaryOperator op) {
  return lhs + to_cstring(op);
}

inline std::string operator+(BinaryOperator op, const std::string &rhs) {
  return to_cstring(op) + rhs;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
