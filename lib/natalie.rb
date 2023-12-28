require_relative 'natalie/compiler'
require_relative 'natalie/parser'
require_relative 'natalie/vm'

if RUBY_ENGINE == 'natalie'
  require_relative 'natalie/api'
end
