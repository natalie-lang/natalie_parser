#include "fragments.hpp"
#include "natalie_parser/creator/debug_creator.hpp"
#include "natalie_parser/parser.hpp"

using namespace NatalieParser;

int passed = 0;
int failed = 0;

TM::String read_file(TM::String &path) {
    auto buf = TM::String();
    FILE *fp = fopen(path.c_str(), "r");
    assert(fp);
    char cbuf[1024];
    while (fgets(cbuf, 1024, fp))
        buf.append(cbuf);
    fclose(fp);
    return buf;
}

TM::String test_code(TM::String code, TM::String path = "(string)") {
    TM::SharedPtr<TM::String> code_ptr = new String { code };
    TM::SharedPtr<TM::String> file = new String { path };
    auto parser = Parser { code_ptr, file };
    auto tree = parser.tree();
    auto creator = DebugCreator {};
    tree->transform(&creator);
    auto result = creator.to_string();
    return result;
}

void test_code_with_syntax_error(TM::String code) {
    try {
        test_code(code);
    } catch (NatalieParser::Parser::SyntaxError &) {
        printf(".");
        return;
    }
    printf("\nExpected `%s' to raise a SyntaxError\n", code.c_str());
    abort();
}

void test_file(TM::String path, size_t expected_output_size) {
    auto code = read_file(path);
    auto output = test_code(code, path);
    if (output.size() == expected_output_size) {
        printf(".");
    } else {
        printf("\nExpected output to be %zu bytes, but it was %zu bytes.\n", expected_output_size, output.size());
        printf("%s\n", output.c_str());
        abort();
    }
}

void test_fragments() {
    auto fragments = build_fragments();
    for (auto fragment : *fragments) {
        test_code(fragment);
        printf(".");
    }
    delete fragments;
}

void test_fragments_with_syntax_errors() {
    auto fragments = build_fragments();
    for (auto fragment : *fragments) {
        test_code_with_syntax_error(fragment + "\n^");
        printf(".");
    }
    delete fragments;
}

void test_syntax_errors() {
    test_code_with_syntax_error("1 + ");
    test_code_with_syntax_error("foo(");
    test_code_with_syntax_error("1 2 3");
}

int main() {
    try {
        test_file("test/support/boardslam.rb", 4371);
        test_fragments();
    } catch (NatalieParser::Parser::SyntaxError &e) {
        printf("\nSyntaxError: %s\n", e.message());
        abort();
    }
    test_syntax_errors();
    test_fragments_with_syntax_errors();
    printf("\n");
    return 0;
}
