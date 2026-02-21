// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026_argparse.hpp>

int main(int argc, const char **argv) {
    pystd2026::ArgParse parser(pystd2026::U8String("Test application for command line parser."));

    pystd2026::Argument foo;
    foo.long_arg = "--foo";
    foo.name = "foo";
    foo.help = pystd2026::U8String("The foo value to use.");
    parser.add_argument(foo);

    pystd2026::Argument bar;
    bar.long_arg = "--bar";
    bar.name = "bar";
    bar.help = pystd2026::U8String("The bar to barnicate.");
    parser.add_argument(bar);

    pystd2026::Argument intval;
    intval.long_arg = "--size";
    intval.name = "size";
    intval.type = pystd2026::ArgumentType::Integer;
    intval.minval = 0;
    intval.maxval = 9;
    parser.add_argument(intval);

    pystd2026::Argument include;
    include.long_arg = "--include";
    include.short_arg = 'I';
    include.name = "include";
    include.type = pystd2026::ArgumentType::StringArray;
    include.aaction = pystd2026::ArgumentAction::AppendArray;
    pystd2026::Vector<pystd2026::CString> default_array;
    default_array.emplace_back("one");
    default_array.emplace_back("two");
    include.default_value = pystd2026::move(default_array);

    parser.add_argument(include);

    pystd2026::Argument verbose;
    verbose.name = "verbose";
    verbose.short_arg = 'v';
    verbose.help = pystd2026::U8String("Verbose mode");
    verbose.type = pystd2026::ArgumentType::Boolean;
    verbose.aaction = pystd2026::ArgumentAction::StoreTrue;
    verbose.default_value = false;

    parser.add_argument(verbose);

    auto res_opt = parser.parse_args(argc, argv);
    auto &result = res_opt.value();

    const auto *r1 = result.value_of("foo");
    printf("Foo: %s\n", r1 ? r1->get<pystd2026::CString>().c_str() : "undef");
    const auto *r2 = result.value_of("bar");
    printf("Bar: %s\n", r2 ? r2->get<pystd2026::CString>().c_str() : "undef");
    const auto *r3 = result.value_of("size");
    printf("Size: %d\n", r3 ? (int)r3->get<int64_t>() : -1);
    const auto *r4 = result.value_of("include");
    if(r4) {
        const auto &arr = r4->get<pystd2026::Vector<pystd2026::CString>>();
        printf("Include: %d entries \n", (int)arr.size());
        for(const auto &v : arr) {
            printf(" %s\n", v.c_str());
        }
    }
    const auto *r5 = result.value_of("verbose");
    printf("Verbose: %d\n", r5->get<bool>() ? 1 : 0);
    return 0;
}
