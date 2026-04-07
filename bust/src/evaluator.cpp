//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Tree-walking evaluator implementation.
//*
//*
//****************************************************************************

#include <evaluator.hpp>

#include "eval/context.hpp"
#include "eval/values.hpp"

//****************************************************************************
namespace bust {
//****************************************************************************

int64_t Evaluator::operator()(const hir::Program & /*program*/) {

  auto context = eval::Context{};

  // Essentially go through program and load functions into env

  // Then "call" main() with the environment?

  return 0;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
