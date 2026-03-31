//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Environment implementation for blip evaluation
//*
//*
//****************************************************************************

#include <environment.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

Environment::Environment(std::shared_ptr<Environment> parent)
    : m_parent(std::move(parent)) {}

void Environment::define(const std::string &name, Value value) {
  // TODO: bind name in current scope
  (void)name;
  (void)value;
}

Value Environment::lookup(const std::string &name) const {
  // TODO: search current scope, then walk parent chain
  // throw if not found
  (void)name;
  return Unit{};
}

void Environment::set(const std::string &name, Value value) {
  // TODO: find existing binding in chain and mutate it
  // throw if not found
  (void)name;
  (void)value;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
