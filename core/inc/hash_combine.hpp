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

template <typename T> inline void hash_combine(std::size_t &seed, const T &v) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

//****************************************************************************
} // namespace core
//****************************************************************************
