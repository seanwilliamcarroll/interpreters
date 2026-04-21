//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Moveable state extracted from TypeUnifier, stored in
//*            hir::Program so downstream passes (ZIR lowering) can resolve
//*            type variables without needing the full unifier.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/types.hpp>
#include <hir/union_find.hpp>
#include <types.hpp>

#include <unordered_map>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct UnifierState {
  UnionFind m_union_find;
  std::unordered_map<size_t, TypeId> m_resolved_type_id;
  std::unordered_map<size_t, PrimitiveTypeClass> m_resolved_type_class;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
