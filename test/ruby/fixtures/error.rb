def method_with_error
  puts something_non_existent
end

def method2
  [1, 2].each { |i| method_with_error }
end

def method1
  method2
end

method1
