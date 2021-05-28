#include <string>
#include <string_view>
#include <variant>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#define SEXPLIB_USE_VECTORSEXP
#include "sexplib.hh"

using VS = sexp::VectorSexp;
TEST_CASE("We can parse a single atom") {
  const std::string sexp_data = "foo";
  auto result                 = sexp::parse<VS>(sexp_data);
  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  const auto data_list = std::get<std::vector<VS>>(result.data);
  REQUIRE(data_list.size() == 1);
  REQUIRE(std::holds_alternative<std::string_view>(data_list[0].data));
  REQUIRE(std::get<std::string_view>(data_list[0].data) == "foo");
}

TEST_CASE("We can parse an empty s-expression") {
  const std::string sexp_data = "";
  auto result                 = sexp::parse<VS>(sexp_data);
  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  const auto data_list = std::get<std::vector<VS>>(result.data);
  REQUIRE(data_list.size() == 0);
}

TEST_CASE("We can parse an empty list") {
  const std::string sexp_data = "()";
  auto result                 = sexp::parse<VS>(sexp_data);
  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  const auto data_list = std::get<std::vector<VS>>(result.data);
  REQUIRE(data_list.size() == 0);
}

inline bool flat_all_atoms_equal(const std::vector<std::string>& atoms, const VS& result) {
  const auto result_atoms = std::get<std::vector<VS>>(result.data);
  for (size_t i = 0; i < atoms.size(); ++i) {
    REQUIRE(std::holds_alternative<std::string_view>(result_atoms[i].data));
    const auto result_atom = std::get<std::string_view>(result_atoms[i].data);
    if (atoms[i] != result_atom) {
      FAIL("Result atoms are not tokenized correctly: " << atoms[i] << " vs. " << result_atom);
    }
  }

  return true;
}

TEST_CASE("We can parse a flat s-expression") {
  const std::string sexp_data          = "(foo bar baz bax 5.3 \"hello\" \"\\\"there you\" :bam)";
  const std::vector<std::string> atoms = {
  "foo", "bar", "baz", "bax", "5.3", "\"hello\"", "\"\\\"there you\"", ":bam"};
  auto result = sexp::parse<VS>(sexp_data);
  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  REQUIRE(flat_all_atoms_equal(atoms, result));
}


TEST_CASE("We can parse nested s-expressions") {
  const std::string sexp_data = "(foo (bar baz (bax 5.3) \"hello\") \"\\\"there you\" :bam)";
  auto result                 = sexp::parse<VS>(sexp_data);

  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  auto data_list = std::get<std::vector<VS>>(result.data);

  REQUIRE(std::get<std::string_view>(data_list[0].data) == "foo");
  REQUIRE(std::holds_alternative<std::vector<VS>>(data_list[1].data));
  auto first_nested_list = std::get<std::vector<VS>>(data_list[1].data);

  REQUIRE(std::get<std::string_view>(first_nested_list[0].data) == "bar");
  REQUIRE(std::get<std::string_view>(first_nested_list[1].data) == "baz");

  REQUIRE(std::holds_alternative<std::vector<VS>>(first_nested_list[2].data));
  auto second_nested_list = std::get<std::vector<VS>>(first_nested_list[2].data);

  REQUIRE(std::get<std::string_view>(second_nested_list[0].data) == "bax");
  REQUIRE(std::get<std::string_view>(second_nested_list[1].data) == "5.3");
  REQUIRE(std::get<std::string_view>(first_nested_list[3].data) == "\"hello\"");
  REQUIRE(std::get<std::string_view>(data_list[2].data) == "\"\\\"there you\"");
  REQUIRE(std::get<std::string_view>(data_list[3].data) == ":bam");
}

TEST_CASE("We can handle unusual whitespace") {
  const std::string sexp_data          = "    \t(foo bar\n\n\t  baz \" bax\n\tbam \")\t   ";
  auto result                          = sexp::parse<VS>(sexp_data);
  const std::vector<std::string> atoms = {"foo", "bar", "baz", "\" bax\n\tbam \""};
  REQUIRE(std::holds_alternative<std::vector<VS>>(result.data));
  REQUIRE(flat_all_atoms_equal(atoms, result));
}
