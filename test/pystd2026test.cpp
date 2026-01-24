// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

// Include this to ensure that mixing pystd headers works.
#include <pystd2025.hpp>

#include <pystd2026.hpp>
#include <pystd_testconfig.hpp>
#include <string.h>

namespace {

const char daikatana[] = "大刀";

}

int breakpoint_opportunity(int number) { return number; }

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_cstring_strip() {
    TEST_START;
    pystd2026::CString input(" abc \t\n\r ");
    const pystd2026::CString correct("abc");
    pystd2026::CString all_whitespace(" ");

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
    pystd2026::CString source("  aa bb cc  ");
    auto parts = source.split();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_cstring_splice() {
    TEST_START;
    pystd2026::CString text("This is short.");
    pystd2026::CString splice("not at all particularly ");
    pystd2026::CString result("This is not at all particularly short.");
    text.insert(8, splice.view());
    ASSERT(text == result);
    return 0;
}

int test_cstring_casing() {
    TEST_START;
    const pystd2026::CString START("heLlo!");
    const pystd2026::CString UPPER("HELLO!");
    const pystd2026::CString LOWER("hello!");

    auto str = START.upper();
    ASSERT(str == UPPER);

    str = str.lower();
    ASSERT(str == LOWER);
    return 0;
}

int test_c_strings() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_cstring_strip();
    failing_subtests += test_cstring_split();
    failing_subtests += test_cstring_splice();
    failing_subtests += test_cstring_casing();
    return failing_subtests;
}

int test_fixed_c_string_basic() {
    TEST_START;
    pystd2026::FixedCString<16> empty;
    ASSERT(empty == "");
    ASSERT(empty != "abc");

    pystd2026::FixedCString<16> one("abc");
    pystd2026::FixedCString<16> two("def");

    ASSERT(one.size() == 3);
    ASSERT(one[1] == 'b');

    // There is no actual equality operator because GCC says it is not standards compliant
    // due to standardese shenanigans.
    ASSERT(one != two.view());

    two.pop_back();
    ASSERT(two == "de");
    ASSERT(two.size() == 2);
    ASSERT(!two.is_empty());
    two.pop_back();
    two.pop_back();
    ASSERT(two.is_empty());
    two = one;
    ASSERT(two == one.view());

    pystd2026::FixedCString<32> big;
    big = one;
    ASSERT(big == one.view());
    return 0;
}

int test_fixed_c_string_toobig() {
    TEST_START;
    pystd2026::FixedCString<8> small;
    bool exception_happened = false;
    try {
        small = "This is way too big for you to handle.";
    } catch(const pystd2026::PyException &) {
        exception_happened = true;
    }

    ASSERT(exception_happened);
    ASSERT(small.is_empty());
    return 0;
}

int test_fixed_c_strings() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_fixed_c_string_basic();
    failing_subtests += test_fixed_c_string_toobig();
    return failing_subtests;
}

int test_u8string_simple() {
    TEST_START;
    pystd2026::U8String str("abc");
    ASSERT(str.size_bytes() == 3);
    ASSERT_WITH(strcmp("abc", str.c_str()) == 0, "String construction failed.");
    return 0;
}

