require 'natalie/inline'

module Math
  class << self
    def hypot(x, y)
      begin
        x = Float(x)
      rescue ArgumentError
        raise TypeError, "can't convert #{x.class.name} into Float"
      end
      begin
        y = Float(y)
      rescue ArgumentError
        raise TypeError, "can't convert #{y.class.name} into Float"
      end
      sqrt(x**2 + y**2)
    end

    __define_method__ :sqrt, [:x], <<-END
      if (x->is_nil())
        env->raise("TypeError", "can't convert nil into Float");
      if (!x->is_float()) {
        try {
          x = GlobalEnv::the()->Object()->as_kernel_module_for_method_binding()->send(env, "Float"_s, { x });
        } catch (ExceptionObject *error) {
          if (error->is_a(env, GlobalEnv::the()->Object()->const_fetch("ArgumentError"_s)))
            env->raise("TypeError", "can't convert {} into Float", x->klass()->inspect_str());
        }
      }
      x->assert_type(env, Object::Type::Float, "Float");
      return Value { sqrt(x->as_float()->to_double()) };
    END
  end

  private

  def hypot(x, y)
    Math.hypot(x, y)
  end

  def sqrt(x)
    Math.sqrt(x)
  end
end
