//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Runtime environment for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
#include "hir/nodes.hpp"
#include <optional>
#include <string>
namespace bust::eval {
//****************************************************************************

struct Environment {

  std::optional<hir::Expression> lookup(const std::string &name) { return {}; }

  void define(const std::string &name, hir::Expression expression) {}
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
