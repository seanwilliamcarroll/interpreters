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
#include <zir/context.hpp>
#include <zir/nodes.hpp>

#include <variant>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TopItemCollector {
  void collect(const hir::TopItem &top_item) {
    std::visit(
        [&](const auto &item) {
          using T = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<T, hir::LetBinding>) {
            auto new_type_id = m_ctx.convert(item.m_expression.m_type);
            auto binding = Binding{.m_name = item.m_variable.m_name,
                                   .m_type = new_type_id};
            auto binding_id = m_ctx.m_arena.push(std::move(binding));
            m_ctx.m_global_bindings[item.m_variable.m_name] = binding_id;
            m_ctx.m_env.define(item.m_variable.m_name, binding_id);
          } else if constexpr (std::is_same_v<T, hir::FunctionDef>) {
            auto new_type_id = m_ctx.convert(item.m_signature.m_type);
            auto binding = Binding{.m_name = item.m_signature.m_function_id,
                                   .m_type = new_type_id};
            auto binding_id = m_ctx.m_arena.push(std::move(binding));
            m_ctx.m_global_bindings[item.m_signature.m_function_id] =
                binding_id;
            m_ctx.m_env.define(item.m_signature.m_function_id, binding_id);
          } else if constexpr (std::is_same_v<T,
                                              hir::ExternFunctionDeclaration>) {
            auto new_type_id = m_ctx.convert(item.m_signature.m_type);
            auto binding = Binding{.m_name = item.m_signature.m_function_id,
                                   .m_type = new_type_id};
            auto binding_id = m_ctx.m_arena.push(std::move(binding));
            m_ctx.m_global_bindings[item.m_signature.m_function_id] =
                binding_id;
            m_ctx.m_env.define(item.m_signature.m_function_id, binding_id);
          }
        },
        top_item);
  }

  Context &m_ctx;
};

struct TopItemLowerer {

  TopItem lower(const hir::TopItem &);

  TopItem operator()(const hir::FunctionDef &);
  TopItem operator()(const hir::ExternFunctionDeclaration &);
  TopItem operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
