//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Instantiation records for monomorphization. Tracks each
//*            time a polymorphic let-bound lambda is instantiated along
//*            with the substitution from original free type variables to
//*            concrete types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/types.hpp>
#include <string>
#include <unordered_map>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

using TypeSubstitution = std::unordered_map<TypeId, TypeId>;

struct InstantiationRecord {
  // Helper to keep track of each time a polymorphic lambda is instantiated for
  // monomorphization later
  std::string m_let_binding;
  // Need to map the original free type variables to the new ones created for
  // this instantiation
  TypeSubstitution m_substitution;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
