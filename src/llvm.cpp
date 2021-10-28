#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/IRBuilder.h>
#include <stdio.h>

#include "natalie/parser.hpp"
#include "natalie/string.hpp"

using namespace llvm;
using namespace Natalie;

void walk_tree(Node *node, IRBuilder<> &builder, Function *printf_fn) {
    switch (node->type()) {
    case Node::Type::Block:
        for (auto n : static_cast<BlockNode *>(node)->nodes())
            walk_tree(n, builder, printf_fn);
        break;
    case Node::Type::Integer: {
        auto i64 = builder.getInt64Ty();
        auto num = ConstantInt::get(i64, static_cast<IntegerNode *>(node)->number());
        auto format_str = builder.CreateGlobalStringPtr("%lli\n");
        builder.CreateCall(printf_fn, { format_str, num });
        break;
    }
    case Node::Type::String: {
        auto str = static_cast<StringNode *>(node)->string()->c_str();
        auto string = builder.CreateGlobalStringPtr(str);
        auto format_str = builder.CreateGlobalStringPtr("%s\n");
        builder.CreateCall(printf_fn, { format_str, string });
        break;
    }
    default:
        printf("I don't know how to handle node type %d\n", (int)node->type());
        abort();
    }
}

int main(int argc, char **argv) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmParser();
    LLVMInitializeNativeAsmPrinter();

    assert(argc > 1);

    auto parser = Parser { new String(argv[1]), new String("(string)") };
    auto tree = parser.tree();

    auto context = std::make_unique<LLVMContext>();
    IRBuilder<> builder(*context);

    auto module = std::make_unique<Module>("hello", *context);

    // build a 'main' function
    auto i32 = builder.getInt32Ty();
    auto prototype = FunctionType::get(i32, false);
    Function *main_fn = Function::Create(prototype, Function::ExternalLinkage, "main", module.get());
    BasicBlock *body = BasicBlock::Create(*context, "body", main_fn);
    builder.SetInsertPoint(body);

    // use libc's printf function
    auto i8p = builder.getInt8PtrTy();
    auto printf_prototype = FunctionType::get(i8p, true);
    auto printf_fn = Function::Create(printf_prototype, Function::ExternalLinkage, "printf", module.get());

    // call printf with our string
    //auto format_str = builder.CreateGlobalStringPtr("hello world\n");
    //builder.CreateCall(printf_fn, { format_str });

    walk_tree(tree, builder, printf_fn);

    // return 0 from main
    auto ret = ConstantInt::get(i32, 0);
    builder.CreateRet(ret);

    // if you want to print the LLVM IR:
    //module->print(llvm::outs(), nullptr);
    //printf("\n\n------------------\n");

    // execute it!
    ExecutionEngine *executionEngine = EngineBuilder(std::move(module)).setEngineKind(llvm::EngineKind::JIT).create();
    Function *main = executionEngine->FindFunctionNamed(StringRef("main"));
    auto result = executionEngine->runFunction(main, {});

    // return the result
    return result.IntVal.getLimitedValue();
}
