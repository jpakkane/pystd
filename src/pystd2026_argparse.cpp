// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026_argparse.hpp>
#include <errno.h>
#include <assert.h>

namespace pystd2026 {

void ArgParse::add_argument(Argument a) {
    for(const auto &arg : arguments) {
        if(a.name == arg.name) {
            throw PyException("Duplicate option name in ArgParse.");
        }
        if(!a.long_arg.is_empty() && a.long_arg == arg.long_arg) {
            throw PyException("Long argument name already taken.");
        }
        if(a.short_arg && a.short_arg == arg.short_arg) {
            throw PyException("Short argument character already taken.");
        }
    }
    arguments.push_back(move(a));
}

static void
append_entry(ParseResult &pr, ArgumentType atype, const CString &name, const ArgumentValue &v) {
    switch(atype) {
    case ArgumentType::String:
        pr.values.emplace_back(name, v.get<CString>());
        break;
    case ArgumentType::Integer:
        pr.values.emplace_back(name, v.get<int64_t>());
        break;
    case ArgumentType::Boolean:
        pr.values.emplace_back(name, v.get<bool>());
        break;
    case ArgumentType::StringArray:
        pr.values.emplace_back(name, v.get<Vector<CString>>());
        break;
    default:
        abort();
    }
}

static void append_entry_valueless(ParseResult &pr, ArgumentType atype, const CString &name) {
    switch(atype) {
    case ArgumentType::String:
        pr.values.emplace_back(name, CString{});
        break;
    case ArgumentType::Integer:
        pr.values.emplace_back(name, int64_t{});
        break;
    case ArgumentType::Boolean:
        pr.values.emplace_back(name, bool{});
        break;
    case ArgumentType::StringArray:
        pr.values.emplace_back(name, Vector<CString>{});
        break;
    default:
        abort();
    }
}

ParseResult ArgParse::create_value_obj() const {
    ParseResult pr;
    for(const auto &arg : arguments) {
        if(arg.default_value) {
            const auto &def = arg.default_value.value();
            append_entry(pr, arg.type, arg.name, def);
        }
    }
    return pr;
}

static void update_value(const Argument &atype, ArgValue &vobj, CStringView source) {
    switch(atype.aaction) {
    case ArgumentAction::Store:
        switch(atype.type) {
        case ArgumentType::String: {
            vobj.v = CString(source);
            break;
        }
        case ArgumentType::Boolean:
            break;
        case ArgumentType::Integer: {
            // FIXME, needed for null terminator.
            CString tmp(source);
            errno = 0;
            int64_t intval = strtol(tmp.data(), nullptr, 10);
            if(errno != 0) {
                throw PyException(strerror(errno));
            }
            if(atype.minval && intval < atype.minval.value()) {
                throw PyException("Argument value less than min value.");
            }
            if(atype.maxval && intval > atype.maxval.value()) {
                throw PyException("Argument value larger than max value.");
            }
            vobj.v = intval;
        } break;
        case ArgumentType::StringArray: {
            // Seems a bit silly, but why not?
            Vector<CString> new_array;
            new_array.emplace_back(source);
            vobj.v = move(new_array);
        } break;
        default:
            abort();
        }
        break;
    case ArgumentAction::StoreFalse:
        vobj.v = false;
        break;
    case ArgumentAction::StoreTrue:
        vobj.v = true;
        break;
    case ArgumentAction::AppendArray:
        vobj.v.get<Vector<CString>>().emplace_back(source);
        break;
    default:
        abort();
    }
}

void ArgParse::argument_matched(ParseResult &pr, size_t match_index, CStringView value) {
    const auto &arg = arguments[match_index];
    auto *existing = pr.find(arg.name.view());
    if(existing) {
        //
    } else {
        append_entry_valueless(pr, arg.type, arg.name);
        existing = &pr.values.back();
    }

    update_value(arg, *existing, value);
}

Optional<ParseOutput> ArgParse::parse_args(int argc, const char **argv) {
    progname = argv[0];
    ParseResult result = create_value_obj();
    for(int i = 1; i < argc; ++i) {
        CStringView current(argv[i]);
        if(current.is_empty()) {
            printf("Empty argument in argumentlist.\n");
            exit(1);
        }
        if(current == "--help" || current == "-h") {
            print_help_and_exit();
        }
        if(current == "--") {
            for(int k = i + 1; k < argc; ++k) {
                extra_args.push_back(CString(argv[k]));
            }
            break;
        } else if(current.starts_with("--")) {
            const auto consumed_extra_args = process_long_argument(argc, argv, result, i, current);
            i += consumed_extra_args;
        } else if(current.starts_with("-")) {
            const auto consumed_extra_args = process_short_argument(argc, argv, result, i, current);
            i += consumed_extra_args;
        } else {
            if(store_posargs) {
                posargs.push_back(CString(current));
            } else {
                fprintf(stderr, "Unknown argument at location %d: %s\n", i, argv[i]);
                exit(1);
            }
        }
    }
    result.extra_args = move(extra_args);
    result.posargs = move(posargs);
    return Optional<ParseOutput>(ParseOutput(pystd2026::move(result)));
}

Optional<size_t> ArgParse::find_long_argument(CStringView optname) {
    for(size_t i = 0; i < arguments.size(); ++i) {
        if(arguments[i].long_arg == optname) {
            return i;
        }
    }
    return Optional<size_t>{};
}

Optional<size_t> ArgParse::find_short_argument(char c) {
    for(size_t i = 0; i < arguments.size(); ++i) {
        if(arguments[i].short_arg == c) {
            return i;
        }
    }
    return Optional<size_t>{};
}

int ArgParse::process_long_argument(
    int argc, const char **argv, ParseResult &result, int i, CStringView current) {
    CStringView keypart;
    CStringView valuepart;
    auto splitpoint = current.find('=');
    int extra_consumed_args = 0;
    CStringView option_name;

    if(splitpoint != (size_t)-1) {
        keypart = current.substr(0, splitpoint);
    } else {
        keypart = current;
    }

    const auto opt_result = find_long_argument(keypart);

    if(!opt_result) {
        fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        exit(1);
    }
    const auto match_index = opt_result.value();
    const auto &arg = arguments[match_index];

    if(arg.needs_parameter()) {
        if(splitpoint != (size_t)-1) {
            valuepart = current.substr(splitpoint + 1);
        } else {
            if(i + 1 >= argc) {
                fprintf(stderr, "Last entry on the command line would need a further argument.\n");
                exit(1);
            }
            valuepart = argv[i + 1];
            extra_consumed_args = 1;
        }
    }
    argument_matched(result, match_index, valuepart);

    return extra_consumed_args;
}

int ArgParse::process_short_argument(
    int argc, const char **argv, ParseResult &result, int i, CStringView current) {
    CStringView valuepart;
    assert(current.starts_with("-"));
    const char shortchar = current[1];
    int extra_consumed_args = 0;

    const auto opt_result = find_short_argument(shortchar);
    if(!opt_result) {
        fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        exit(1);
    }
    const auto match_index = opt_result.value();
    const auto &arg = arguments[match_index];

    if(arg.needs_parameter()) {
        if(current.size() == 2) {
            valuepart = argv[i + 1];
            extra_consumed_args = 1;
        } else {
            // FIXME, check for "-x=something".
            valuepart = current.substr(2);
        }
    } else {
    }
    argument_matched(result, match_index, valuepart);

    return extra_consumed_args;
}

void ArgParse::print_help_and_exit() {
    printf("Usage: %s [-h]\n", progname);
    printf("\n");
    if(!description.is_empty()) {
        printf("%s\n", description.c_str());
    }
    if(!arguments.is_empty()) {
        printf("\nArguments\n\n");
    }
    for(const auto &a : arguments) {
        if(!a.long_arg.is_empty()) {
            printf("%s ", a.long_arg.c_str());
        }
        if(a.short_arg != 0) {
            printf("-%c ", a.short_arg);
        }
        if(!a.help.is_empty()) {
            printf("          %s", a.help.c_str());
        }
        if(a.default_value) {
            printf(" (default: ");
            const auto &v = a.default_value.value();
            if(auto *b = v.get_if<bool>()) {
                printf("%s", *b ? "true" : "false");
            } else if(auto *i = v.get_if<int64_t>()) {
                printf("%ld", *i);
            } else if(auto *s = v.get_if<CString>()) {
                printf("%s", s->c_str());
            } else if(auto *arr = v.get_if<Vector<CString>>()) {
                if(arr->is_empty()) {
                    printf("empty array");
                } else {
                    printf("[");
                    for(size_t i = 0; i < arr->size(); ++i) {
                        printf("%s", (*arr)[i].c_str());
                        if(i != arr->size() - 1) {
                            printf(", ");
                        }
                    }
                    printf("]");
                }
            } else {
                abort();
            }
            printf(")");
        }
        printf("\n");
    }
    exit(0);
}

const ArgumentValue *ParseOutput::value_of(CStringView arg_name) const {
    for(const auto &s : pr.values) {
        if(s.name == arg_name) {
            return &s.v;
        }
    }
    return nullptr;
}

ArgumentValue *ParseOutput::value_of(CStringView arg_name) {
    for(auto &s : pr.values) {
        if(s.name == arg_name) {
            return &s.v;
        }
    }
    return nullptr;
}

} // namespace pystd2026
