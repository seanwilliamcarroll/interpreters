#pragma once

#include <vector>

// Union-Find (Disjoint Set Union)

class UnionFind {
public:
  explicit UnionFind() = default;

  size_t find(size_t x) {
    size_t root_node = x;
    while (get_parent(root_node) != root_node) {
      root_node = get_parent(root_node);
    }
    while (get_parent(x) != root_node) {
      size_t next_node = get_parent(x);
      get_parent(x) = root_node;
      x = next_node;
    }
    return root_node;
  }

  void unite(size_t x, size_t y) {
    auto parent_x = find(x);
    auto parent_y = find(y);
    if (parent_x == parent_y) {
      return;
    }

    if (get_rank(parent_x) < get_rank(parent_y)) {
      get_parent(parent_x) = parent_y;
    } else if (get_rank(parent_x) > get_rank(parent_y)) {
      get_parent(parent_y) = parent_x;
    } else {
      get_parent(parent_x) = parent_y;
      get_rank(parent_y)++;
    }
  }

  bool connected(size_t x, size_t y) { return find(x) == find(y); }

  size_t add_node() {
    auto new_node = parent.size();
    parent.push_back(new_node);
    rank.push_back(0);
    return new_node;
  }

private:
  [[nodiscard]] const size_t &get_parent(size_t index) const {
    return parent[index];
  }

  size_t &get_parent(size_t index) {
    return const_cast<size_t &>(
        static_cast<const UnionFind *>(this)->get_parent(index));
  }

  [[nodiscard]] const size_t &get_rank(size_t index) const {
    return rank[index];
  }

  size_t &get_rank(size_t index) {
    return const_cast<size_t &>(
        static_cast<const UnionFind *>(this)->get_rank(index));
  }

  std::vector<size_t> parent;
  std::vector<size_t> rank;
};
