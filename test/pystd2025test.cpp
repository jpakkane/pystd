// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <pystd_testconfig.hpp>
#include <string.h>

namespace {

const char daikatana[] = "大刀";

}

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return 1;                                                                                  \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_cstring_strip() {
    TEST_START;
    pystd2025::CString input(" abc \t\n\r ");
    const pystd2025::CString correct("abc");
    pystd2025::CString all_whitespace(" ");

    ASSERT(input != correct);
    input.strip();
    ASSERT(input == correct);
    ASSERT(strlen(input.data()) == 3);

    all_whitespace.strip();
    ASSERT(all_whitespace.size() == 0);
    ASSERT(strlen(all_whitespace.data()) == 0);
    return 0;
}

int test_cstring_split() {
    TEST_START;
    pystd2025::CString source("  aa bb cc  ");
    auto parts = source.split();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_cstring_splice() {
    TEST_START;
    pystd2025::CString text("This is short.");
    pystd2025::CString splice("not at all particularly ");
    pystd2025::CString result("This is not at all particularly short.");
    text.insert(8, splice.view());
    ASSERT(text == result);
    return 0;
}

int test_c_strings() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_cstring_strip();
    failing_subtests += test_cstring_split();
    failing_subtests += test_cstring_splice();
    return failing_subtests;
}

int test_u8string_simple() {
    TEST_START;
    pystd2025::U8String str("abc");
    ASSERT(str.size_bytes() == 3);
    ASSERT_WITH(strcmp("abc", str.c_str()) == 0, "String construction failed.");
    return 0;
}

int test_u8_iterator() {
    TEST_START;
    pystd2025::U8String ascii("abc");
    auto start = ascii.cbegin();
    auto end = ascii.cend();
    ASSERT_WITH(start != end, "Iterator construction fail.");
    ASSERT(*start == 'a');
    ++start;
    ASSERT(*start == 'b');
    start++;
    ASSERT(*start == 'c');
    ASSERT(start != end);
    ++start;
    ASSERT(start == end);
    return 0;
}

int test_u8_iterator_cjk() {
    TEST_START;
    pystd2025::U8String cjk(daikatana);
    auto it = cjk.cbegin();
    auto c1 = *it;
    ASSERT(c1 == 22823);
    ++it;
    auto c2 = *it;
    ASSERT(c2 == 20992);
    ++it;
    ASSERT(it == cjk.cend());
    return 0;
}

int test_u8_reverse_iterator() {
    TEST_START;
    pystd2025::U8String ascii("abc");
    ASSERT(ascii.size_bytes() == 3);
    auto start = ascii.crbegin();
    auto end = ascii.crend();
    ASSERT_WITH(start != end, "Iterator construction fail.");
    ASSERT(*start == 'c');
    ++start;
    ASSERT(*start == 'b');
    start++;
    ASSERT(*start == 'a');
    ASSERT(start != end);
    ++start;
    ASSERT(start == end);
    return 0;
}

int test_u8_reverse_iterator_cjk() {
    TEST_START;
    pystd2025::U8String cjk(daikatana);
    auto it = cjk.crbegin();
    auto c1 = *it;
    ASSERT(c1 == 20992);
    ++it;
    auto c2 = *it;
    ASSERT(c2 == 22823);
    ++it;
    ASSERT(it == cjk.crend());
    return 0;
}

