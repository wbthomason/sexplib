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
  const std::string sexp_data = "(foo bar baz bax 5.3 \"hello\" \"\\\"there you\" :bam)";
  auto result                 = sexp::parse<S>(sexp_data);
  auto find_1                 = result.find_first("foo/bar");
  auto find_2                 = result.find_first("foo/:bam");
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
  auto result                 = sexp::parse<S>(sexp_data);
  auto find_1                 = result.find_first("foo/bar");
  auto find_2                 = result.find_first("foo/bar/bax");
  auto find_3                 = result.find_first("foo/:bam");
  REQUIRE(find_1);
  REQUIRE(*find_1 == &result.tail->at(0));
  REQUIRE(find_2);
  REQUIRE(*find_2 == &result.tail->at(0).tail->at(1));
  REQUIRE(find_3);
  REQUIRE(*find_3 == &(*result.tail)[2]);
}

TEST_CASE("find_all finds all instances of a path correctly") {
  const std::string sexp_data =
  "(foo (bar baz (bax 5.3) (bax 6.7) (bax 10) \"hello\") (bax \"oh no\") \"\\\"there you\" :bam)";
  auto result = sexp::parse<S>(sexp_data);
  auto find_1 = result.find_all("foo");
  REQUIRE(find_1);
  auto find_1_result = find_1.value();
  REQUIRE(&(*find_1_result) == &result);
  auto find_2 = result.find_all("foo/bar");
  REQUIRE(find_2);
  auto find_2_result = find_2.value();
  REQUIRE(&(*find_2_result) == &result.tail->at(0));
  auto find_3 = result.find_all("foo/bar/bax");
  REQUIRE(find_3);
  auto find_3_result          = find_3.value();
  unsigned int result_counter = 0;
  std::array<std::string, 3> result_values{"5.3", "6.7", "10"};
  while (!find_3_result.done()) {
    auto& iter_result = *find_3_result;
    REQUIRE(iter_result.tail);
    REQUIRE(iter_result.tail->at(0).head.value() == result_values[result_counter]);
    ++find_3_result;
    ++result_counter;
  }

  REQUIRE(result_counter == 3);
}

TEST_CASE("We can handle unusual whitespace") {
  const std::string sexp_data          = "    \t(foo bar\n\n\t  baz \" bax\n\tbam \")\t   ";
  auto result                          = sexp::parse<S>(sexp_data);
  const std::vector<std::string> atoms = {"foo", "bar", "baz", "\" bax\n\tbam \""};
  REQUIRE(flat_all_atoms_equal(atoms, result));
}

TEST_CASE("We can parse real PDDL") {
  const std::string sexp_data = "(and (on-surface ?ob ?surf) (arm-empty ?m) (at ?m ?ob))";
  auto result = sexp::parse<S>(sexp_data);
  REQUIRE(result.head);
  REQUIRE(result.head.value() == "and");
  REQUIRE(result.tail->size() == 3);
  auto& sub_sexp_1 = result.tail->at(0);
  REQUIRE(sub_sexp_1.head.value() == "on-surface");
  REQUIRE(sub_sexp_1.tail->at(0).head.value() == "?ob");
  REQUIRE(sub_sexp_1.tail->at(1).head.value() == "?surf");
  auto& sub_sexp_2 = result.tail->at(1);
  REQUIRE(sub_sexp_2.head.value() == "arm-empty");
  REQUIRE(sub_sexp_2.tail->at(0).head.value() == "?m");
  auto& sub_sexp_3 = result.tail->at(2);
  REQUIRE(sub_sexp_3.head.value() == "at");
  REQUIRE(sub_sexp_3.tail->at(0).head.value() == "?m");
  REQUIRE(sub_sexp_3.tail->at(1).head.value() == "?ob");
}
