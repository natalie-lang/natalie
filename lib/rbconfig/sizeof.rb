require 'natalie/inline'

module RbConfig
  SIZEOF = {}

  __inline__ <<~CPP
    auto SIZEOF = self->const_get("SIZEOF"_s)->as_hash();
    SIZEOF->put(env, new StringObject { "double" }, Value::integer(sizeof(double)));
    SIZEOF->put(env, new StringObject { "float" }, Value::integer(sizeof(float)));
    SIZEOF->put(env, new StringObject { "int" }, Value::integer(sizeof(int)));
    SIZEOF->put(env, new StringObject { "long" }, Value::integer(sizeof(long)));
    SIZEOF->put(env, new StringObject { "short" }, Value::integer(sizeof(short)));
    SIZEOF->put(env, new StringObject { "void*" }, Value::integer(sizeof(void *)));
  CPP
end