int test_u8_iterator() {
    TEST_START;
    pystd2026::U8String ascii("abc");
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
    pystd2026::U8String cjk(daikatana);
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
    pystd2026::U8String ascii("abc");
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
    pystd2026::U8String cjk(daikatana);
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
    pystd2026::U8String source("  aa bb cc  ");
    auto parts = source.split_ascii();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_u8_append() {
    TEST_START;
    pystd2026::U8String buf("aa");
    const pystd2026::U8String add("bb");

    buf += add;
    ASSERT(buf == "aabb");
    buf += buf;
    ASSERT(buf == "aabbaabb");
    return 0;
}

int test_u8_join() {
    TEST_START;
    pystd2026::U8String separator(", ");
    pystd2026::Vector<pystd2026::U8String> entries;
    entries.push_back(pystd2026::U8String("aa"));
    entries.push_back(pystd2026::U8String("bb"));
    entries.push_back(pystd2026::U8String("cc"));
    auto joined = separator.join(entries);
    ASSERT(joined == "aa, bb, cc");

    return 0;
}

int test_u8_splice() {
    TEST_START;
    pystd2026::U8String text(daikatana);
    pystd2026::U8String splice("a");
    pystd2026::U8String result("大a刀");
    auto loc = text.cbegin();
    ++loc;
    text.insert(loc, splice.view());
    ASSERT(text == result);
    return 0;
}

int test_u8_remove() {
    TEST_START;
    pystd2026::U8String text("大a刀");
    pystd2026::U8String splice("a");
    pystd2026::U8String result(daikatana);

    auto start = text.cbegin();
    ++start;
    auto end = start;
    ++end;

    pystd2026::U8StringView to_remove(start, end);
    text.remove(to_remove);
    ASSERT(text == result);
    return 0;
}

int test_u8_pop() {
    TEST_START;
    pystd2026::U8String text("ab大");
    pystd2026::U8String r1("b大");
    pystd2026::U8String r2("b");

    text.pop_front();
    ASSERT(text == r1);

    text.pop_back();
    ASSERT(text == r2);
    return 0;
}

int test_u8_casing() {
    TEST_START;
    const pystd2026::U8String START("Aa-åÄö.487/-");
    const pystd2026::U8String UPPER("AA-ÅÄÖ.487/-");
    const pystd2026::U8String LOWER("aa-åäö.487/-");

    pystd2026::U8String str = START.upper();
    ASSERT(str == UPPER);

    str = str.lower();
    ASSERT(str == LOWER);

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
    failing_subtests += test_u8_remove();
    failing_subtests += test_u8_pop();
    failing_subtests += test_u8_casing();
    return failing_subtests;
}

int test_u8_regex_simple() {
    TEST_START;
    pystd2026::U8String text("abcabcabc");
    pystd2026::U8String retext("(b).*?(a)");
    pystd2026::U8Regex r(retext);

    auto match = pystd2026::regex_search(r, text);
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
    pystd2026::Optional<uint64_t> empty;
    pystd2026::Optional<uint64_t> filled(666);

    ASSERT(!empty);
    ASSERT(filled);
    ASSERT(*filled == 666);

    empty = pystd2026::move(filled);

    ASSERT(empty);
    ASSERT(!filled);
    ASSERT(*empty == 666);

    filled = 86;
    ASSERT(*filled == 86);
    filled = 1024;
    ASSERT(*filled == 1024);
    return 0;
}

int test_span_basic() {
    TEST_START;
    const int BUFSIZE = 5;
    const int buf[BUFSIZE] = {0, 1, 2, 3, 4};
    pystd2026::Span<int> empty_span;
    pystd2026::Span<const int> basic_span(buf, BUFSIZE);

    ASSERT(empty_span.is_empty());
    ASSERT(!basic_span.is_empty());

    ASSERT(basic_span[1] == 1);
    bool exception_happened = false;
    try {
        const auto nope = basic_span[BUFSIZE];
        (void)nope;
    } catch(const pystd2026::PyException &) {
        exception_happened = true;
    }
    ASSERT(exception_happened);

    return 0;
}

int test_span_sub() {
    TEST_START;
    const int BUFSIZE = 5;
    const int buf[BUFSIZE] = {0, 1, 2, 3, 4};
    pystd2026::Span<const int> basic_span(buf, BUFSIZE);

    auto subspan = basic_span.subspan(1, 2);
    ASSERT(subspan.size() == 2);
    ASSERT(subspan[0] == 1);

    subspan = basic_span.subspan(2);
    ASSERT(subspan.size() == 3);
    ASSERT(subspan[0] == 2);

    subspan = basic_span.subspan(0, 0);
    ASSERT(subspan.is_empty());
    return 0;
}

int test_span_write() {
    TEST_START;
    const int BUFSIZE = 5;
    int buf[BUFSIZE] = {0, 1, 2, 3, 4};
    pystd2026::Span<int> basic_span(buf, BUFSIZE);

    basic_span[2] = 666;
    ASSERT(buf[2] == 666);
    return 0;
}

int test_span() {
    printf("Testing Span.\n");
    int failing_subtests = 0;
    failing_subtests += test_span_basic();
    failing_subtests += test_span_sub();
    failing_subtests += test_span_write();
    return failing_subtests;
}

int test_range1() {
    TEST_START;
    pystd2026::Range r(3);

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
    pystd2026::Range r(10);
    pystd2026::Vector<int64_t> result;
    for(auto &value : pystd2026::LoopView(r)) {
        result.push_back(value);
    }
    ASSERT(result.size() == 10);
    return 0;
}

int test_range3() {
    TEST_START;
    pystd2026::Vector<int64_t> result;
    for(auto &value : pystd2026::Loopsume(pystd2026::Range(10))) {
        result.push_back(value);
    }
    ASSERT(result.size() == 10);
    return 0;
}

int test_range4() {
    TEST_START;
    pystd2026::Range r(10);
    pystd2026::Loopsume loop(pystd2026::move(r));
    for(auto [i, value] : pystd2026::EnumerateView(loop)) {
        ASSERT(i == (size_t)value);
    }
    return 0;
}

int test_vector_simple() {
    TEST_START;
    pystd2026::U8String text("abcabcabc");
    pystd2026::Vector<pystd2026::U8String> v;

    ASSERT(v.size() == 0);
    v.push_back(text);
    ASSERT(v.size() == 1);
    ASSERT(v[0] == text);
    v.pop_back();
    ASSERT(v.size() == 0);

    return 0;
}

int test_vector_uptr() {
    pystd2026::U8String text("abcabcabc");
    pystd2026::Vector<pystd2026::unique_ptr<pystd2026::U8String>> v;

    ASSERT(v.size() == 0);
    v.emplace_back(pystd2026::unique_ptr<pystd2026::U8String>(new pystd2026::U8String(text)));
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
    pystd2026::Path testdir(PYSTD_TESTDIR);
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

int test_hashmap() {
    TEST_START;
    pystd2026::HashMap<int, int> map;
    const int NUM_ENTRIES = 32;
    ASSERT(map.is_empty());
    for(int i = 0; i < NUM_ENTRIES; ++i) {
        ASSERT(map.lookup(i) == nullptr);
        map.insert(i, NUM_ENTRIES - i);
        // All other entries must remain the same.
        for(int j = 0; j <= i; ++j) {
            ASSERT(*map.lookup(j) == NUM_ENTRIES - j);
        }
        for(int j = i + 1; j < NUM_ENTRIES; ++j) {
            ASSERT(!map.lookup(j));
        }
    }
    ASSERT(map.size() == NUM_ENTRIES);

    for(int i = 0; i < NUM_ENTRIES; ++i) {
        auto *l = map.lookup(i);
        ASSERT(l);
        ASSERT(*l == NUM_ENTRIES - i);
        map.remove(i);
        ASSERT(map.lookup(i) == nullptr);
        for(int j = 0; j < i; ++j) {
            ASSERT(!map.lookup(j));
        }
        for(int j = i + 1; j < NUM_ENTRIES; ++j) {
            ASSERT(*map.lookup(j) == NUM_ENTRIES - j);
        }
    }
    ASSERT(map.is_empty());

    return 0;
}

int test_hashset() {
    TEST_START;
    pystd2026::HashSet<int> set;
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
    pystd2026::Variant<int32_t, int64_t> v;

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
    pystd2026::Variant<int32_t, int64_t> v1, v2;

    v1.insert(999);
    v2.insert(int64_t{6});

    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == 999);

    v1 = pystd2026::move(v2);

    ASSERT(v1.contains<int64_t>());
    ASSERT(v1.get<int64_t>() == 6);

    return 0;
}

int test_variant3() {
    TEST_START;
    pystd2026::Variant<int32_t, pystd2026::U8String> v1, v2;

    v1.insert(999);
    v2.insert(pystd2026::U8String("String"));
    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == 999);
    ASSERT(v2.contains<pystd2026::U8String>());
    ASSERT(v2.get<pystd2026::U8String>() == "String");

    v1 = pystd2026::move(v2);

    ASSERT(v1.contains<pystd2026::U8String>());
    ASSERT(v1.get<pystd2026::U8String>() == "String");

    return 0;
}

