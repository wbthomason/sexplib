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
concept Deserializable = std::default_initializable<T>&& requires(
T t, const std::string::const_iterator& start, const std::string::const_iterator& end) {
  // clang-format off
  { t.push_atom(start, end) } -> std::same_as<void>; { t.start_list() } -> std::same_as<T*>;
  { t.end_list() } -> std::same_as<T*>;
  // clang-format on
};

void skip_until_non_blank(std::string::const_iterator& it,
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

 private:
  VectorSexp* parent;
};

struct MapSexp {};

}  // namespace sexp
