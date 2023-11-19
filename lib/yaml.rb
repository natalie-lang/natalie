require 'natalie/inline'
require 'yaml.cpp'

__ld_flags__ '-lyaml'

module YAML
  __bind_static_method__ :dump, :YAML_dump
  __bind_static_method__ :load, :YAML_load

  def self.load_file(file)
    File.open(file) { |fh| self.load(fh) }
  end
end

# Compatibility version, required to run the specs
module Psych
  VERSION = '4.0.0'.freeze
end

class Object
  def to_yaml
    YAML.dump(self)
  end
end