int test_variant4() {
    TEST_START;
    pystd2026::Variant<int32_t, pystd2026::U8String> v1(666);
    pystd2026::Variant<int32_t, pystd2026::U8String> v2(pystd2026::U8String("bob"));

    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == 666);

    ASSERT(v2.contains<pystd2026::U8String>());
    ASSERT(v2.get<pystd2026::U8String>() == "bob");

    return 0;
}

int test_variant5() {
    TEST_START;
    int32_t number = 666;
    pystd2026::Variant<int32_t, pystd2026::U8String> v1(number);
    pystd2026::Variant<int32_t, pystd2026::U8String> v2(pystd2026::U8String("bob"));

    ASSERT(v1.contains<int32_t>());
    ASSERT(v1.get<int32_t>() == number);

    ASSERT(v2.contains<pystd2026::U8String>());
    ASSERT(v2.get<pystd2026::U8String>() == "bob");

    return 0;
}

int test_variant() {
    printf("Testing variants.\n");
    int failing_subtests = 0;
    failing_subtests += test_variant1();
    failing_subtests += test_variant2();
    failing_subtests += test_variant3();
    failing_subtests += test_variant4();
    failing_subtests += test_variant5();
    return failing_subtests;
}

int test_format1() {
    TEST_START;
    auto str = pystd2026::cformat("Number is %d.", 86);
    ASSERT(str == "Number is 86.");
    return 0;
}

