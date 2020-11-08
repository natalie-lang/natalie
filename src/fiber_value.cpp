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

extern "C" {

void fiber_exit() {
    // FIXME: this should be the function that calls fiber_asm_switch but I don't know how to do that yet :-)
    abort(); // fiber_asm_switch should never return for an exiting fiber.
}

void fiber_wrapper_func(Natalie::Env *env, Natalie::FiberValue *fiber) {
    fiber->set_status(Natalie::FiberValue::Status::Active);
    assert(fiber->block());
    Natalie::Value **return_args = new Natalie::Value *[1];
    try {
        return_args[0] = NAT_RUN_BLOCK_WITHOUT_BREAK(env, fiber->block(), env->global_env()->fiber_argc(), env->global_env()->fiber_args(), nullptr);
    } catch (Natalie::ExceptionValue *exception) {
        Natalie::handle_top_level_exception(env, exception, false);
        exit(1);
    }
    fiber->set_status(Natalie::FiberValue::Status::Terminated);
    env->global_env()->reset_current_fiber();
    env->global_env()->set_fiber_args(1, return_args);
    // FIXME: maybe this should be done in fiber_exit, but I'm not sure how to get the args needed there
    fiber_asm_switch(env->global_env()->main_fiber(env)->fiber(), fiber->fiber(), 0, env, fiber);
}

/* Used to handle the correct stack alignment on Mac OS X, which requires a
16-byte aligned stack. The process returns here from its "main" function,
leaving the stack at 16-byte alignment. The call instruction then places a
return address on the stack, making the stack correctly aligned for the
process_exit function. */
asm(".globl " NAT_ASM_PREFIX "asm_call_fiber_exit\n" NAT_ASM_PREFIX
    "asm_call_fiber_exit:\n"
    "\tcall " NAT_ASM_PREFIX "fiber_exit\n");

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

    // save registers: rbx rbp r12 r13 r14 r15 (rsp into structure)
    "\tpushq %rbx\n"
    "\tpushq %rbp\n"
    "\tpushq %r12\n"
    "\tpushq %r13\n"
    "\tpushq %r14\n"
    "\tpushq %r15\n"
    "\tmovq %rsp, (%rsi)\n"

    // move args for function
    "\tmovq %rdi, %r15\n"
    "\tmovq %rcx, %rdi\n" // Env *
    "\tmovq %r8, %rsi\n" // Fiber *

    // swap stack
    "\tmovq (%r15), %rsp\n"

    // restore registers
    "\tpopq %r15\n"
    "\tpopq %r14\n"
    "\tpopq %r13\n"
    "\tpopq %r12\n"
    "\tpopq %rbp\n"
    "\tpopq %rbx\n"

    // return to the "next" fiber with eax set to return_value
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
