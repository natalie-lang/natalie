#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Array_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 2);
    NatObject *ary = array_new(env);
    if (argc == 0) {
        return ary;
    }
    NatObject *size = args[0];
    if (NAT_TYPE(size) == NAT_VALUE_ARRAY) {
        // given an array, just return it
        return size;
    }
    NAT_ASSERT_TYPE(size, NAT_VALUE_INTEGER, "Integer");
    NatObject *value = argc == 2 ? args[1] : NAT_NIL;
    for (int64_t i = 0; i < NAT_INT_VALUE(size); i++) {
        array_push(env, ary, value);
    }
    return ary;
}

// Array[]
NatObject *Array_square_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *ary = array_new(env);
    for (ssize_t i = 0; i < argc; i++) {
        array_push(env, ary, args[i]);
    }
    return ary;
}

NatObject *Array_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *out = string(env, "[");
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        NatObject *obj = static_cast<NatObject *>(vector_get(&self->ary, i));
        NatObject *repr = send(env, obj, "inspect", 0, NULL, NULL);
        string_append_string(env, out, repr);
        if (i < vector_size(&self->ary) - 1) {
            string_append(env, out, ", ");
        }
    }
    string_append_char(env, out, ']');
    return out;
}

NatObject *Array_ltlt(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *arg = args[0];
    array_push(env, self, arg);
    return self;
}

NatObject *Array_add(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_ARRAY, "Array");
    NatObject *new_array = array_new(env);
    vector_add(&new_array->ary, &self->ary, &arg->ary);
    return new_array;
}

NatObject *Array_sub(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_ARRAY, "Array");
    NatObject *new_array = array_new(env);
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        NatObject *item = static_cast<NatObject *>(vector_get(&self->ary, i));
        int found = 0;
        for (ssize_t j = 0; j < vector_size(&arg->ary); j++) {
            NatObject *compare_item = static_cast<NatObject *>(vector_get(&arg->ary, j));
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

NatObject *Array_ref(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *index_obj = args[0];
    if (NAT_TYPE(index_obj) == NAT_VALUE_INTEGER) {
        ssize_t index = NAT_INT_VALUE(index_obj);
        if (index < 0) {
            index = vector_size(&self->ary) + index;
        }
        if (index < 0 || index >= vector_size(&self->ary)) {
            return NAT_NIL;
        } else if (argc == 1) {
            return static_cast<NatObject *>(vector_get(&self->ary, index));
        }
        NatObject *size = args[1];
        NAT_ASSERT_TYPE(size, NAT_VALUE_INTEGER, "Integer");
        ssize_t end = index + NAT_INT_VALUE(size);
        ssize_t max = vector_size(&self->ary);
        end = end > max ? max : end;
        NatObject *result = array_new(env);
        for (ssize_t i = index; i < end; i++) {
            array_push(env, result, static_cast<NatObject *>(vector_get(&self->ary, i)));
        }
        return result;
    } else if (NAT_TYPE(index_obj) == NAT_VALUE_RANGE) {
        NatObject *begin_obj = index_obj->range_begin;
        NatObject *end_obj = index_obj->range_end;
        NAT_ASSERT_TYPE(begin_obj, NAT_VALUE_INTEGER, "Integer");
        NAT_ASSERT_TYPE(end_obj, NAT_VALUE_INTEGER, "Integer");
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
        if (!index_obj->range_exclude_end) end++;
        ssize_t max = vector_size(&self->ary);
        end = end > max ? max : end;
        NatObject *result = array_new(env);
        for (int64_t i = begin; i < end; i++) {
            array_push(env, result, static_cast<NatObject *>(vector_get(&self->ary, i)));
        }
        return result;
    } else {
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into Integer", NAT_OBJ_CLASS(index_obj)->class_name);
    }
}

NatObject *Array_refeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    NAT_ASSERT_NOT_FROZEN(self);
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NatObject *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, NAT_VALUE_INTEGER, "Integer"); // TODO: accept a range
    assert(NAT_INT_VALUE(index_obj) >= 0); // TODO: accept negative index
    ssize_t index = NAT_INT_VALUE(index_obj);
    NatObject *val;
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
    NatObject *len_obj = args[1];
    NAT_ASSERT_TYPE(len_obj, NAT_VALUE_INTEGER, "Integer");
    int length = NAT_INT_VALUE(len_obj);
    assert(length >= 0);
    val = args[2];
    // PERF: inefficient for large arrays where changes are being made to only the right side
    NatObject *ary2 = array_new(env);
    // stuff before the new entry/entries
    for (ssize_t i = 0; i < index; i++) {
        if (i >= vector_size(&self->ary)) break;
        array_push(env, ary2, static_cast<NatObject *>(vector_get(&self->ary, i)));
    }
    // extra nils if needed
    array_expand_with_nil(env, ary2, index);
    // the new entry/entries
    if (NAT_TYPE(val) == NAT_VALUE_ARRAY) {
        for (ssize_t i = 0; i < vector_size(&val->ary); i++) {
            array_push(env, ary2, static_cast<NatObject *>(vector_get(&val->ary, i)));
        }
    } else {
        array_push(env, ary2, val);
    }
    // stuff after the new entry/entries
    for (ssize_t i = index + length; i < vector_size(&self->ary); i++) {
        array_push(env, ary2, static_cast<NatObject *>(vector_get(&self->ary, i)));
    }
    vector_copy(&self->ary, &ary2->ary);
    return val;
}