int test_format2() {
    TEST_START;
    pystd2026::CString output("prefix: ");

    pystd2026::format_append(output, "Number is %d.", 86);
    ASSERT(output == "prefix: Number is 86.");
    return 0;
}

int test_format() {
    printf("Testing format.\n");
    int failing_subtests = 0;
    failing_subtests += test_format1();
    failing_subtests += test_format2();
    return failing_subtests;
}

void sort_init(pystd2026::Vector<int> &buf) {
    buf.clear();
    buf.push_back(3);
    buf.push_back(2);
    buf.push_back(1);
    buf.push_back(0);
}

int test_sort1() {
    TEST_START;
    pystd2026::Vector<int> buf;
    pystd2026::Vector<int> result;

    result.push_back(0);
    result.push_back(1);
    result.push_back(2);
    result.push_back(3);

    sort_init(buf);
    ASSERT(buf != result);

    pystd2026::insertion_sort(buf.begin(), buf.end());
    ASSERT(buf == result);

    pystd2026::insertion_sort(buf.begin(), buf.end());
    ASSERT(buf == result);

    return 0;
}

int test_sort() {
    printf("Testing sort.\n");
    int failing_subtests = 0;
    failing_subtests += test_sort1();
    return failing_subtests;
}

enum class ErrorCode : int32_t {
    NoError,
    DynamicError,
};