int test_u8_split() {
    TEST_START;
    pystd2025::U8String source("  aa bb cc  ");
    auto parts = source.split_ascii();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_u8_append() {
    TEST_START;
    pystd2025::U8String buf("aa");
    const pystd2025::U8String add("bb");

    buf += add;
    ASSERT(buf == "aabb");
    buf += buf;
    ASSERT(buf == "aabbaabb");
    return 0;
}

int test_u8_join() {
    TEST_START;
    pystd2025::U8String separator(", ");
    pystd2025::Vector<pystd2025::U8String> entries;
    entries.push_back(pystd2025::U8String("aa"));
    entries.push_back(pystd2025::U8String("bb"));
    entries.push_back(pystd2025::U8String("cc"));
    auto joined = separator.join(entries);
    ASSERT(joined == "aa, bb, cc");

    return 0;
}

int test_u8_splice() {
    TEST_START;
    pystd2025::U8String text(daikatana);
    pystd2025::U8String splice("a");
    pystd2025::U8String result("大a刀");
    auto loc = text.cbegin();
    ++loc;
    text.insert(loc, splice.view());
    ASSERT(text == result);
    return 0;
}

int test_u8_strings() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_u8string_simple();
    failing_subtests += test_u8_iterator();
    failing_subtests += test_u8_iterator_cjk();
    failing_subtests += test_u8_reverse_iterator();
    failing_subtests += test_u8_reverse_iterator_cjk();
    failing_subtests += test_u8_split();
    failing_subtests += test_u8_append();
    failing_subtests += test_u8_join();
    failing_subtests += test_u8_splice();
    return failing_subtests;
}

int test_u8_regex_simple() {
    TEST_START;
    pystd2025::U8String text("abcabcabc");
    pystd2025::U8String retext("(b).*?(a)");
    pystd2025::U8Regex r(retext);

    auto match = pystd2025::regex_search(r, text);
    ASSERT(match.num_groups() == 2);
    ASSERT(match.group(0) == "bca");
    ASSERT(match.group(1) == "b");
    ASSERT(match.group(2) == "a");

    return 0;
}

int test_u8_regex() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_u8_regex_simple();
    return failing_subtests;
}

int test_optional() {
    TEST_START;
    pystd2025::Optional<uint64_t> empty;
    pystd2025::Optional<uint64_t> filled(666);

    ASSERT(!empty);
    ASSERT(filled);
    ASSERT(*filled == 666);

    empty = pystd2025::move(filled);

    ASSERT(empty);
    ASSERT(!filled);
    ASSERT(*empty == 666);

    filled = 86;
    ASSERT(*filled == 86);
    filled = 1024;
    ASSERT(*filled == 1024);
    return 0;
}

int test_range1() {
    TEST_START;
    pystd2025::Range r(3);

    auto n = r.next();
    ASSERT(*n == 0);
    n = r.next();
    ASSERT(*n == 1);
    n = r.next();
    ASSERT(*n == 2);
    n = r.next();
    ASSERT(!n);
    return 0;
}

int test_range2() {
    TEST_START;
    pystd2025::Range r(10);
    pystd2025::Vector<int64_t> result;
    for(auto &value : pystd2025::LoopView(r)) {
        result.push_back(value);
    }
    ASSERT(result.size() == 10);
    return 0;
}

int test_range3() {
    TEST_START;
    pystd2025::Vector<int64_t> result;
    for(auto &value : pystd2025::Loopsume(pystd2025::Range(10))) {
        result.push_back(value);
    }
    ASSERT(result.size() == 10);
    return 0;
}

int test_range4() {
    TEST_START;
    pystd2025::Range r(10);
    pystd2025::Loopsume loop(pystd2025::move(r));
    for(auto [i, value] : pystd2025::EnumerateView(loop)) {
        ASSERT(i == (size_t)value);
    }
    return 0;
}

int test_vector_simple() {
    TEST_START;
    pystd2025::U8String text("abcabcabc");
    pystd2025::Vector<pystd2025::U8String> v;

    ASSERT(v.size() == 0);
    v.push_back(text);
    ASSERT(v.size() == 1);
    ASSERT(v[0] == text);
    v.pop_back();
    ASSERT(v.size() == 0);

    return 0;
}

