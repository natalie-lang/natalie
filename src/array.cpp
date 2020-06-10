#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Array_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 2);
    ArrayValue *ary = new ArrayValue { env };
    if (argc == 0) {
        return ary;
    }
    Value *size = args[0];
    if (NAT_TYPE(size) == Value::Type::Array) {
        // given an array, just return it
        return size;
    }
    NAT_ASSERT_TYPE(size, Value::Type::Integer, "Integer");
    Value *value = argc == 2 ? args[1] : NAT_NIL;
    for (int64_t i = 0; i < size->as_integer()->to_int64_t(); i++) {
        ary->push(value);
    }
    return ary;
}

// Array[]
Value *Array_square_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *ary = new ArrayValue { env };
    for (ssize_t i = 0; i < argc; i++) {
        ary->push(args[i]);
    }
    return ary;
}

Value *Array_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ArrayValue *self = self_value->as_array();
    StringValue *out = new StringValue { env, "[" };
    for (ssize_t i = 0; i < self->size(); i++) {
        Value *obj = (*self)[i];
        StringValue *repr = send(env, obj, "inspect", 0, NULL, NULL)->as_string();
        out->append_string(env, repr);
        if (i < self->size() - 1) {
            out->append(env, ", ");
        }
    }
    out->append_char(env, ']');
    return out;
}

Value *Array_ltlt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *arg = args[0];
    self->push(arg);
    return self;
}

Value *Array_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *arg = args[0]->as_array();
    ArrayValue *new_array = ArrayValue::copy(env, *self);
    new_array->concat(*arg);
    return new_array;
}

Value *Array_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *arg = args[0]->as_array();
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *self) {
        int found = 0;
        for (auto &compare_item : *arg) {
            if (send(env, item, "==", 1, &compare_item, nullptr)->is_truthy()) {
                found = 1;
                break;
            }
        }
        if (!found) {
            new_array->push(item);
        }
    }
    return new_array;
}

Value *Array_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    ArrayValue *self = self_value->as_array();
    Value *index_obj = args[0];
    if (NAT_TYPE(index_obj) == Value::Type::Integer) {
        ssize_t index = index_obj->as_integer()->to_int64_t();
        if (index < 0) {
            index = self->size() + index;
        }
        if (index < 0 || index >= self->size()) {
            return NAT_NIL;
        } else if (argc == 1) {
            return (*self)[index];
        }
        Value *size = args[1];
        NAT_ASSERT_TYPE(size, Value::Type::Integer, "Integer");
        ssize_t end = index + size->as_integer()->to_int64_t();
        ssize_t max = self->size();
        end = end > max ? max : end;
        ArrayValue *result = new ArrayValue { env };
        for (ssize_t i = index; i < end; i++) {
            result->push((*self)[i]);
        }
        return result;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        Value *begin_obj = range->range_begin;
        Value *end_obj = range->range_end;
        NAT_ASSERT_TYPE(begin_obj, Value::Type::Integer, "Integer");
        NAT_ASSERT_TYPE(end_obj, Value::Type::Integer, "Integer");
        int64_t begin = begin_obj->as_integer()->to_int64_t();
        int64_t end = end_obj->as_integer()->to_int64_t();
        if (begin < 0) {
            begin = self->size() + begin;
        }
        if (end < 0) {
            end = self->size() + end;
        }
        if (begin < 0 || end < 0) {
            return NAT_NIL;
        }
        if (!range->range_exclude_end) end++;
        ssize_t max = self->size();
        end = end > max ? max : end;
        ArrayValue *result = new ArrayValue { env };
        for (int64_t i = begin; i < end; i++) {
            result->push((*self)[i]);
        }
        return result;
    } else {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into Integer", NAT_OBJ_CLASS(index_obj)->class_name);
    }
}

