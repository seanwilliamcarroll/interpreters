//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for Token and SourceLocation
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*            https://github.com/emil-e/rapidcheck
//*            for more on the 'RapidCheck' project.
//*
//****************************************************************************

#include <blip_tokens.hpp>
#include <source_location.hpp>

#include <doctest/doctest.h>
#include <rapidcheck.h>

//****************************************************************************
namespace rc {
//****************************************************************************

template <> struct Arbitrary<core::SourceLocation> {
  static Gen<core::SourceLocation> arbitrary() {
    return gen::construct<core::SourceLocation>(
        gen::map(gen::arbitrary<std::string>(),
                 [](const std::string &in_str) { return in_str.c_str(); }),
        gen::arbitrary<unsigned int>(), gen::arbitrary<unsigned int>());
  }
};

template <> struct Arbitrary<blip::TokenType> {
  static Gen<blip::TokenType> arbitrary() {
    return gen::map(
        gen::inRange<unsigned int>(
            0, static_cast<unsigned int>(blip::TokenType::DEFINE) + 1),
        [](unsigned int v) { return static_cast<blip::TokenType>(v); });
  }
};

template <> struct Arbitrary<blip::Token> {
  static Gen<blip::Token> arbitrary() {
    return gen::construct<blip::Token>(gen::arbitrary<core::SourceLocation>(),
                                       gen::arbitrary<blip::TokenType>());
  }
};

//****************************************************************************
} // namespace rc
//****************************************************************************

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.token") {
  TEST_CASE("token_equality") {
    rc::check("Given same args, Tokens should be identical",
              [](const core::SourceLocation &loc, TokenType type) {
                auto token_a = Token(loc, type);
                auto token_b = Token(loc, type);
                RC_ASSERT(token_a.get_token_type() == token_b.get_token_type());
                RC_ASSERT(token_a.get_location() == token_b.get_location());
              });
  }

  TEST_CASE("token_type_equality") {
    rc::check("type equality doesn't care about location",
              [](const core::SourceLocation &loc_a,
                 const core::SourceLocation &loc_b, TokenType type) {
                auto token_a = Token(loc_a, type);
                auto token_b = Token(loc_b, type);
                RC_ASSERT(token_a.get_token_type() == token_b.get_token_type());
              });
  }
}
//****************************************************************************
} // namespace blip
//****************************************************************************
