//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type definitions for bust HIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <source_location.hpp>
#include <types.hpp>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
using bust::PrimitiveType;
using core::HasLocation;
//****************************************************************************

struct PrimitiveTypeValue : public HasLocation {
  PrimitiveType m_type;
};

struct FunctionType;

struct NeverType : public HasLocation {};

struct UnknownType : public HasLocation {};

// TODO: Do we want to have an InferredType and ExplicitType, where Explicit has
// a location?
// TODO: Unknown type?
// TODO: User defined types of some kind
using Type = std::variant<PrimitiveTypeValue, std::unique_ptr<FunctionType>,
                          NeverType, UnknownType>;

struct FunctionType : public HasLocation {
  // I think this should have a location, inferred types may not
  std::vector<Type> m_argument_types;
  Type m_return_type;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
