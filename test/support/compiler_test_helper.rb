require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'rbconfig'
require_relative 'nat_binary'
require_relative 'llvm/object_linker'
require_relative '../../lib/natalie/compiler/backends/llvm_backend'

module CompilerTestHelper
  include LlvmObjectLinker

  ROOT = File.expand_path('../..', __dir__)
end
