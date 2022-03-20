#include "natalie_parser/creator/debug_creator.hpp"
#include "natalie_parser/parser.hpp"

using namespace NatalieParser;

void test() {
    TM::SharedPtr<TM::String> code = new String { "'hello'" };
    TM::SharedPtr<TM::String> file = new String { "(string)" };
    auto parser = Parser { code, file };
    auto tree = parser.tree();
    auto creator = DebugCreator { tree };
    tree->transform(&creator);
    printf("%s\n", creator.to_string().c_str());
    delete tree;
}

int main() {
    test();
    return 0;
}
