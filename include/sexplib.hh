#pragma once
#include <cassert>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace sexp {
template <typename T>
concept Deserializable = std::default_initializable<T> &&
requires(T t, const std::string::const_iterator& start, const std::string::const_iterator& end) {
  // clang-format off
  { t.push_atom(start, end) } -> std::same_as<void>;
  { t.start_list() } -> std::same_as<T*>;
  { t.end_list() } -> std::same_as<T*>;
  // clang-format on
};

namespace {
  [[maybe_unused]] void skip_until_non_blank(std::string::const_iterator & it,
                                             const std::string::const_iterator& end) {
    while (it != end) {
      switch (*it) {
        case ' ':
        case '\t':
        case '\n':
          break;
        default:
          return;
      };

      ++it;
    }
  }

  template <Deserializable T>
  void push_current_token(bool& non_empty,
                          T* result,
                          std::string::const_iterator& start,
                          std::string::const_iterator& current,
                          const std::string::const_iterator& end) {
    if (non_empty) {
      result->push_atom(start, current);
      non_empty = false;
    }

    start = current + 1;
    skip_until_non_blank(start, end);
    current = start - 1;
  }
}  // namespace

template <Deserializable T> T parse(const std::string& sexp_data) {
  auto start = sexp_data.begin();
  auto end   = sexp_data.end();
  skip_until_non_blank(start, end);
  bool escaped_char = false;
  bool in_string    = false;
  bool non_empty    = false;
  T result;
  T* current_list = &result;
  for (auto current = start; current != end; ++current) {
    if (escaped_char) {
      escaped_char = false;
      continue;
    }

    switch (*current) {
      case '\\':
        escaped_char = in_string;
        break;
      case '"':
        in_string = !in_string;
        break;
      case '(':
        if (!in_string) {
          current_list = current_list->start_list();
          start        = current + 1;
          skip_until_non_blank(start, end);
          current = start - 1;
        }

        break;
      case ')':
        if (!in_string) {
          push_current_token(non_empty, current_list, start, current, end);
          current_list = current_list->end_list();
        }
        break;
      case ' ':
      case '\t':
      case '\n':
        if (in_string) {
          continue;
        }

        push_current_token(non_empty, current_list, start, current, end);
        break;
      default:
        non_empty = true;
        break;
    }
  }

  assert(current_list == &result);
  push_current_token(non_empty, &result, start, end, end);

  return result;
}

#ifdef SEXPLIB_USE_VECTORSEXP
struct VectorSexp {
  std::variant<std::string_view, std::vector<VectorSexp>> data;
  VectorSexp() noexcept : data(std::vector<VectorSexp>(0)), parent(nullptr) {}
  VectorSexp(VectorSexp* parent,
             std::variant<std::string_view, std::vector<VectorSexp>>&& data) noexcept
  : data(data), parent(parent) {}

  void
  push_atom(const std::string::const_iterator& start, const std::string::const_iterator& end) {
    assert(std::holds_alternative<std::vector<VectorSexp>>(data));
    std::get_if<std::vector<VectorSexp>>(&data)->emplace_back(this, std::string_view(start, end));
  }

  VectorSexp* start_list() {
    assert(std::holds_alternative<std::vector<VectorSexp>>(data));
    auto data_list = std::get_if<std::vector<VectorSexp>>(&data);
    if (parent == nullptr && data_list->empty()) {
      return this;
    }

    data_list->emplace_back(this, std::vector<VectorSexp>(0));
    return &data_list->back();
  }

  VectorSexp* end_list() {
    if (parent != nullptr) {
      return parent;
    }

    return this;
  }

  std::optional<const VectorSexp*> find(const std::string& path) const {
    const auto first_slash = path.find('/');
    if (first_slash == std::string::npos ||
        std::holds_alternative<std::vector<VectorSexp>>(data)) {
      const auto current_key =
      std::string_view(path.begin(),
                       first_slash == std::string::npos ? path.end() : path.begin() + first_slash);
      const auto* const data_ptr = std::get_if<std::string_view>(&data);
      if (data_ptr != nullptr && *data_ptr == current_key) {
        return this;
      }

      auto* const data_list = std::get_if<std::vector<VectorSexp>>(&data);
      if (data_list != nullptr) {
        const auto remaining_path = path.substr(first_slash + 1);
        for (auto& child : *data_list) {
          const auto child_result = child.find(remaining_path);
          if (child_result) {
            return child_result;
          }
        }
      }
    }

    return std::nullopt;
  }

