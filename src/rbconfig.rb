module RbConfig
  darwin = RUBY_PLATFORM.include?('darwin')
  ruby_version_parts = RUBY_VERSION.split('.')

  CONFIG = {
    'bindir' => File.expand_path('../bin', __dir__),
    'ruby_install_name' => 'natalie',
    'RUBY_INSTALL_NAME' => 'natalie',
    'EXEEXT' => (RUBY_PLATFORM.include?('msys') ? '.exe' : ''),
    'DLEXT' => (darwin ? 'bundle' : 'so'),
    'SOEXT' => (darwin ? 'dylib' : 'so'),
    'LIBPATHENV' => (darwin ? 'DYLD_LIBRARY_PATH' : 'LD_LIBRARY_PATH'),
    'host_cpu' => RUBY_PLATFORM.split('-')[0],
    'host_os' => RUBY_PLATFORM.split('-')[1],
    'AR' => 'ar',
    'STRIP' => 'strip',
    'MAJOR' => ruby_version_parts[0],
    'MINOR' => ruby_version_parts[1],
    'TEENY' => ruby_version_parts[2],
    'PATCHLEVEL' => RUBY_PATCHLEVEL.to_s,
  }.freeze
  TOPDIR = nil
end
