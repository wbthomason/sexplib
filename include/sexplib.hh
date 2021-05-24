#pragma once
#include <concepts>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sexp {
template <typename T>
concept Deserializable = std::default_initializable<T>&& requires(
T t, const std::string::const_iterator& start, const std::string::const_iterator& end) {
  // clang-format off
  { t.push_atom(start, end) } -> std::same_as<void>;
  { t.start_list() } -> std::same_as<void>;
  { t.end_list() } -> std::same_as<void>;
  // clang-format on
};

void skip_until_non_blank(std::string::const_iterator& it,
                          const std::string::const_iterator& end) {
  while (it != end && *it == ' ') {
    ++it;
  }
}

template <Deserializable T>
void push_current_token(bool& non_empty,
                        T& result,
                        std::string::const_iterator& start,
                        std::string::const_iterator& current,
                        const std::string::const_iterator& end) {
  if (non_empty) {
    result.push_atom(start, current);
    non_empty = false;
  }

  start = current;
  skip_until_non_blank(start, end);
  current = start;
}

template <Deserializable T> T parse(const std::string& sexp_data) {
  auto start     = sexp_data.begin();
  const auto end = sexp_data.end();
  skip_until_non_blank(start, end);
  bool escaped_char = false;
  bool in_string    = false;
  bool non_empty    = false;
  T result;
  for (auto current = start; current != end; ++current) {
    if (escaped_char) {
      escaped_char = false;
      continue;
    }

    switch (*current) {
      case '\\':
        escaped_char = true;
        break;
      case '"':
        in_string = !in_string;
        break;
      case '(':
        if (!in_string) {
          result.start_list();
        }
        break;
      case ')':
        if (!in_string) {
          push_current_token(non_empty, result, start, current, end);
          result.end_list();
        }
        break;
      case ' ':
        if (in_string) {
          continue;
        }

        push_current_token(non_empty, result, start, current, end);
        break;
      default:
        non_empty = true;
        break;
    }
  }

  push_current_token(non_empty, result, start, end, end);
}

struct Sexp;
struct Sexp {
  Sexp* const parent;
  std::variant<std::string_view, std::vector<Sexp>> data;
  Sexp(Sexp* parent, std::variant<std::string_view, std::vector<Sexp>>&& data)
  : parent(parent), data(data) {}
};

struct VectorSexp {
  Sexp data;
  VectorSexp() noexcept : data(nullptr, std::vector<Sexp>()) {}

  void
  push_atom(const std::string::const_iterator& start, const std::string::const_iterator& end) {
    current_list->emplace_back(current_atom, std::string_view(start, end));
  }

  void start_list() {
    if (current_list == nullptr) {
      current_list = &std::get<std::vector<Sexp>>(data.data);
      return;
    }

    current_list->emplace_back(current_list, std::vector<Sexp>());
    current_atom = &current_list->back();
    current_list = &std::get<std::vector<Sexp>>(current_atom->data);
  }

  void end_list() {
    if (current_atom != nullptr) {
      current_atom = current_atom->parent;
      current_list = &std::get<std::vector<Sexp>>(current_atom->data);
    }
  }

 private:
  Sexp* current_atom              = nullptr;
  std::vector<Sexp>* current_list = nullptr;
};

struct MapSexp {};

}  // namespace sexp
