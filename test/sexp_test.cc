#include <string>
#include <string_view>
#include <variant>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "sexplib.hh"

using S = sexp::Sexp;
TEST_CASE("We can parse a single atom") {
  const std::string sexp_data = "foo";
  auto result                 = sexp::parse<S>(sexp_data);
  REQUIRE(result.head);
  REQUIRE(!result.tail);
  REQUIRE(*result.head == "foo");
}

TEST_CASE("We can parse an empty s-expression") {
  const std::string sexp_data = "";
  auto result                 = sexp::parse<S>(sexp_data);
  REQUIRE(!result.head);
  REQUIRE(!result.tail);
}

TEST_CASE("We can parse an empty list") {
  const std::string sexp_data = "()";
  auto result                 = sexp::parse<S>(sexp_data);
  REQUIRE(!result.head);
  REQUIRE(!result.tail);
}

inline bool flat_all_atoms_equal(const std::vector<std::string>& atoms, const S& result) {
  const S* current = &result;
  for (size_t i = 0; i < atoms.size(); ++i) {
    REQUIRE(current->head);
    if (atoms[i] != *current->head) {
      FAIL("Result atoms are not tokenized correctly: " << atoms[i] << " vs. " << *current->head);
    }

    if (result.tail) {
      current = &(*result.tail)[i];
    }
  }

  return true;
}

TEST_CASE("We can parse a flat s-expression") {
  const std::string sexp_data          = "(foo bar baz bax 5.3 \"hello\" \"\\\"there you\" :bam)";
  const std::vector<std::string> atoms = {
  "foo", "bar", "baz", "bax", "5.3", "\"hello\"", "\"\\\"there you\"", ":bam"};
  auto result = sexp::parse<S>(sexp_data);
  REQUIRE(result.head);
  REQUIRE(flat_all_atoms_equal(atoms, result));
}

TEST_CASE("Find works for flat s-expressions") {
  const std::string sexp_data          = "(foo bar baz bax 5.3 \"hello\" \"\\\"there you\" :bam)";
  auto result = sexp::parse<S>(sexp_data);
  auto find_1 = result.find("foo/bar");
  auto find_2 = result.find("foo/:bam");
  REQUIRE(find_1);
  REQUIRE(*find_1 == &(*result.tail)[0]);
  REQUIRE(find_2);
  REQUIRE(*find_2 == &(*result.tail)[6]);
}

TEST_CASE("We can parse nested s-expressions") {
  const std::string sexp_data = "(foo (bar baz (bax 5.3) \"hello\") \"\\\"there you\" :bam)";
  auto result                 = sexp::parse<S>(sexp_data);

  REQUIRE(result.head);
  REQUIRE(*result.head == "foo");
  REQUIRE(result.tail);

  auto first_nested_list = (*result.tail)[0];
  REQUIRE(first_nested_list.head);
  REQUIRE(*first_nested_list.head == "bar");
  REQUIRE(first_nested_list.tail);
  REQUIRE((*first_nested_list.tail)[0].head);
  REQUIRE(!(*first_nested_list.tail)[0].tail);
  REQUIRE(*(*first_nested_list.tail)[0].head == "baz");

  auto second_nested_list = (*first_nested_list.tail)[1];
  REQUIRE(second_nested_list.head);
  REQUIRE(*second_nested_list.head == "bax");
  REQUIRE(second_nested_list.tail);
  REQUIRE((*second_nested_list.tail)[0].head);
  REQUIRE(*(*second_nested_list.tail)[0].head == "5.3");

  REQUIRE((*first_nested_list.tail)[2].head);
  REQUIRE(!(*first_nested_list.tail)[2].tail);
  REQUIRE(*(*first_nested_list.tail)[2].head == "\"hello\"");

  REQUIRE((*result.tail)[1].head);
  REQUIRE(!(*result.tail)[1].tail);
  REQUIRE(*(*result.tail)[1].head == "\"\\\"there you\"");

  REQUIRE((*result.tail)[2].head);
  REQUIRE(!(*result.tail)[2].tail);
  REQUIRE(*(*result.tail)[2].head == ":bam");
}

TEST_CASE("Find works for nested s-expressions") {
  const std::string sexp_data = "(foo (bar baz (bax 5.3) \"hello\") \"\\\"there you\" :bam)";
  auto result = sexp::parse<S>(sexp_data);
  auto find_1 = result.find("foo/bar");
  auto find_2 = result.find("foo/bar/bax");
  auto find_3 = result.find("foo/:bam");
  REQUIRE(find_1);
  REQUIRE(*find_1 == &result.tail->at(0));
  REQUIRE(find_2);
  REQUIRE(*find_2 == &result.tail->at(0).tail->at(1));
  REQUIRE(find_3);
  REQUIRE(*find_3 == &(*result.tail)[2]);
}

TEST_CASE("We can handle unusual whitespace") {
  const std::string sexp_data          = "    \t(foo bar\n\n\t  baz \" bax\n\tbam \")\t   ";
  auto result                          = sexp::parse<S>(sexp_data);
  const std::vector<std::string> atoms = {"foo", "bar", "baz", "\" bax\n\tbam \""};
  REQUIRE(flat_all_atoms_equal(atoms, result));
}