int test_vector_uptr() {
    pystd2025::U8String text("abcabcabc");
    pystd2025::Vector<pystd2025::unique_ptr<pystd2025::U8String>> v;

    ASSERT(v.size() == 0);
    v.emplace_back(pystd2025::unique_ptr<pystd2025::U8String>(new pystd2025::U8String(text)));
    ASSERT(v.size() == 1);
    ASSERT(*v[0] == text);
    v.pop_back();
    ASSERT(v.size() == 0);

    return 0;
}

int test_vector() {
    printf("Testing Vector.\n");
    int failing_subtests = 0;
    failing_subtests += test_vector_simple();
    return failing_subtests;
}

int test_range() {
    printf("Testing Range.\n");
    int failing_subtests = 0;
    failing_subtests += test_range1();
    failing_subtests += test_range2();
    failing_subtests += test_range3();
    failing_subtests += test_range4();
    return failing_subtests;
}

int test_file_load() {
    TEST_START;
    pystd2025::Path testdir(PYSTD_TESTDIR);
    auto testfile = testdir / "testfile.txt";
    auto contents = testfile.load_text();
    ASSERT(contents);
    ASSERT(*contents == "This is a test file.\n");
    return 0;
}

int test_files() {
    printf("Testing file access.\n");
    int failing_subtests = 0;
    failing_subtests += test_file_load();
    return failing_subtests;
}

int test_hashset() {
    TEST_START;
    pystd2025::HashSet<int> set;
    ASSERT(set.size() == 0);
    ASSERT(!set.contains(10));
    set.insert(10);
    ASSERT(set.size() == 1);
    ASSERT(set.contains(10));
    ASSERT(!set.contains(11));

    set.insert(10);
    ASSERT(set.size() == 1);
    ASSERT(set.contains(10));
    ASSERT(!set.contains(11));

    set.insert(11);
    ASSERT(set.size() == 2);
    ASSERT(set.contains(10));
    ASSERT(set.contains(11));

    set.remove(10);
    ASSERT(set.size() == 1);
    ASSERT(!set.contains(10));
    ASSERT(set.contains(11));

    set.remove(11);
    ASSERT(set.size() == 0);
    ASSERT(!set.contains(10));
    ASSERT(!set.contains(11));

    return 0;
}

int test_variant1() {
    TEST_START;
    pystd2025::Variant<int32_t, int64_t> v;

    ASSERT(v.contains<int32_t>());
    ASSERT(!v.contains<int64_t>());
    ASSERT(v.get<int32_t>() == 0);

    v.insert(int64_t(33));
    ASSERT(!v.contains<int32_t>());
    ASSERT(v.contains<int64_t>());

    ASSERT(v.get<int64_t>() == 33);
    return 0;
}

int test_variant2() {
    TEST_START;
    pystd2025::Variant<int32_t, int64_t> v1, v2;

    v1.insert(999);
    v2.insert(int64_t{6});

    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == 999);

    v1 = pystd2025::move(v2);

    ASSERT(v1.contains<int64_t>());
    ASSERT(v1.get<int64_t>() == 6);

    return 0;
}

int test_variant3() {
    TEST_START;
    pystd2025::Variant<int32_t, pystd2025::U8String> v1, v2;

    v1.insert(999);
    v2.insert(pystd2025::U8String("String"));
    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == 999);
    ASSERT(v2.contains<pystd2025::U8String>());
    ASSERT(v2.get<pystd2025::U8String>() == "String");

    v1 = pystd2025::move(v2);

    ASSERT(v1.contains<pystd2025::U8String>());
    ASSERT(v1.get<pystd2025::U8String>() == "String");

    return 0;
}

int test_variant() {
    printf("Testing variants.\n");
    int failing_subtests = 0;
    failing_subtests += test_variant1();
    failing_subtests += test_variant2();
    failing_subtests += test_variant3();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_c_strings();
        total_errors += test_u8_strings();
        total_errors += test_u8_regex();
        total_errors += test_optional();
        total_errors += test_range();
        total_errors += test_vector();
        total_errors += test_files();
        total_errors += test_hashset();
        total_errors += test_variant();
    } catch(const pystd2025::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
