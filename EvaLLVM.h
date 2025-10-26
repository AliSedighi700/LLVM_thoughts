#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"



class EvaLLVM
{
public:

    EvaLLVM()
    {
        moduleInit();
        setupExternalFunctions();
    }

    void exec(const std::string& program)
    {

        //1. pars the program
        //auto ast = parser->parser(program);


        //2. Compile to LLVM IR
        //for the starting point we do not have the
        //ast so lets just return 42 like before
        compile();

        //print generated code
        module -> print(llvm::outs(), nullptr);

        //3.save module IR to file
        saveModuleToFile("./out.ll");

    }

private:

    void compile (/* TODO: std */)
    {
        //1.create main function
        fn = createFunction("main",
                llvm::FunctionType::get(/*return type*/builder->getInt32Ty(),
                                        /*vararg*/ false ));

        //llvm:FunctionType::get has multiple ctor one of them just get
        //the retunr type.

        //2.compile main function
        //the gen function accept any AST and will be
        //the recursive compiler (*******)
        auto result = gen(/* ast */);


        //3. when the result is ready we should
        //return it. since we wanna return 42,
        //we should cast what ever the result is
        //to int.
        //auto i32Result = // we commented that because it will cast str to int
            //builder -> CreateIntCast(result, builder->getInt32Ty(), true);
        // the boolian if for withe rthe i32Result is signed.

        //create ret instruction
        builder -> CreateRet(builder->getInt32(0));
    }


    //now lets introduce our gen method(*******)
    //the result of the gen function should be LLVM value.
    //this is a very basic type which can accept pretty much
    //everything might be numbers, string , etc
    //here the result is a ineger number
    llvm::Value *gen(/* ast */)
    {
        //return builder -> getInt32(42);


        //Now intead of an int, lets harcode a
        //string. All literal string that is the strings
        //containing only characters are considers global
        //strings and the builder has a methd for that.

        auto str = builder -> CreateGlobalString("Hello, world\n");

        //now with the printf definition of the printf,
        //we print the what builder is giving to us.- Hello, world
        //first we should tell the module to get the function
        auto printn = module -> getFunction("printf");

        //then we need to define the arguments
        std::vector<llvm::Value*> args{str};

        //Then we need to use the creatCall method to call the printf
        return builder->CreateCall(printn, args);

        //the result will be
        //; ModuleID = 'EvaLLVM'
        //source_filename = "EvaLLVM"
        //
        //@0 = private unnamed_addr constant [14 x i8] c"Hello, world\0A\00", align 1

        //declare i32 @printf(ptr, ...)

        //define i32 @main() {
        //entry:
          //%0 = call i32 (ptr, ...) @printf(ptr @0) <-- here 0 is a pointer to characer
          //                                             we see that becuase we passed the
          //                                             args
          //ret i32 0
        //}
        //Hello, world <-- the actual result. The string is printed
        //0


    }

    //Now we said that we can distinguish the function from the
    //function declation and the definition. so if a external
    //funcion comes from another module and so on. We should
    //be possible to define an external function.

    void setupExternalFunctions()
    {
        //from stl, printf accepts the format string,
        //which is the null terminated charcter pointer string
        //and the char class from C++ us just an alias for
        //the byte.
        //The way we do that in llvm is exactly the same.
        //we create an extra type like
        //auto bytePtrTry = builder -> getInt8Ty() -> getPointerTo();
        auto bytePtrTry = llvm::PointerType::get(*ctx,0);


        //to declare a function like printf we use getOrInsertFunction.
        module -> getOrInsertFunction("printf",
                //int printf(const char* format, ...)
                llvm::FunctionType::get(
                    /*return type*/ builder->getInt32Ty(),
                    /*format arg*/ bytePtrTry,
                    /*vararg*/true));
    }





    //cerateFunction:
    //Remembber the gloal is to see how the llvm compiles a .ll file.
    //we said we need three things for that.
    //1. module , 2.context , 3.IR builder
    //we also said that inside of the module we have
    //Function Definitions
    //now we should define the CreateFunction function


    //WHAT WE NEED TO CREATE A FUNCTION?
    //
    //1. function type.
    //   what is function type? It is the type with parameters and
    //   return type.
    //
    //2. Entry Basics Block.
    //   which create the function body.
    //   We can think about a Block as a list of instructions:
    //   A function has always the entry block. We will use an
    //   explicit (entry:)label for the block.
    //   A function can have one or multi blocks. Some of the blocks
    //   may contain the so-called TERMINATOR INSTRUCTIONS,
    //   such as branch(if-else, switch, .) which can be
    //   transit to other blocks. So in general A functiin
    //   is a n abstration which has arguments, entry blocks,
    //   and multiple optional blocks.
    //
    //   Here for our createFunction we need just one entry block.
    //   For that we need the name and the type of the function.

    llvm::Function *createFunction(const std::string& fnName,
                                   llvm::FunctionType* fnType)
    {
        //needless to say we have function decleration and
        //functoion definition.
        //function decleration is only the function signiture that
        //is the FUNCTION PROTOTYPE, which has information about
        //it's type, that is the return and parameter type and
        //the function name.

        //The function declation may already be exist, to
        //check this we can call the method on the module
        //which is getFunction.
        auto fn = module -> getFunction(fnName);

        //if it does not exit we create it.
        if(fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType);
        }

        //Now the function is define, we can create its
        //basic blocks.
        createFunctionBlock(fn);
        return fn;

    }

    llvm::Function* createFunctionProto(const std::string& fnName,
                                       llvm::FunctionType* fnType)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage,
                                         fnName, *module);

        //once the function is defined we can call varify that.
        verifyFunction(*fn);
        return fn;
    }


    void createFunctionBlock(llvm::Function* fn)
    {
        auto entry = createBB("entry", fn); // create Basic Block

        //The builder that is the code emitter works as a state machine.
        //once we allocated the new block, we have to explicitly tell
        //the builder to emit the code exactly to this block, so
        builder -> SetInsertPoint(entry);
    }


    // to create the basic block, which is called the appropriate
    // method in the basic block class, create, passing the name
    // and the function.
    llvm::BasicBlock* createBB(std::string name, llvm::Function* fn = nullptr)
    {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }


    void saveModuleToFile(const std::string& fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    void moduleInit()
    {
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    //currently compiling function
    llvm::Function *fn;

    std::unique_ptr<llvm::LLVMContext> ctx{};
    std::unique_ptr<llvm::Module> module{};
    std::unique_ptr<llvm::IRBuilder<>> builder{};

};



#endif
