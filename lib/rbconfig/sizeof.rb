require 'natalie/inline'
__inline__ '#include <limits>'

module RbConfig
  SIZEOF = {}
  LIMITS = {}

  __inline__ <<~CPP
    auto SIZEOF = self->const_get("SIZEOF"_s)->as_hash();
    SIZEOF->put(env, new StringObject { "double" }, Value::integer(sizeof(double)));
    SIZEOF->put(env, new StringObject { "float" }, Value::integer(sizeof(float)));
    SIZEOF->put(env, new StringObject { "int" }, Value::integer(sizeof(int)));
    SIZEOF->put(env, new StringObject { "long" }, Value::integer(sizeof(long)));
    SIZEOF->put(env, new StringObject { "short" }, Value::integer(sizeof(short)));
    SIZEOF->put(env, new StringObject { "void*" }, Value::integer(sizeof(void *)));

    auto LIMITS = self->const_get("LIMITS"_s)->as_hash();
    LIMITS->put(env, new StringObject { "CHAR_MIN" }, Value::integer(std::numeric_limits<char>::min()));
    LIMITS->put(env, new StringObject { "CHAR_MAX" }, Value::integer(std::numeric_limits<char>::max()));
    LIMITS->put(env, new StringObject { "SHRT_MIN" }, Value::integer(std::numeric_limits<short>::min()));
    LIMITS->put(env, new StringObject { "SHRT_MAX" }, Value::integer(std::numeric_limits<short>::max()));
    LIMITS->put(env, new StringObject { "INT_MIN" }, Value::integer(std::numeric_limits<int>::min()));
    LIMITS->put(env, new StringObject { "INT_MAX" }, Value::integer(std::numeric_limits<int>::max()));
    LIMITS->put(env, new StringObject { "LONG_MIN" }, Value::integer(std::numeric_limits<long>::min()));
    LIMITS->put(env, new StringObject { "LONG_MAX" }, Value::integer(std::numeric_limits<long>::max()));
    LIMITS->put(env, new StringObject { "FIXNUM_MIN" }, Value::integer(std::numeric_limits<long>::min() >> 1));
    LIMITS->put(env, new StringObject { "FIXNUM_MAX" }, Value::integer(std::numeric_limits<long>::max() >> 1));
  CPP
end
