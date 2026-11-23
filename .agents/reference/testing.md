# Testing

Prefer imported ruby/spec coverage that matches MRI behavior. Use `NATFIXME` for known unsupported cases instead of commenting out tests. Name specs `*_spec.rb` and custom tests `*_test.rb`. Host integration tests use Minitest.

Run focused tests with `bin/natalie spec/core/string/strip_spec.rb` or `bundle exec ruby test/ruby/argv_test.rb`. Add relevant regression coverage; no numeric coverage threshold is documented.

Run the expected-passing suite with `bundle exec rake test` (builds Natalie first).
For LLVM tests and the current spec-runner boundary, see [LLVM backend](llvm.md).
