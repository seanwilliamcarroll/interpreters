#pragma once

#include <vector>

// Union-Find (Disjoint Set Union)

class UnionFind {
public:
  explicit UnionFind(int n)
      : parent(static_cast<size_t>(n), 0), rank(static_cast<size_t>(n), 0) {
    for (int node = 0; node < n; ++node) {
      get_parent(node) = node;
    }
  }

  int find(int x) {
    int root_node = x;
    while (get_parent(root_node) != root_node) {
      root_node = get_parent(root_node);
    }
    while (get_parent(x) != root_node) {
      int next_node = get_parent(x);
      get_parent(x) = root_node;
      x = next_node;
    }
    return root_node;
  }

  void unite(int x, int y) {
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

  bool connected(int x, int y) { return find(x) == find(y); }

private:
  [[nodiscard]] const int &get_parent(int index) const {
    return parent[static_cast<size_t>(index)];
  }

  int &get_parent(int index) {
    return const_cast<int &>(
        static_cast<const UnionFind *>(this)->get_parent(index));
  }

  [[nodiscard]] const int &get_rank(int index) const {
    return rank[static_cast<size_t>(index)];
  }

  int &get_rank(int index) {
    return const_cast<int &>(
        static_cast<const UnionFind *>(this)->get_rank(index));
  }

  std::vector<int> parent;
  std::vector<int> rank;
};
