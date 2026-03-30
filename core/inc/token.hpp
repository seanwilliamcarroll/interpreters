//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Token<TT> and TokenOf<TT, V> class templates
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ostream>
#include <string>

#include <source_location.hpp>

//****************************************************************************
namespace core {
//****************************************************************************

template <typename TT> class Token {
public:
  Token(const SourceLocation &loc, TT type) : m_loc(loc), m_type(type) {}

  virtual ~Token() = default;

  virtual std::ostream &dump(std::ostream &out) const {
    out << get_location()
        << " TokenType: " << static_cast<unsigned int>(get_token_type());
    return out;
  }

  const SourceLocation &get_location() const { return m_loc; }

  TT get_token_type() const { return m_type; }

private:
  const SourceLocation m_loc;
  const TT m_type;
};

template <typename TT, typename V> class TokenOf : public Token<TT> {
public:
  TokenOf(const SourceLocation &loc, TT type, const V &value)
      : Token<TT>(loc, type), m_value(value) {}

  std::ostream &dump(std::ostream &out) const override {
    return Token<TT>::dump(out) << " Value: " << get_value();
  }

  const V &get_value() const { return m_value; }

private:
  const V m_value;
};

template <typename TT>
inline std::ostream &operator<<(std::ostream &out, const Token<TT> &token) {
  return token.dump(out);
}

//****************************************************************************
} // namespace core
//****************************************************************************
