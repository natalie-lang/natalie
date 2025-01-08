module RbConfig
  CONFIG = {
    "bindir" => File.expand_path('../bin', __dir__),
    "ruby_install_name" => 'natalie',
    "RUBY_INSTALL_NAME" => 'natalie',
    "EXEEXT" => (RUBY_PLATFORM.include?('msys') ? '.exe' : ''),
    "DLEXT" => (RUBY_PLATFORM.include?('darwin') ? 'bundle' : 'so'),
    "SOEXT" => (RUBY_PLATFORM.include?('darwin') ? 'dylib' : 'so'),
    "LIBPATHENV" => (RUBY_PLATFORM.include?('darwin') ? 'DYLD_LIBRARY_PATH' : 'LD_LIBRARY_PATH'),
    "host_cpu" => RUBY_PLATFORM.split('-')[0],
    "host_os" => RUBY_PLATFORM.split('-')[1],
    "AR" => "ar",
    "STRIP" => "strip",
  }.freeze
  TOPDIR = nil
end
