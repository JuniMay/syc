#ifndef SYC_PASSES_ADT_H_
#define SYC_PASSES_ADT_H_

template <typename T>
struct DisjointSet {
  std::unordered_map<T, T> parent;
  std::unordered_map<T, size_t> rank;

  DisjointSet() = default;

  void make_set(T x) {
    if (parent.count(x)) {
      return;
    }
    parent[x] = x;
    rank[x] = 0;
  }

  std::optional<T> find_set(T x) {
    if (!parent.count(x)) {
      return std::nullopt;
    }
    if (parent[x] == x) {
      return x;
    }
    parent[x] = find_set(parent[x]).value();
    return parent[x];
  }

  void union_set(T x, T y) {
    auto maybe_x_root = find_set(x);
    auto maybe_y_root = find_set(y);

    if (!maybe_x_root.has_value() || !maybe_y_root.has_value()) {
      return;
    }

    auto x_root = maybe_x_root.value();
    auto y_root = maybe_y_root.value();

    if (x_root == y_root) {
      return;
    }

    if (rank[x_root] < rank[y_root]) {
      parent[x_root] = y_root;
    } else if (rank[x_root] > rank[y_root]) {
      parent[y_root] = x_root;
    } else {
      parent[y_root] = x_root;
      rank[x_root] += 1;
    }
  }
};

#endif