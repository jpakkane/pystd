// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026_argparse.hpp>

int main(int argc, const char **argv) {
    pystd2026::ArgParse parser(pystd2026::U8String("Test application for command line parser."));

    parser.add_argument({.name{"foo"}, .long_arg{"--foo"}, .help{"The foo value to use."}});
    parser.add_argument({.name{"bar"}, .long_arg{"--bar"}, .help{"The bar to barnicate."}});

    parser.add_argument({.name{"size"},
                         .long_arg{"--size"},
                         .type = pystd2026::ArgumentType::Integer,
                         .minval{0},
                         .maxval{9}});

    pystd2026::Vector<pystd2026::CString> default_array;
    default_array.emplace_back("one");
    default_array.emplace_back("two");

    parser.add_argument({.name{"include"},
                         .short_arg = 'I',
                         .long_arg{"--include"},
                         .type = pystd2026::ArgumentType::StringArray,
                         .default_value{pystd2026::move(default_array)},
                         .aaction = pystd2026::ArgumentAction::AppendArray});

    parser.add_argument({
        .name{"verbose"},
        .short_arg = 'v',
        .help{"Verbose mode"},
        .type = pystd2026::ArgumentType::Boolean,
        .default_value{false},
        .aaction = pystd2026::ArgumentAction::StoreTrue,
    });

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
