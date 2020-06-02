#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Array_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 2);
    Value *ary = array_new(env);
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
    for (int64_t i = 0; i < NAT_INT_VALUE(size); i++) {
        array_push(env, ary, value);
    }
    return ary;
}

// Array[]
Value *Array_square_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    Value *ary = array_new(env);
    for (ssize_t i = 0; i < argc; i++) {
        array_push(env, ary, args[i]);
    }
    return ary;
}

Value *Array_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ArrayValue *self = self_value->as_array();
    StringValue *out = string(env, "[");
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        Value *obj = static_cast<Value *>(vector_get(&self->ary, i));
        StringValue *repr = send(env, obj, "inspect", 0, NULL, NULL)->as_string();
        string_append_string(env, out, repr);
        if (i < vector_size(&self->ary) - 1) {
            string_append(env, out, ", ");
        }
    }
    string_append_char(env, out, ']');
    return out;
}

Value *Array_ltlt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_NOT_FROZEN(self);
    Value *arg = args[0];
    array_push(env, self, arg);
    return self;
}

Value *Array_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *arg = args[0]->as_array();
    ArrayValue *new_array = array_new(env);
    vector_add(&new_array->ary, &self->ary, &arg->ary);
    return new_array;
}

Value *Array_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *arg = args[0]->as_array();
    Value *new_array = array_new(env);
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        Value *item = static_cast<Value *>(vector_get(&self->ary, i));
        int found = 0;
        for (ssize_t j = 0; j < vector_size(&arg->ary); j++) {
            Value *compare_item = static_cast<Value *>(vector_get(&arg->ary, j));
            if (truthy(send(env, item, "==", 1, &compare_item, NULL))) {
                found = 1;
                break;
            }
        }
        if (!found) {
            array_push(env, new_array, item);
        }
    }
    return new_array;
}

Value *Array_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    ArrayValue *self = self_value->as_array();
    Value *index_obj = args[0];
    if (NAT_TYPE(index_obj) == Value::Type::Integer) {
        ssize_t index = NAT_INT_VALUE(index_obj);
        if (index < 0) {
            index = vector_size(&self->ary) + index;
        }
        if (index < 0 || index >= vector_size(&self->ary)) {
            return NAT_NIL;
        } else if (argc == 1) {
            return static_cast<Value *>(vector_get(&self->ary, index));
        }
        Value *size = args[1];
        NAT_ASSERT_TYPE(size, Value::Type::Integer, "Integer");
        ssize_t end = index + NAT_INT_VALUE(size);
        ssize_t max = vector_size(&self->ary);
        end = end > max ? max : end;
        Value *result = array_new(env);
        for (ssize_t i = index; i < end; i++) {
            array_push(env, result, static_cast<Value *>(vector_get(&self->ary, i)));
        }
        return result;
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();
        Value *begin_obj = range->range_begin;
        Value *end_obj = range->range_end;
        NAT_ASSERT_TYPE(begin_obj, Value::Type::Integer, "Integer");
        NAT_ASSERT_TYPE(end_obj, Value::Type::Integer, "Integer");
        int64_t begin = NAT_INT_VALUE(begin_obj);
        int64_t end = NAT_INT_VALUE(end_obj);
        if (begin < 0) {
            begin = vector_size(&self->ary) + begin;
        }
        if (end < 0) {
            end = vector_size(&self->ary) + end;
        }
        if (begin < 0 || end < 0) {
            return NAT_NIL;
        }
        if (!range->range_exclude_end) end++;
        ssize_t max = vector_size(&self->ary);
        end = end > max ? max : end;
        Value *result = array_new(env);
        for (int64_t i = begin; i < end; i++) {
            array_push(env, result, static_cast<Value *>(vector_get(&self->ary, i)));
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
    assert(NAT_INT_VALUE(index_obj) >= 0); // TODO: accept negative index
    ssize_t index = NAT_INT_VALUE(index_obj);
    Value *val;
    if (argc == 2) {
        val = args[1];
        if (index < vector_size(&self->ary)) {
            vector_set(&self->ary, index, val);
        } else {
            array_expand_with_nil(env, self, index);
            array_push(env, self, val);
        }
        return val;
    }
    Value *len_obj = args[1];
    NAT_ASSERT_TYPE(len_obj, Value::Type::Integer, "Integer");
    int length = NAT_INT_VALUE(len_obj);
    assert(length >= 0);
    val = args[2];
    // PERF: inefficient for large arrays where changes are being made to only the right side
    ArrayValue *ary2 = array_new(env);
    // stuff before the new entry/entries
    for (ssize_t i = 0; i < index; i++) {
        if (i >= vector_size(&self->ary)) break;
        array_push(env, ary2, static_cast<Value *>(vector_get(&self->ary, i)));
    }
    // extra nils if needed
    array_expand_with_nil(env, ary2, index);
    // the new entry/entries
    if (val->is_array()) {
        for (ssize_t i = 0; i < vector_size(&val->as_array()->ary); i++) {
            array_push(env, ary2, static_cast<Value *>(vector_get(&val->as_array()->ary, i)));
        }
    } else {
        array_push(env, ary2, val);
    }
    // stuff after the new entry/entries
    for (ssize_t i = index + length; i < vector_size(&self->ary); i++) {
        array_push(env, ary2, static_cast<Value *>(vector_get(&self->ary, i)));
    }
    vector_copy(&self->ary, &ary2->ary);
    return val;
}

Value *Array_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    return integer(env, vector_size(&self->ary));
}

