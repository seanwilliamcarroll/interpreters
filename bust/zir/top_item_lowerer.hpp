//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item lowerer — HIR top items to ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <zir/arena.hpp>
#include <zir/context.hpp>
#include <zir/environment.hpp>
#include <zir/nodes.hpp>

#include <string>
#include <type_traits>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TopItemCollector {
  void collect(const hir::TopItem &top_item) {

    auto create_binding_and_define = [&](const std::string &name,
                                         hir::TypeId type_id) {
      auto new_type_id = m_ctx.convert(type_id);
      auto binding = Binding{.m_name = name, .m_type = new_type_id};
      auto binding_id = m_ctx.arena().push(std::move(binding));
      m_ctx.set_global_binding(name, binding_id);
      m_ctx.env().define(name, binding_id);
    };

    std::visit(
        [&](const auto &item) {
          using T = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<T, hir::FunctionDef> ||
                        std::is_same_v<T, hir::ExternFunctionDeclaration>) {
            create_binding_and_define(item.m_signature.m_function_id,
                                      item.m_signature.m_type);
          }
        },
        top_item);
  }

  Context &m_ctx;
};

struct TopItemLowerer {

  TopItem lower(const hir::TopItem &);

  TopItem operator()(const hir::FunctionDef &);
  TopItem operator()(const hir::ExternFunctionDeclaration &) const;

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