Value *Array_refeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, Value::Type::Integer, "Integer"); // TODO: accept a range
    int64_t index = index_obj->as_integer()->to_int64_t();
    assert(index >= 0); // TODO: accept negative index
    Value *val;
    if (argc == 2) {
        val = args[1];
        if (index < self->size()) {
            (*self)[index] = val;
        } else {
            self->expand_with_nil(env, index);
            self->push(val);
        }
        return val;
    }
    Value *len_obj = args[1];
    NAT_ASSERT_TYPE(len_obj, Value::Type::Integer, "Integer");
    int64_t length = len_obj->as_integer()->to_int64_t();
    assert(length >= 0);
    val = args[2];
    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayValue *ary2 = new ArrayValue { env };
    // stuff before the new entry/entries
    for (ssize_t i = 0; i < index; i++) {
        if (i >= self->size()) break;
        ary2->push((*self)[i]);
    }
    // extra nils if needed
    ary2->expand_with_nil(env, index);
    // the new entry/entries
    if (val->is_array()) {
        for (auto &v : *val->as_array()) {
            ary2->push(v);
        }
    } else {
        ary2->push(val);
    }
    // stuff after the new entry/entries
    for (ssize_t i = index + length; i < self->size(); i++) {
        ary2->push((*self)[i]);
    }
    self->overwrite(*ary2);
    return val;
}

Value *Array_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, self->size() };
}

Value *Array_any(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    if (block) {
        for (auto &obj : *self) {
            Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
            if (result->is_truthy()) return NAT_TRUE;
        }
    } else if (self->size() > 0) {
        return NAT_TRUE;
    }
    return NAT_FALSE;
}

Value *Array_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    if (!args[0]->is_array()) return NAT_FALSE;
    ArrayValue *arg = args[0]->as_array();
    if (self->size() != arg->size()) return NAT_FALSE;
    if (self->size() == 0) return NAT_TRUE;
    for (ssize_t i = 0; i < self->size(); i++) {
        // TODO: could easily be optimized for strings and numbers
        Value *item = (*arg)[i];
        Value *result = send(env, (*self)[i], "==", 1, &item, NULL);
        if (NAT_TYPE(result) == Value::Type::False) return result;
    }
    return NAT_TRUE;
}

Value *Array_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (auto &obj : *self) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
    }
    return self;
}

Value *Array_each_with_index(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (ssize_t i = 0; i < self->size(); i++) {
        Value *args[2] = { (*self)[i], new IntegerValue { env, i } };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, args, NULL);
    }
    return self;
}

Value *Array_map(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    ArrayValue *new_array = new ArrayValue { env };
    for (auto &item : *self) {
        Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        new_array->push(result);
    }
    return new_array;
}

Value *Array_first(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    if (self->size() > 0) {
        return (*self)[0];
    } else {
        return NAT_NIL;
    }
}

Value *Array_last(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    if (self->size() > 0) {
        return (*self)[self->size() - 1];
    } else {
        return NAT_NIL;
    }
}

Value *Array_to_ary(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self_value;
}

Value *Array_pop(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_NOT_FROZEN(self);
    return self->pop(env);
}

Value *Array_include(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    Value *item = args[0];
    if (self->size() == 0) {
        return NAT_FALSE;
    } else {
        for (auto &compare_item : *self) {
            if (send(env, item, "==", 1, &compare_item, nullptr)->is_truthy()) {
                return NAT_TRUE;
            }
        }
        return NAT_FALSE;
    }
}

Value *Array_sort(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    ArrayValue *copy = ArrayValue::copy(env, *self);
    copy->sort(env);
    return copy;
}

Value *Array_join(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    if (self->size() == 0) {
        return new StringValue { env };
    } else if (self->size() == 1) {
        return send(env, (*self)[0], "to_s", 0, NULL, NULL);
    } else {
        Value *joiner = args[0];
        NAT_ASSERT_TYPE(joiner, Value::Type::String, "String");
        StringValue *out = send(env, (*self)[0], "to_s", 0, NULL, NULL)->as_string();
        for (auto i = 1; i < self->size(); i++) {
            Value *item = (*self)[i];
            out->append_string(env, joiner->as_string());
            out->append_string(env, send(env, item, "to_s", 0, NULL, NULL)->as_string());
        }
        return out;
    }
}

Value *Array_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *other = args[0]->as_array();
    for (ssize_t i = 0; i < self->size(); i++) {
        if (i >= other->size()) {
            return new IntegerValue { env, 1 };
        }
        Value *item = (*other)[i];
        Value *cmp_obj = send(env, (*self)[i], "<=>", 1, &item, NULL);
        assert(NAT_TYPE(cmp_obj) == Value::Type::Integer);
        int64_t cmp = cmp_obj->as_integer()->to_int64_t();
        if (cmp < 0) return new IntegerValue { env, -1 };
        if (cmp > 0) return new IntegerValue { env, 1 };
    }
    if (other->size() > self->size()) {
        return new IntegerValue { env, -1 };
    }
    return new IntegerValue { env, 0 };
}

Value *Array_to_a(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    return self_value;
}

}
