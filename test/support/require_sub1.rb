def foo1
  'foo1'
end

class Bar1
  require_relative './require_sub6'

  autoload :Baz1, 'require_sub7'

  # this file does not define Bar1::Baz2
  autoload :Baz2, 'require_sub8'

  def bar1
    'bar1'
  end
end
