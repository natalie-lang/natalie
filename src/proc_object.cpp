#include "natalie.hpp"
#include "tm/owned_ptr.hpp"

namespace Natalie {

Value ProcObject::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

Value ProcObject::call(Env *env, Args &&args, Block *block) {
    assert(m_block);
    if (is_lambda() && m_break_point != 0) {
        try {
            return m_block->run(env, std::move(args), block);
        } catch (ExceptionObject *exception) {
            if (exception->is_local_jump_error_with_break_point(m_break_point))
                return exception->send(env, "exit_value"_s);
            throw exception;
        }
    }
    return m_block->run(env, std::move(args), block);
}

bool ProcObject::equal_value(Value other) const {
    return other.is_proc() && other.as_proc()->m_block == m_block;
}

static Value compose_ltlt(Env *env, Value self, Args &&args, Block *block) {
    auto block_var = ProcObject::from_block_maybe(block);
    auto call = "call"_s;
    auto other_call_result = env->outer()->var_get("other", 0).send(env, call, std::move(args), to_block(env, block_var));
    return self.send(env, call, { other_call_result });
}

static Value compose_gtgt(Env *env, Value self, Args &&args, Block *block) {
    auto block_var = ProcObject::from_block_maybe(block);
    auto call = "call"_s;
    auto self_call_result = self.send(env, call, std::move(args), to_block(env, block_var));
    return env->outer()->var_get("other", 0).send(env, call, { self_call_result });
}

Value ProcObject::ltlt(Env *env, Value other) {
    if (!other.respond_to(env, "call"_s))
        env->raise("TypeError", "callable object is expected");

    env->var_set("other", 0, true, other);
    auto block = Block::create(*env, this, compose_ltlt, -1);
    if (other.is_proc() && other.as_proc()->is_lambda())
        block->set_type(Block::BlockType::Lambda);
    return ProcObject::create(block);
}

Value ProcObject::gtgt(Env *env, Value other) {
    if (!other.respond_to(env, "call"_s))
        env->raise("TypeError", "callable object is expected");

    env->var_set("other", 0, true, other);
    auto block = Block::create(*env, this, compose_gtgt, -1);
    if (is_lambda())
        block->set_type(Block::BlockType::Lambda);
    return ProcObject::create(block);
}

Value ProcObject::ruby2_keywords(Env *env) {
    auto block_wrapper = [](Env *env, Value self, Args &&args, Block *block) -> Value {
        auto kwargs = args.has_keyword_hash() ? args.pop_keyword_hash() : HashObject::create();
        auto new_args = args.to_array_for_block(env, 0, -1, true);
        if (!kwargs->is_empty())
            new_args->push(HashObject::ruby2_keywords_hash(env, kwargs));
        auto old_block = env->outer()->var_get("old_block", 1).as_proc();
        old_block->block()->set_self(self);
        return old_block->call(env, std::move(new_args), block);
    };

    OwnedPtr<Env> inner_env { Env::create(*env) };
    inner_env->var_set("old_block", 1, true, ProcObject::create(m_block));
    m_block = Block::create(std::move(inner_env), this, block_wrapper, -1);

    return this;
}

Value ProcObject::source_location() {
    assert(m_block);
    auto file = m_block->env()->file();
    if (file == nullptr) return Value::nil();
    return ArrayObject::create({ StringObject::create(file), Value::integer(static_cast<nat_int_t>(m_block->env()->line())) });
}

StringObject *ProcObject::to_s(Env *env) {
    assert(m_block);
    String suffix {};
    if (m_block->env()->file())
        suffix.append(String::format(" {}:{}", m_block->env()->file(), m_block->env()->line()));
    if (is_lambda() || m_block->is_from_method())
        suffix.append(" (lambda)");
    if (m_block->self().is_symbol())
        suffix.append(String::format(" (&:{})", m_block->self().as_symbol()->string()));
    auto str = String::format("#<{}:{}{}>", m_klass->inspect_module(), String::hex(object_id(this), String::HexFormat::LowercaseAndPrefixed), suffix);
    return StringObject::create(std::move(str), Encoding::ASCII_8BIT);
}

}
