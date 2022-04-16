#include "fragments.hpp"
#include "natalie_parser/creator/debug_creator.hpp"
#include "natalie_parser/parser.hpp"

using namespace NatalieParser;

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

TM::String test_code(TM::String &code, TM::String &path) {
    TM::SharedPtr<TM::String> code_ptr = new String { code };
    TM::SharedPtr<TM::String> file = new String { path };
    auto parser = Parser { code_ptr, file };
    Node *tree = parser.tree();
    auto creator = DebugCreator {};
    tree->transform(&creator);
    auto result = creator.to_string();
    delete tree;
    return result;
}

void test_file(TM::String &path, size_t expected_output_size) {
    auto code = read_file(path);
    auto output = test_code(code, path);
    if (expected_output_size > 0 && output.size() != expected_output_size) {
        printf("Expected output to be %zu bytes, but it was %zu bytes.\n", expected_output_size, output.size());
        printf("%s\n", output.c_str());
        abort();
    }
}

int main() {
    try {
        auto path = String("test/support/boardslam.rb");
        test_file(path, 4371);
        auto fragments = build_fragments();
        auto string_path = String("(string)");
        for (auto fragment : *fragments) {
            test_code(fragment, string_path);
        }
        delete fragments;
    } catch (NatalieParser::Parser::SyntaxError &e) {
        printf("SyntaxError: %s\n", e.message());
        return 1;
    }
    return 0;
}
