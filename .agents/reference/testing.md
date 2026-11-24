# Testing

Prefer imported ruby/spec coverage that matches MRI behavior. Use `NATFIXME` for known unsupported cases instead of commenting out tests. Name specs `*_spec.rb` and custom tests `*_test.rb`. Host integration tests use Minitest.

Keep files under `spec/` aligned with upstream ruby/spec. Put Natalie-specific
regression cases and other custom tests under `test/`; do not add them to the
imported spec files. Existing `NATFIXME` markers may be removed when their
upstream examples pass.

Use MRI as the semantic reference. Fix runtime or compiler bugs exposed by
comparisons rather than matching incorrect behavior. Remove resolved
`NATFIXME` markers and keep regression coverage.

Run focused tests with `bin/natalie spec/core/string/strip_spec.rb` or `bundle exec ruby test/ruby/argv_test.rb`. Add relevant regression coverage; no numeric coverage threshold is documented.

Run the expected-passing suite with `bundle exec rake test` (builds Natalie first).
For LLVM tests and the current spec-runner boundary, see [LLVM backend](llvm.md).