Value *Array_any(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    if (block) {
        for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
            Value *obj = static_cast<Value *>(vector_get(&self->ary, i));
            Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
            if (truthy(result)) return NAT_TRUE;
        }
    } else if (vector_size(&self->ary) > 0) {
        return NAT_TRUE;
    }
    return NAT_FALSE;
}

Value *Array_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    if (!args[0]->is_array()) return NAT_FALSE;
    ArrayValue *arg = args[0]->as_array();
    if (vector_size(&self->ary) != vector_size(&arg->ary)) return NAT_FALSE;
    if (vector_size(&self->ary) == 0) return NAT_TRUE;
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        // TODO: could easily be optimized for strings and numbers
        Value *item = static_cast<Value *>(vector_get(&arg->ary, i));
        Value *result = send(env, static_cast<Value *>(vector_get(&self->ary, i)), "==", 1, &item, NULL);
        if (NAT_TYPE(result) == Value::Type::False) return result;
    }
    return NAT_TRUE;
}

Value *Array_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        Value *obj = static_cast<Value *>(vector_get(&self->ary, i));
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
    }
    return self;
}

Value *Array_each_with_index(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        Value *args[2] = { static_cast<Value *>(vector_get(&self->ary, i)), integer(env, i) };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, args, NULL);
    }
    return self;
}

Value *Array_map(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    Value *new_array = array_new(env);
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        Value *item = static_cast<Value *>(vector_get(&self->ary, i));
        Value *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        array_push(env, new_array, result);
    }
    return new_array;
}

Value *Array_first(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    if (vector_size(&self->ary) > 0) {
        return static_cast<Value *>(vector_get(&self->ary, 0));
    } else {
        return NAT_NIL;
    }
}

Value *Array_last(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    if (vector_size(&self->ary) > 0) {
        return static_cast<Value *>(vector_get(&self->ary, vector_size(&self->ary) - 1));
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
    if (vector_size(&self->ary) == 0) {
        return NAT_NIL;
    } else {
        Value *val = static_cast<Value *>(vector_get(&self->ary, vector_size(&self->ary) - 1));
        self->ary.size--;
        return val;
    }
}

Value *Array_include(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    Value *item = args[0];
    if (vector_size(&self->ary) == 0) {
        return NAT_FALSE;
    } else {
        for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
            Value *compare_item = static_cast<Value *>(vector_get(&self->ary, i));
            if (truthy(send(env, item, "==", 1, &compare_item, NULL))) {
                return NAT_TRUE;
            }
        }
        return NAT_FALSE;
    }
}

Value *Array_sort(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(0);
    ArrayValue *copy = array_copy(env, self);
    quicksort(env, (Value **)copy->ary.data, 0, vector_size(&copy->ary) - 1);
    return copy;
}

Value *Array_join(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    if (vector_size(&self->ary) == 0) {
        return string(env, "");
    } else if (vector_size(&self->ary) == 1) {
        return send(env, static_cast<Value *>(vector_get(&self->ary, 0)), "to_s", 0, NULL, NULL);
    } else {
        Value *joiner = args[0];
        NAT_ASSERT_TYPE(joiner, Value::Type::String, "String");
        StringValue *out = send(env, static_cast<Value *>(vector_get(&self->ary, 0)), "to_s", 0, NULL, NULL)->as_string();
        for (ssize_t i = 1; i < vector_size(&self->ary); i++) {
            string_append_string(env, out, joiner->as_string());
            string_append_string(env, out, send(env, static_cast<Value *>(vector_get(&self->ary, i)), "to_s", 0, NULL, NULL)->as_string());
        }
        return out;
    }
}

Value *Array_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *self = self_value->as_array();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Array, "Array");
    ArrayValue *other = args[0]->as_array();
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        if (i >= vector_size(&other->ary)) {
            return integer(env, 1);
        }
        Value *item = static_cast<Value *>(vector_get(&other->ary, i));
        Value *cmp_obj = send(env, static_cast<Value *>(vector_get(&self->ary, i)), "<=>", 1, &item, NULL);
        assert(NAT_TYPE(cmp_obj) == Value::Type::Integer);
        int64_t cmp = NAT_INT_VALUE(cmp_obj);
        if (cmp < 0) return integer(env, -1);
        if (cmp > 0) return integer(env, 1);
    }
    if (vector_size(&other->ary) > vector_size(&self->ary)) {
        return integer(env, -1);
    }
    return integer(env, 0);
}

Value *Array_to_a(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    return self_value;
}

}
