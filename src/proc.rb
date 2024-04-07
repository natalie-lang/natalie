class Proc
  public :binding

  def >>(other)
    raise TypeError, 'callable object is expected' unless other.respond_to?(:call)

    if lambda?
      ->(*args, **kwargs, &block) { other.call(call(*args, **kwargs, &block)) }
    else
      proc { |*args, **kwargs, &block| other.call(call(*args, **kwargs, &block)) }
    end
  end
end
