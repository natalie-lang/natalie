# Run after `rake` and `rake llvm_runtime`:
# bin/natalie --backend=llvm examples/llvm_poc.rb

numbers = []
i = 1
while i <= 10
  numbers << i * i
  i = i + 1
end

puts 'Squares from LLVM:'
p numbers
puts numbers.sum
puts(
  if numbers.size == 10
    'ok'
  else
    'failed'
  end,
)

# Keep generated-code values alive across a runtime collection.
GC.start
puts numbers.join(', ')
