//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared arena templates for interning and append-only storage.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <variant>

//****************************************************************************
namespace bust {
//****************************************************************************

// TODO: Concept to enforce Variant-ness?

template <typename Type>
concept Hashable = requires(Type input) {
  { std::hash<Type>{}(input) } -> std::convertible_to<std::size_t>;
};

template <typename Type>
concept HasId = requires(Type input) {
  { input.m_id } -> std::convertible_to<std::size_t>;
};

template <typename Type>
concept HashableAndHasId = Hashable<Type> && HasId<Type>;

template <HashableAndHasId IdType, Hashable ActualType>
struct AbstractInternArena {
  virtual ~AbstractInternArena() = default;

  [[nodiscard]] IdType intern(const ActualType &actual) const {
    auto iter = m_actual_to_id.find(actual);
    if (iter == m_actual_to_id.end()) {
      throw core::InternalCompilerError(
          "Couldn't find ActualType through const lookup!");
    }
    return iter->second;
  }

  IdType intern(const ActualType &actual) {
    auto iter = m_actual_to_id.find(actual);
    if (iter == m_actual_to_id.end()) {
      auto new_id = m_id_to_actual.size();
      m_actual_to_id.emplace(actual, new_id);
      m_id_to_actual.emplace_back(actual);
      return {new_id};
    }
    return iter->second;
  }

  [[nodiscard]] const ActualType &get(IdType id) const {
    if (id.m_id >= m_id_to_actual.size()) {
      throw core::InternalCompilerError("Failed to call get on id: " +
                                        to_string(id));
    }
    return m_id_to_actual[id.m_id];
  }

  [[nodiscard]] virtual std::string to_string(IdType) const = 0;
  [[nodiscard]] virtual std::string to_string(const ActualType &) const = 0;

  template <typename VariantType>
  const VariantType &as(IdType id, const char *function) const {
    const auto &actual = get(id);
    if (!std::holds_alternative<VariantType>(actual)) {
      throw core::InternalCompilerError(
          std::string(function) + " Bad access to arena with " + to_string(id));
    }
    return std::get<VariantType>(actual);
  }

  template <typename VariantType>
  [[nodiscard]] [[nodiscard]] bool is(IdType id) const {
    const auto &actual = get(id);
    return std::holds_alternative<VariantType>(actual);
  }

private:
  std::unordered_map<ActualType, IdType> m_actual_to_id;
  std::vector<ActualType> m_id_to_actual;
};

template <HashableAndHasId IdType, Hashable ActualType>
struct AbstractPushArena {
  virtual ~AbstractPushArena() = default;

  IdType push(ActualType actual) {
    auto new_id = m_id_to_actual.size();
    m_actual_to_id.emplace(actual, new_id);
    m_id_to_actual.emplace_back(std::move(actual));
    return {new_id};
  }

  [[nodiscard]] const ActualType &get(IdType type_id) const {
    if (type_id.m_id >= m_id_to_actual.size()) {
      throw core::InternalCompilerError("Failed to call get on type_id: " +
                                        to_string(type_id));
    }
    return m_id_to_actual[type_id.m_id];
  }

  [[nodiscard]] virtual std::string to_string(IdType) const = 0;
  [[nodiscard]] virtual std::string to_string(const ActualType &) const = 0;

private:
  std::unordered_map<ActualType, IdType> m_actual_to_id{};
  std::vector<ActualType> m_id_to_actual{};
};

//****************************************************************************
} // namespace bust
//****************************************************************************
