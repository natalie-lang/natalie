module RbConfig
  CONFIG = {
    "bindir" => File.expand_path('../bin', __dir__),
    "ruby_install_name" => 'natalie',
    "RUBY_INSTALL_NAME" => 'natalie',
    "EXEEXT" => (RUBY_PLATFORM =~ /msys/ ? '.exe' : ''),
    "host_cpu" => RUBY_PLATFORM.split('-')[0],
    "host_os" => RUBY_PLATFORM.split('-')[1],
    "AR" => "ar",
    "STRIP" => "strip",
  }.freeze
  TOPDIR = nil
end
