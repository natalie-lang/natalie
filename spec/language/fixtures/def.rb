def toplevel_define_other_method
  def nested_method_in_toplevel_method
    42
  end
end

def some_toplevel_method
end

NATFIXME 'Implement main#public', exception: NoMethodError, message: "undefined method `public' for main" do
  public
end
def public_toplevel_method
end

NATFIXME 'Implement main#private', exception: NoMethodError, message: "undefined method `private' for main" do
  private
end
