//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Symbol table for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cassert>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

// struct SsaTemporary {
//   static SsaTemporary next() {
//     static size_t count = 1;
//     return {.m_name="%" + std::to_string(count++)};
//   }

//   std::string m_name;
// };

// struct Pointer {
//   std::string m_name;
// };

// using Handle = std::variant<SsaTemporary, Pointer>;

using Handle = std::string;

struct Scope {

  void define(const std::string &name, const Handle &handle) {
    m_symbol_to_handle[name] = handle;
  }

  std::optional<Handle> lookup(const std::string &name) const {
    auto iter = m_symbol_to_handle.find(name);

    if (iter == m_symbol_to_handle.end()) {
      return {};
    }

    return {iter->second};
  }

private:
  std::unordered_map<std::string, Handle> m_symbol_to_handle;
};

struct SymbolTable {
  SymbolTable() : m_scopes{{}} {}

  void push_scope() { m_scopes.emplace_back(); }

  void pop_scope() {
    assert((m_scopes.size() > 1) &&
           "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  Handle define(const std::string &name) {
    auto count = ++m_master_name_count[name];
    auto new_handle = "%" + name + "." + std::to_string(count);

    m_scopes.back().define(name, new_handle);

    return new_handle;
  }

  Handle lookup(const std::string &name) const {
    for (const auto &scope : m_scopes | std::views::reverse) {
      auto maybe_handle = scope.lookup(name);
      if (maybe_handle.has_value()) {
        return maybe_handle.value();
      }
    }
    assert(false && "Shouldn't happen");
    return "";
  }

  static Handle next_ssa_temporary() {
    static size_t count = 1;
    return "%" + std::to_string(count++);
  }

private:
  std::vector<Scope> m_scopes;
  std::unordered_map<std::string, size_t> m_master_name_count;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
