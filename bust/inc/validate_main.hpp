//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Semantic pass to validate the presence and signature of main.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Read-only pass: validates that the program contains exactly one `main`
/// function with return type `i64`. Passes the Program through unchanged,
/// or throws a CompilerException on failure.
struct ValidateMain {
  Program operator()(Program program) const;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
