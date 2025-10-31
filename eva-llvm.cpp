
#include <string>
#include "/Users/alisedighi/Desktop/llvm/src/EvaLLVM.h"


int main(int argc, char const *argv[])
{

    std::string program = R"(
       //(printf "Value: %d\n" 42)
       //(printf "True: %d\n" false)

       (var VERSION 42)


    )";


    EvaLLVM vm{};


    vm.exec(program);
    return 0;
}