int test_fixedvector1() {
    TEST_START;
    pystd2026::FixedVector<int, 5> fv;
    fv.push_back(1);
    ASSERT(fv.size() == 1);
    ASSERT(!fv.is_full());
    fv.insert(0, 0);
    fv.push_back(3);
    fv.insert(3, 4);
    fv.insert(2, 2);
    ASSERT(fv.is_full());
    int i = 0;
    for(const auto &value : fv) {
        ASSERT(value == i);
        ++i;
    }

    ASSERT(!fv.try_push_back(42));
    ASSERT(fv.size() == 5);
    fv.pop_back();
    ASSERT(fv.size() == 4);
    fv.delete_at(0);
    ASSERT(fv.size() == 3);
    ASSERT(fv[0] == 1);
    ASSERT(fv[1] == 2);
    ASSERT(fv[2] == 3);

    fv.delete_at(1);
    ASSERT(fv.size() == 2);
    ASSERT(fv[0] == 1);
    ASSERT(fv[1] == 3);

    return 0;
}

int test_fixedvector() {
    printf("Testing FixedVector.\n");
    int failing_subtests = 0;
    failing_subtests += test_fixedvector1();
    return failing_subtests;
}

#include <pystd2026_btree.hpp>

int test_btree1() {
    TEST_START;
    const int shuffled[] = {7,  17, 19, 24, 2, 20, 14, 1,  6, 23, 8, 12, 25,
                            21, 15, 22, 5,  0, 18, 4,  16, 3, 11, 9, 13, 10};
    const int arr_size = 26;
    pystd2026::BTree<int, 5> btree;
    ASSERT(btree.is_empty());
    for(int i = 0; i < arr_size; ++i) {
        btree.insert(shuffled[i]);
        for(int j = 0; j < arr_size; ++j) {
            auto *valptr = btree.lookup(shuffled[j]);
            if(j <= i) {
                ASSERT(valptr);
                ASSERT(*valptr == shuffled[j]);
            } else {
                ASSERT(!valptr);
            }
        }
    }
    ASSERT(btree.size() == 26);
    btree.insert(7);
    ASSERT(btree.size() == 26);

    ASSERT(!btree.lookup(100));

    size_t expected_size = 26;

    ASSERT(btree.size() == expected_size);
    for(int i = 0; i < arr_size; ++i) {
        btree.remove(shuffled[i]);
        --expected_size;
        ASSERT(btree.size() == expected_size);
        for(int j = 0; j < arr_size; ++j) {
            auto *valptr = btree.lookup(shuffled[j]);
            if(j <= i) {
                ASSERT(!valptr);
            } else {
                ASSERT(valptr);
                ASSERT(*valptr == shuffled[j]);
            }
        }
    }
    ASSERT(btree.is_empty());

    return 0;
}

int test_btree_iteration() {
    TEST_START;
    pystd2026::BTree<int, 5> btree;
    for(int i = 99; i >= 0; --i) {
        btree.insert(i);
    }

    int expected = 0;
    for(const auto &val : btree) {
        ASSERT(val == expected);
        ++expected;
    }
    ASSERT(expected == 100);
    return 0;
}

int test_btree() {
    printf("Testing Btree.\n");
    int failing_subtests = 0;
    failing_subtests += test_btree1();
    failing_subtests += test_btree_iteration();
    return failing_subtests;
}

int test_partition1() {
    TEST_START;
    int buf[6] = {1, 3, 1, 3, 3, 1};
    auto midpoint = pystd2026::partition(buf, buf + 6, [](const int i) -> bool { return i < 2; });

    ASSERT(midpoint - buf == 3);
    ASSERT(buf[0] == 1);
    ASSERT(buf[1] == 1);
    ASSERT(buf[2] == 1);
    ASSERT(buf[3] == 3);
    ASSERT(buf[4] == 3);
    ASSERT(buf[5] == 3);
    return 0;
}

int test_partition() {
    printf("Testing partition.\n");
    int failing_subtests = 0;
    failing_subtests += test_partition1();
    return failing_subtests;
}

