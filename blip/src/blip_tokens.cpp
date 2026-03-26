//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::token_type_to_string implementation
//*
//*
//****************************************************************************

#include <string>

#include <blip_tokens.hpp>

#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::string token_type_to_string(BlipTokenType type) {
  switch (type) {
  case BlipToken::IF:
    return "IF";
  case BlipToken::WHILE:
    return "WHILE";
  case BlipToken::SET:
    return "SET";
  case BlipToken::BEGIN:
    return "BEGIN";
  case BlipToken::PRINT:
    return "PRINT";
  case BlipToken::DEFINE:
    return "DEFINE";
  default:
    return sc::token_type_to_string(type);
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
