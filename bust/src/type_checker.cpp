//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the TypeChecker pass.
//*
//*
//****************************************************************************

#include <type_checker.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

Program TypeChecker::operator()(Program program) const {
  // TODO: type check all top-level items
  return program;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
