# Native Code

Ruby owns local variables and control flow. Scoped `__inline__` is retired on
both backends: do not embed C++ statements in Ruby methods, blocks, or class
bodies, or refer to generated names such as `value_var` from native code.
Top-level `__inline__` declarations and includes remain supported.

For an operation inside a Ruby method, put its native implementation in a C++
helper with explicit arguments and a return value. Require its `.cpp` file and
declare the signature with `__function__`, then call it with `__call__`:

```ruby
require 'natalie/inline'
require 'date.cpp'
__function__ 'Date_strftime', ['Env *', 'Value', 'Value'], 'Value'

def strftime(format = '%F')
  __call__('Date_strftime', self, format)
end
```

An initial `Env *` argument is supplied implicitly using the current Ruby
environment. The helper creates no Ruby method frame. Preserve this behavior
for operations that depend on the caller, regexp match state, or pending thread
interrupts. Assign returned values explicitly in Ruby; a helper returning
`void` produces Ruby `nil`. Other supported return types are `Value` and
`double`; explicit argument types are `Value`, `double`, `int`, and `bool`.

Extension files follow the existing `Value init_basename(Env *env, Value self)`
convention. Initialization runs before the Ruby body, so use a helper call at
the appropriate Ruby execution point for work that depends on Ruby constants
or locals being initialized. Runtime helpers can instead live in `src/` with
declarations in `include/natalie/`.

For a method implemented entirely in C++, use `__define_method__` or a native
binding. An explicit argument list enforces and records its arity:

```ruby
__define_method__ :sum, [:left, :right], <<~CPP
  return left.send(env, "+"_s, { right });
CPP
```

Use `[]` for a zero-argument method. Omit the list only when the native body
handles `Args` and validates its own argument count. Every native method body
must return a `Value`, including `Value::nil()` when appropriate.
