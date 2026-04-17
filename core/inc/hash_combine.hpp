//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Generic hash combiner for building composite std::hash
//*            specializations.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstddef>
#include <functional>

//****************************************************************************
namespace core {
//****************************************************************************

// I don't like magic numbers, but these literally are magic numbers
constexpr size_t RECIPROCAL_OF_GOLDEN_RATIO = 0x9e3779b9;
constexpr size_t SIX = 6;
constexpr size_t TWO = 2;

template <typename T> inline void hash_combine(std::size_t &seed, const T &v) {
  seed ^= std::hash<T>{}(v) + RECIPROCAL_OF_GOLDEN_RATIO + (seed << SIX) +
          (seed >> TWO);
}

//****************************************************************************
} // namespace core
//****************************************************************************
