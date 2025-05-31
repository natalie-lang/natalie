require 'natalie/inline'
__inline__ '#include <limits>'

module RbConfig
  SIZEOF = {}
  LIMITS = {}

  __inline__ <<~CPP
    auto SIZEOF = self.as_module()->const_fetch("SIZEOF"_s).as_hash();
    SIZEOF->put(env, StringObject::create("double"), Value::integer(sizeof(double)));
    SIZEOF->put(env, StringObject::create("float"), Value::integer(sizeof(float)));
    SIZEOF->put(env, StringObject::create("int"), Value::integer(sizeof(int)));
    SIZEOF->put(env, StringObject::create("long"), Value::integer(sizeof(long)));
    SIZEOF->put(env, StringObject::create("short"), Value::integer(sizeof(short)));
    SIZEOF->put(env, StringObject::create("void*"), Value::integer(sizeof(void *)));

    auto LIMITS = self.as_module()->const_fetch("LIMITS"_s).as_hash();
    LIMITS->put(env, StringObject::create("CHAR_MIN"), Value::integer(std::numeric_limits<char>::min()));
    LIMITS->put(env, StringObject::create("CHAR_MAX"), Value::integer(std::numeric_limits<char>::max()));
    LIMITS->put(env, StringObject::create("SHRT_MIN"), Value::integer(std::numeric_limits<short>::min()));
    LIMITS->put(env, StringObject::create("SHRT_MAX"), Value::integer(std::numeric_limits<short>::max()));
    LIMITS->put(env, StringObject::create("INT_MIN"), Value::integer(std::numeric_limits<int>::min()));
    LIMITS->put(env, StringObject::create("INT_MAX"), Value::integer(std::numeric_limits<int>::max()));
    LIMITS->put(env, StringObject::create("LONG_MIN"), Value::integer(std::numeric_limits<long>::min()));
    LIMITS->put(env, StringObject::create("LONG_MAX"), Value::integer(std::numeric_limits<long>::max()));
    LIMITS->put(env, StringObject::create("FIXNUM_MIN"), Value::integer(std::numeric_limits<long>::min() >> 1));
    LIMITS->put(env, StringObject::create("FIXNUM_MAX"), Value::integer(std::numeric_limits<long>::max() >> 1));
  CPP
end
