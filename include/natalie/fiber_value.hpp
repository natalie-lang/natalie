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

#pragma once

#include <assert.h>
#include <stdint.h>

#include "natalie/array_value.hpp"
#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/nil_value.hpp"
#include "natalie/value.hpp"

extern "C" {
typedef struct {
    void **stack;
} fiber_stack_struct;

extern int fiber_asm_switch(fiber_stack_struct *, fiber_stack_struct *, int, Natalie::Env *, Natalie::FiberValue *);
extern void fiber_wrapper_func(Natalie::Env *, Natalie::FiberValue *);
extern void fiber_exit();
}

namespace Natalie {

struct FiberValue : Value {
    enum class Status {
        Created,
        Active,
        Suspended,
        Terminated,
    };

    FiberValue(Env *env)
        : Value { Value::Type::Fiber, env->Object()->const_fetch("Fiber")->as_class() } { }

    FiberValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Fiber, klass } { }

    const int STACK_SIZE = 1024 * 1024;

    FiberValue *initialize(Env *env, Block *block) {
        NAT_ASSERT_BLOCK();
        create_stack(env, STACK_SIZE);
        m_stack_base.mem_base = m_stack_bottom;
        m_block = block;
        return this;
    }

    void set_current_stack_bottom() {
        GC_get_my_stackbottom(&m_stack_base);
    }

    static Value *yield(Env *env, ssize_t argc, Value **args) {
        auto main_fiber = env->global_env()->main_fiber(env);
        auto current_fiber = env->global_env()->current_fiber();
        current_fiber->set_status(Status::Suspended);
        env->global_env()->reset_current_fiber();
        env->global_env()->set_fiber_args(argc, args);
        GC_set_stackbottom(nullptr, main_fiber->stack_base());
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

    void create_stack(Env *env, int stack_size) {
#ifdef __x86_64
        // x86-64: rbx, rbp, r12, r13, r14, r15
        static const int NUM_REGISTERS = 6;
#else
        // x86: ebx, ebp, edi, esi
        static const int NUM_REGISTERS = 4;
#endif
        assert(stack_size % 16 == 0);
        m_stack_bottom = malloc(stack_size);
        if (m_stack_bottom == 0) {
            NAT_RAISE(env, "StandardError", "could not allocate stack for Fiber");
        }
        m_fiber.stack = (void **)((char *)m_stack_bottom + stack_size);
#ifdef __APPLE__
        assert((uintptr_t)m_fiber.stack % 16 == 0);
#endif

        // 4 bytes below 16-byte alignment: mac os x wants return address here so this points to a call instruction.
        *(--m_fiber.stack) = (void *)((uintptr_t)&fiber_exit);
        // 8 bytes below 16-byte alignment: will "return" to start this function
        *(--m_fiber.stack) = (void *)((uintptr_t)&fiber_wrapper_func); // Cast to avoid ISO C warnings.
        // push NULL words to initialize the registers loaded by fiber_asm_switch
        for (int i = 0; i < NUM_REGISTERS; ++i) {
            *(--m_fiber.stack) = 0;
        }
    }

    Value *resume(Env *env, ssize_t argc, Value **args) {
        if (m_status == Status::Terminated) {
            NAT_RAISE(env, "FiberError", "dead fiber called");
        }
        auto main_fiber = env->global_env()->main_fiber(env);
        env->global_env()->set_current_fiber(this);
        env->global_env()->set_fiber_args(argc, args);
        GC_set_stackbottom(nullptr, stack_base());
        fiber_asm_switch(fiber(), main_fiber->fiber(), 0, env, this);
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

    void *stack_bottom() { return m_stack_bottom; }

    fiber_stack_struct *fiber() { return &m_fiber; }
    Block *block() { return m_block; }
    void set_status(Status status) { m_status = status; }

    struct GC_stack_base *stack_base() {
        return &m_stack_base;
    }

private:
    Block *m_block { nullptr };
    ::fiber_stack_struct m_fiber {};
    void *m_stack_bottom { nullptr };
    Status m_status { Status::Created };
    struct GC_stack_base m_stack_base;
};

}
