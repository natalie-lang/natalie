require 'natalie/inline'
require 'tempfile.cpp'

class Tempfile
  class << self
    def create(basename)
      if block_given?
        begin
          tmpfile = new(basename)
          yield(tmpfile.instance_variable_get(:@tmpfile))
        ensure
          tmpfile.unlink
        end
      else
        new(basename).instance_variable_get(:@tmpfile)
      end
    end
  end

  __bind_method__ :initialize, :Tempfile_initialize

  def unlink
    File.unlink(@tmpfile.path)
  end
  alias delete unlink

  (File.public_instance_methods(false) + [:close, :open, :path, :print, :puts, :rewind, :write]).each do |method|
    define_method(method) do |*args, **kwargs, &block|
      @tmpfile.public_send(method, *args, **kwargs, &block)
    end
  end
end
