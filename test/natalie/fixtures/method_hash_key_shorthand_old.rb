# Ruby 3.0 version, where we can't use the shorthand

def method_with_kwargs1(a, b:)
  [a, b]
end

b = 2
p method_with_kwargs1(1, b: b)
