# Ruby 3.0 version, where we can't use the shorthand

foo = 1
bar = 2
h = { foo: foo, bar: bar }
p [h[:foo], h[:bar]]
