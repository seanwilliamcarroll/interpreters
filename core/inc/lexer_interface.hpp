//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Purpose : LexerInterface<TT> abstract interface
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include <token.hpp>

//****************************************************************************
namespace core {
//****************************************************************************

template <typename TT> struct LexerInterface {
  virtual ~LexerInterface() = default;
  virtual std::unique_ptr<Token<TT>> get_next_token() = 0;
};

//****************************************************************************
} // namespace core
//****************************************************************************