 private:
  VectorSexp* parent;
};
#endif

struct Sexp {
  std::optional<std::string_view> head;
  std::optional<std::vector<Sexp>> tail;
  Sexp() noexcept : head(), tail(std::nullopt), parent(nullptr) {}
  Sexp(Sexp* parent) noexcept : head(), tail(std::nullopt), parent(parent) {}
  Sexp(Sexp* parent, std::string_view head) : head(head), tail(std::nullopt), parent(parent) {}

  void
  push_atom(const std::string::const_iterator& start, const std::string::const_iterator& end) {
    if (!head) {
      head.emplace(start, end);
    } else if (!tail) {
      tail.emplace({Sexp(this, std::string_view(start, end))});
    } else {
      tail->emplace_back(Sexp(this, std::string_view(start, end)));
    }
  }

  Sexp* start_list() {
    if (!head) {
      return this;
    }

    if (!tail) {
      tail.emplace({Sexp(this)});
    } else {
      tail->emplace_back(Sexp(this));
    }

    return &tail->back();
  }

  Sexp* end_list() {
    if (parent != nullptr) {
      return parent;
    }

    return this;
  }

  struct SexpIterator {
    const Sexp& operator*() const {
      if (singleton_result) {
        return **singleton_result;
      }

      return *current_it.value();
    }

    void operator++() {
      if (key && *current_it != *end_it) {
        auto& curr = *current_it;
        auto& end  = *end_it;
        ++curr;
        while (curr != end) {
          if (curr->head.value() == key) {
            return;
          }

          ++curr;
        }
      }
    }
    bool done() const { return singleton_result || current_it.value() == end_it.value(); }
    SexpIterator(std::string_view key,
                 std::vector<Sexp>::const_iterator current_it,
                 std::vector<Sexp>::const_iterator end_it)
    : singleton_result(std::nullopt), key(key), current_it(current_it), end_it(end_it) {}
    SexpIterator(const Sexp* const singleton_result)
    : singleton_result(singleton_result)
    , key(std::nullopt)
    , current_it(std::nullopt)
    , end_it(std::nullopt) {}

   private:
    std::optional<const Sexp*> singleton_result;
    std::optional<std::string_view> key;
    std::optional<std::vector<Sexp>::const_iterator> current_it;
    std::optional<std::vector<Sexp>::const_iterator> end_it;
  };

  std::optional<const Sexp*> find_first(const std::string& path) const { return find(path).first; }
  std::optional<SexpIterator> find_all(const std::string& path) const {
    const auto results = find(path);


    if (results_range) {
      return SexpIterator(results_range->first, results_range->second);
    }

    return std::nullopt;
  }

  Sexp& get_child(unsigned int n) { return (*tail)[n]; }

 private:
  Sexp* parent;
  std::pair<
  std::optional<const Sexp*>,
  std::optional<std::pair<std::vector<Sexp>::const_iterator, std::vector<Sexp>::const_iterator>>>
  find(const std::string& path) const {
    if (head) {
      const auto first_slash = path.find('/');
      const auto current_key =
      std::string_view(path.begin(),
                       first_slash == std::string::npos ? path.end() : path.begin() + first_slash);
      if (*head == current_key) {
        if (first_slash == std::string::npos) {
          return {this, std::nullopt};
        }

        if (tail) {
          const auto remaining_path = path.substr(first_slash + 1);
          for (auto child_it = tail->begin(); child_it != tail->end(); ++child_it) {
            const auto child_result = child_it->find(remaining_path);
            if (child_result.first) {
              if (child_result.second) {
                return child_result;
              }

              return {std::move(child_result.first),
                      std::make_pair(std::move(child_it), tail->end())};
            }
          }
        }
      }
    }

    return {std::nullopt, std::nullopt};
  }
};
}  // namespace sexp