NatObject *Array_size(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    return integer(env, vector_size(&self->ary));
}

NatObject *Array_any(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    if (block) {
        for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
            NatObject *obj = static_cast<NatObject *>(vector_get(&self->ary, i));
            NatObject *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
            if (truthy(result)) return NAT_TRUE;
        }
    } else if (vector_size(&self->ary) > 0) {
        return NAT_TRUE;
    }
    return NAT_FALSE;
}

NatObject *Array_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_ARRAY) return NAT_FALSE;
    if (vector_size(&self->ary) != vector_size(&arg->ary)) return NAT_FALSE;
    if (vector_size(&self->ary) == 0) return NAT_TRUE;
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        // TODO: could easily be optimized for strings and numbers
        NatObject *item = static_cast<NatObject *>(vector_get(&arg->ary, i));
        NatObject *result = send(env, static_cast<NatObject *>(vector_get(&self->ary, i)), "==", 1, &item, NULL);
        if (NAT_TYPE(result) == NAT_VALUE_FALSE) return result;
    }
    return NAT_TRUE;
}

NatObject *Array_each(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        NatObject *obj = static_cast<NatObject *>(vector_get(&self->ary, i));
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &obj, NULL);
    }
    return self;
}

NatObject *Array_each_with_index(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        NatObject *args[2] = { static_cast<NatObject *>(vector_get(&self->ary, i)), integer(env, i) };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 2, args, NULL);
    }
    return self;
}

NatObject *Array_map(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    NatObject *new_array = array_new(env);
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        NatObject *item = static_cast<NatObject *>(vector_get(&self->ary, i));
        NatObject *result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        array_push(env, new_array, result);
    }
    return new_array;
}

NatObject *Array_first(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    if (vector_size(&self->ary) > 0) {
        return static_cast<NatObject *>(vector_get(&self->ary, 0));
    } else {
        return NAT_NIL;
    }
}

NatObject *Array_last(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0); // TODO: accept integer and return array
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    if (vector_size(&self->ary) > 0) {
        return static_cast<NatObject *>(vector_get(&self->ary, vector_size(&self->ary) - 1));
    } else {
        return NAT_NIL;
    }
}

NatObject *Array_to_ary(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self;
}

NatObject *Array_pop(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    NAT_ASSERT_NOT_FROZEN(self);
    if (vector_size(&self->ary) == 0) {
        return NAT_NIL;
    } else {
        NatObject *val = static_cast<NatObject *>(vector_get(&self->ary, vector_size(&self->ary) - 1));
        self->ary.size--;
        return val;
    }
}

NatObject *Array_include(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *item = args[0];
    if (vector_size(&self->ary) == 0) {
        return NAT_FALSE;
    } else {
        for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
            NatObject *compare_item = static_cast<NatObject *>(vector_get(&self->ary, i));
            if (truthy(send(env, item, "==", 1, &compare_item, NULL))) {
                return NAT_TRUE;
            }
        }
        return NAT_FALSE;
    }
}

NatObject *Array_sort(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(0);
    NatObject *copy = array_copy(env, self);
    quicksort(env, (NatObject **)copy->ary.data, 0, vector_size(&copy->ary) - 1);
    return copy;
}

NatObject *Array_join(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    if (vector_size(&self->ary) == 0) {
        return string(env, "");
    } else if (vector_size(&self->ary) == 1) {
        return send(env, static_cast<NatObject *>(vector_get(&self->ary, 0)), "to_s", 0, NULL, NULL);
    } else {
        NatObject *joiner = args[0];
        NAT_ASSERT_TYPE(joiner, NAT_VALUE_STRING, "String");
        NatObject *out = send(env, static_cast<NatObject *>(vector_get(&self->ary, 0)), "to_s", 0, NULL, NULL);
        for (ssize_t i = 1; i < vector_size(&self->ary); i++) {
            string_append_string(env, out, joiner);
            string_append_string(env, out, send(env, static_cast<NatObject *>(vector_get(&self->ary, i)), "to_s", 0, NULL, NULL));
        }
        return out;
    }
}

NatObject *Array_cmp(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_ARRAY);
    NAT_ASSERT_ARGC(1);
    NatObject *other = args[0];
    NAT_ASSERT_TYPE(other, NAT_VALUE_ARRAY, "Array");
    for (ssize_t i = 0; i < vector_size(&self->ary); i++) {
        if (i >= vector_size(&other->ary)) {
            return integer(env, 1);
        }
        NatObject *item = static_cast<NatObject *>(vector_get(&other->ary, i));
        NatObject *cmp_obj = send(env, static_cast<NatObject *>(vector_get(&self->ary, i)), "<=>", 1, &item, NULL);
        assert(NAT_TYPE(cmp_obj) == NAT_VALUE_INTEGER);
        int64_t cmp = NAT_INT_VALUE(cmp_obj);
        if (cmp < 0) return integer(env, -1);
        if (cmp > 0) return integer(env, 1);
    }
    if (vector_size(&other->ary) > vector_size(&self->ary)) {
        return integer(env, -1);
    }
    return integer(env, 0);
}

NatObject *Array_to_a(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    return self;
}

}
