require_relative 'natalie/compiler'
require_relative 'natalie/parser'
if RUBY_ENGINE == 'natalie'
    require_relative 'natalie/repl/legacy'
else
    require_relative 'nat_extra_macros'
    nat_ignore_require_relative 'natalie/repl/main'
end