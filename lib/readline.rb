require 'natalie/inline'

__inline__ <<-END
#include <iostream>
END

module Readline
  class << self
    __define_method__ :readline, [:prompt], <<-END
      prompt.assert_type(env, Object::Type::String, "String");
      std::cout << prompt.as_string()->c_str();

      std::string line;
      std::getline(std::cin, line);
      return new StringObject { line.c_str() };
    END
  end
end
