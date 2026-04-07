//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of let-binding evaluator.
//*
//*
//****************************************************************************

#include "eval/let_binding_evaluator.hpp"

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value LetBindingEvaluator::operator()(const hir::LetBinding &) { return {}; }

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