int test_uppercasing() {
    TEST_START;
    auto rc = pystd2026::uppercase_unicode('c');
    ASSERT(rc.codepoints[0] == 'C');
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::uppercase_unicode('C');
    ASSERT(rc.codepoints[0] == 'C');
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::uppercase_unicode(223); // ß
    ASSERT(rc.codepoints[0] == 'S');
    ASSERT(rc.codepoints[1] == 'S');
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::uppercase_unicode(958); // ξ
    ASSERT(rc.codepoints[0] == 926);        // Ξ
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::uppercase_unicode(33333);
    ASSERT(rc.codepoints[0] == 33333);
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    return 0;
}

int test_lowercasing() {
    TEST_START;
    auto rc = pystd2026::lowercase_unicode('C');
    ASSERT(rc.codepoints[0] == 'c');
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::lowercase_unicode('c');
    ASSERT(rc.codepoints[0] == 'c');
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::lowercase_unicode(304); // İ
    ASSERT(rc.codepoints[0] == 105);
    ASSERT(rc.codepoints[1] == 775);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::lowercase_unicode(926); // Ξ
    ASSERT(rc.codepoints[0] == 958);        // ξ
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    memset(&rc, 42, sizeof(rc));
    rc = pystd2026::lowercase_unicode(33333);
    ASSERT(rc.codepoints[0] == 33333);
    ASSERT(rc.codepoints[1] == 0);
    ASSERT(rc.codepoints[2] == 0);

    return 0;
}

int test_unicode() {
    printf("Testing partition.\n");
    int failing_subtests = 0;
    failing_subtests += test_uppercasing();
    failing_subtests += test_lowercasing();
    return failing_subtests;
}

#include <pystd2026_threading.hpp>

int test_mutex() {
    TEST_START;

    pystd2026::Mutex m;

    ASSERT(m.try_lock());
    ASSERT(!m.try_lock());
    m.unlock();

    {
        pystd2026::LockGuard<pystd2026::Mutex> guard(m);
        ASSERT(!m.try_lock());
    }
    ASSERT(m.try_lock());
    m.unlock();
    return 0;
}

int test_thread() {
    TEST_START;
    typedef struct {
        int x;
        pystd2026::Mutex m;
    } Context;
    Context ctx;
    ctx.x = 0;

    auto incrementer = [](void *ctx) -> void * {
        Context *c = reinterpret_cast<Context *>(ctx);
        for(int i = 0; i < 1000; ++i) {
            pystd2026::LockGuard<pystd2026::Mutex> lg(c->m);
            ++c->x;
        }
        return nullptr;
    };
    {
        pystd2026::Thread th1(incrementer, &ctx);
        pystd2026::Thread th2(incrementer, &ctx);
        pystd2026::Thread th3(incrementer, &ctx);
        pystd2026::Thread th4(incrementer, &ctx);
    }
    ASSERT(ctx.x == 4000);

    return 0;
}

int test_threading() {
    printf("Testing threading.\n");
    int failing_subtests = 0;
    failing_subtests += test_mutex();
    failing_subtests += test_thread();
    return failing_subtests;
}

int test_std_mixing() {
    printf("Testing mixing std versions.\n");
    int failing_subtests = 0;
    pystd2025::CString str1("abc");
    pystd2026::CString str2("abc");

    ASSERT(str1 == str2.c_str());
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_c_strings();
        total_errors += test_fixed_c_strings();
        total_errors += test_u8_strings();
        total_errors += test_u8_regex();
        total_errors += test_optional();
        total_errors += test_span();
        total_errors += test_range();
        total_errors += test_vector();
        total_errors += test_files();
        total_errors += test_hashmap();
        total_errors += test_hashset();
        total_errors += test_variant();
        total_errors += test_format();
        total_errors += test_sort();
        total_errors += test_fixedvector();
        total_errors += test_btree();
        total_errors += test_partition();
        total_errors += test_unicode();
        total_errors += test_threading();
        total_errors += test_std_mixing();
    } catch(const pystd2026::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
        return 42;
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
