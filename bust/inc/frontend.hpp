//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Frontend entry point: lex + parse a bust source stream into
//*            an ast::Program with the compiler-shipped prelude prepended.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>

#include <iosfwd>

//****************************************************************************
namespace bust {
//****************************************************************************

ast::Program parse_program(std::istream &input, const char *filename);

//****************************************************************************
} // namespace bust
//****************************************************************************
