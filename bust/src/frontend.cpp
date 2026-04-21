//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the frontend entry point.
//*
//*
//****************************************************************************

#include <frontend.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <prelude_source.hpp>

#include <iterator>
#include <sstream>
#include <string>

//****************************************************************************
namespace bust {
//****************************************************************************

namespace {

ast::Program parse_one(std::istream &input, const char *filename) {
  auto lexer = make_lexer(input, filename);
  Parser parser(std::move(lexer));
  return parser.parse();
}

} // namespace

ast::Program parse_program(std::istream &input, const char *filename) {
  std::istringstream prelude_stream{std::string(prelude_source)};
  auto prelude_program = parse_one(prelude_stream, "<prelude>");
  auto user_program = parse_one(input, filename);
  user_program.m_items.insert(
      user_program.m_items.begin(),
      std::make_move_iterator(prelude_program.m_items.begin()),
      std::make_move_iterator(prelude_program.m_items.end()));
  return user_program;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
