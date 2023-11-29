# Ruby 3.1 version, where we can use the shorthand

def method_with_kwargs1(a, b:)
  [a, b]
end

b = 2
p method_with_kwargs1(1, b:)
