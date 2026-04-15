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
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

using InnerTypeBindingId = size_t;

struct BindingId {
  InnerTypeBindingId m_id;
  auto operator<=>(const BindingId &) const = default;
};

using TypeSubstitution = std::unordered_map<TypeId, TypeId>;

struct InstantiationRecord {
  // Need to map the original free type variables to the new ones created for
  // this instantiation
  TypeSubstitution m_substitution;
};

using InstantiationRecords = std::vector<InstantiationRecord>;

using BindingIdInstantiations =
    std::unordered_map<BindingId, InstantiationRecords>;

//****************************************************************************
} // namespace bust::hir
//****************************************************************************

//****************************************************************************
namespace std {
//****************************************************************************

template <> struct hash<bust::hir::BindingId> {
  size_t operator()(const bust::hir::BindingId &id) const {
    return std::hash<bust::hir::InnerTypeBindingId>{}(id.m_id);
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
