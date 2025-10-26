
#include <string>
#include "/Users/alisedighi/Desktop/llvm/src/EvaLLVM.h"


int main(int argc, char const *argv[])
{

    std::string program = R"(42)";


    EvaLLVM vm{};

    vm.exec(program);
    return 0;
}
