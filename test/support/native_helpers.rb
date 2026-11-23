require 'natalie/inline'

__inline__ <<~CPP
  void native_record_method(Env *env, Value receiver, Value value) {
      receiver->ivar_set(env, "@native_value"_s, value);
      receiver->ivar_set(env, "@native_method"_s, StringObject::create(env->current_method()->name()));
  }
CPP
__function__ 'native_record_method', ['Env *', 'Value', 'Value'], 'void'

class NativeHelpers
  __define_method__ :zero, [], 'return Value::integer(7);'
  __define_method__ :sum, %i[left right], 'return left.send(env, "+"_s, { right });'

  attr_reader :native_value, :native_method

  def record(value)
    __call__('native_record_method', self, value)
  end
end

object = NativeHelpers.new
p object.record(42)
p [object.native_value, object.native_method]
p [object.method(:zero).arity, object.method(:sum).arity]
p [object.method(:zero).parameters, object.method(:sum).parameters]
p [object.zero, object.sum(3, 4)]
[-> { object.zero(1) }, -> { object.sum(1) }].each do |call|
  begin
    call.call
  rescue ArgumentError
    puts 'wrong arity'
  end
end
