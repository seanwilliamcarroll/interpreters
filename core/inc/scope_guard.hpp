//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : RAII guard for scoped environments.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <concepts>

//****************************************************************************
namespace core {
//****************************************************************************

template <typename T>
concept Scoped = requires(T t) {
  t.push_scope();
  t.pop_scope();
};

template <Scoped T> struct ScopeGuard {
  explicit ScopeGuard(T &scoped) : m_scoped(scoped) { m_scoped.push_scope(); }
  ~ScopeGuard() { m_scoped.pop_scope(); }

  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
  ScopeGuard(ScopeGuard &&) = delete;
  ScopeGuard &operator=(ScopeGuard &&) = delete;

  T &m_scoped;
};

//****************************************************************************
} // namespace core
//****************************************************************************
