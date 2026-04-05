//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Generic pass pipeline for chaining AST transformations.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <type_traits>
#include <utility>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Runs a sequence of passes, threading the output of each into the next.
///
/// A read-only pass takes and returns the same type (pass-through).
/// A transform pass takes one type and returns a different type.
/// If any pass throws, the pipeline stops and the exception propagates.

template <typename AST, typename Pass>
auto run_pipeline(AST &&ast, Pass &&pass) {
  return pass(std::forward<AST>(ast));
}

template <typename AST, typename Pass, typename... Rest>
auto run_pipeline(AST &&ast, Pass &&pass, Rest &&...rest) {
  return run_pipeline(pass(std::forward<AST>(ast)),
                      std::forward<Rest>(rest)...);
}

//****************************************************************************
} // namespace bust
//****************************************************************************
