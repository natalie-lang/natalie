Header files end in `.hpp` and C++ files end in `.cpp` in this codebase.
Never alter files in the `build` directory. Those files are generated from other sources.
Never write a script to make changes to the codebase. Make all changes carefully and manually.
You can usually build the project using `rake build`, but if you make changes to the compiler, you may need to run `rake clean build`.
You can do a quick smoke-test by running `bin/natalie examples/hello.rb` or `bin/natalie examples/boardslam.rb 5 3 1`. Those are good tests.
Of course, running an actual test like `bin/natalie spec/core/float/to_s_spec.rb` (or similar) is probably better.
You should run `rake clean test` before committing to make sure the entire test suite passes.
And lastly, please run `rake format` before committing to make sure all code is formatted correctly.
