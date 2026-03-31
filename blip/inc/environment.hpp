//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Lexically-scoped environment for blip evaluation
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <string>
#include <unordered_map>

#include <value.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Environment : public std::enable_shared_from_this<Environment> {
public:
  explicit Environment(std::shared_ptr<Environment> parent = nullptr);

  void define(const std::string &name, Value value);

  Value lookup(const std::string &name) const;

  void set(const std::string &name, Value value);

private:
  std::unordered_map<std::string, Value> m_bindings;
  std::shared_ptr<Environment> m_parent;
};

std::shared_ptr<Environment> default_global_environment();

//****************************************************************************
} // namespace blip
//****************************************************************************
