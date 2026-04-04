//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Operator enum definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstdint>

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

//****************************************************************************
} // namespace bust
//****************************************************************************
