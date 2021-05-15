/* Copyright (c) 2020 Evan Jones
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ----
 *
 * Large portions of this file were copied from Even Jones' libfiber library
 * made available at: https://github.com/evanj/libfiber
 */

#include "natalie.hpp"

namespace Natalie {

FiberValue *FiberValue::initialize(Env *env, Block *block) {
    assert(this != env->global_env()->main_fiber(env)); // can never be main fiber
    env->assert_block_given(block);
    create_stack(env, STACK_SIZE);
    m_stack_base = Natalie::Heap::the().start_of_stack();
    m_block = block;
    return this;
}

ValuePtr FiberValue::yield(Env *env, size_t argc, ValuePtr *args) {
    auto main_fiber = env->global_env()->main_fiber(env);
    auto current_fiber = env->global_env()->current_fiber();
    current_fiber->set_status(Status::Suspended);
    env->global_env()->reset_current_fiber();
    env->global_env()->set_fiber_args(argc, args);
    Heap::the().set_start_of_stack(main_fiber->stack_base());
    fiber_asm_switch(main_fiber->fiber(), current_fiber->fiber(), 0, env, main_fiber);
    argc = env->global_env()->fiber_argc();
    args = env->global_env()->fiber_args();
    if (argc == 0) {
        return env->nil_obj();
    } else if (argc == 1) {
        return args[0];
    } else {
        return new ArrayValue { env, argc, args };
    }
}

}

extern "C" {

void fiber_exit() {
    // fiber_asm_switch should never return for an exiting fiber.
    NAT_UNREACHABLE();
}

void fiber_wrapper_func(Natalie::Env *env, Natalie::FiberValue *fiber) {
    Natalie::Heap::the().set_start_of_stack(fiber->stack_base());
    fiber->set_status(Natalie::FiberValue::Status::Active);
    assert(fiber->block());
    Natalie::ValuePtr *return_args = new Natalie::ValuePtr[1];
    try {
        return_args[0] = NAT_RUN_BLOCK_WITHOUT_BREAK(env, fiber->block(), env->global_env()->fiber_argc(), env->global_env()->fiber_args(), nullptr);
    } catch (Natalie::ExceptionValue *exception) {
        Natalie::handle_top_level_exception(env, exception, false);
        exit(1);
    }
    fiber->set_status(Natalie::FiberValue::Status::Terminated);
    env->global_env()->reset_current_fiber();
    env->global_env()->set_fiber_args(1, return_args);
    auto main_fiber = env->global_env()->main_fiber(env);
    Natalie::Heap::the().set_start_of_stack(main_fiber->stack_base());
    fiber_asm_switch(main_fiber->fiber(), fiber->fiber(), 0, env, fiber);
}

void Natalie::FiberValue::visit_children(Visitor &visitor) {
    Value::visit_children(visitor);
    visitor.visit(m_block);
    // TODO: need to visit fiber_stack_struct m_fiber
    // ...and m_stack_base possibly too
}

#ifdef __x86_64
/* arguments:
 * rdi: next fiber
 * rsi: current fiber
 * rdx: return value
 * rcx: Natalie::Env *
 * r8: Natalie::Fiber *
 * */
asm(".globl " NAT_ASM_PREFIX "fiber_asm_switch\n" NAT_ASM_PREFIX "fiber_asm_switch:\n"
#ifndef __APPLE__
    "\t.type fiber_asm_switch, @function\n"
#endif
    // Move return value into rax
    "\tmovq %rdx, %rax\n"

    // save registers to current stack
    "\tpushq %rbx\n"
    "\tpushq %rbp\n"
    "\tpushq %r12\n"
    "\tpushq %r13\n"
    "\tpushq %r14\n"
    "\tpushq %r15\n"

    // save current stack pointer into fiber struct
    "\tmovq %rsp, (%rsi)\n"

    // move args for function
    "\tmovq %rdi, %r15\n"
    "\tmovq %rcx, %rdi\n" // Env *
    "\tmovq %r8, %rsi\n" // Fiber *

    // swap stack
    "\tmovq (%r15), %rsp\n"

    // restore registers from new stack
    "\tpopq %r15\n"
    "\tpopq %r14\n"
    "\tpopq %r13\n"
    "\tpopq %r12\n"
    "\tpopq %rbp\n"
    "\tpopq %rbx\n"

    // return to the "next" fiber with rax set to return_value
    "\tret\n");
#else
asm(".globl " NAT_ASM_PREFIX "fiber_asm_switch\n" NAT_ASM_PREFIX "fiber_asm_switch:\n"
#ifndef __APPLE__
    "\t.type fiber_asm_switch, @function\n"
#endif
    // Move return value into eax, current pointer into ecx, next pointer into edx
    "\tmov 12(%esp), %eax\n"
    "\tmov 8(%esp), %ecx\n"
    "\tmov 4(%esp), %edx\n"

    // save registers: ebx ebp esi edi (esp into structure)
    "\tpush %ebx\n"
    "\tpush %ebp\n"
    "\tpush %esi\n"
    "\tpush %edi\n"
    "\tmov %esp, (%ecx)\n"

    // TODO: pass args

    // restore registers
    "\tmov (%edx), %esp\n"
    "\tpop %edi\n"
    "\tpop %esi\n"
    "\tpop %ebp\n"
    "\tpop %ebx\n"

    // return to the "next" fiber with eax set to return_value
    "\tret\n");
#endif
}
