//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the ZirLowerer pipeline pass.
//*
//*
//****************************************************************************

#include <zir_lowerer.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

zir::Program ZirLowerer::operator()(hir::Program program) {

  auto type_registry = std::move(program.m_type_registry);
  (void)type_registry;

  auto unifier_state = std::move(program.m_unifier_state);
  (void)unifier_state;

  (void)program;
  return {};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
