require 'natalie/inline'
require 'etc.cpp'

module Etc
  Passwd = Struct.new(:name, :passwd, :uid, :gid, :gecos, :dir, :shell)
  Group = Struct.new(:name, :passwd, :gid, :mem)

  VERSION = '1.3.0'.freeze

  # POSIX.1 variables [confstr(3) man page]
  __constant__('CS_PATH', 'int', '_CS_PATH')

  # POSIX.1 variables [sysconf(3) man page]
  __constant__('SC_ARG_MAX', 'int', '_SC_ARG_MAX')
  __constant__('SC_CHILD_MAX', 'int', '_SC_CHILD_MAX')
  __constant__('SC_HOST_NAME_MAX', 'int', '_SC_HOST_NAME_MAX')
  __constant__('SC_LOGIN_NAME_MAX', 'int', '_SC_LOGIN_NAME_MAX')
  __constant__('SC_NGROUPS_MAX', 'int', '_SC_NGROUPS_MAX')
  __constant__('SC_CLK_TCK', 'int', '_SC_CLK_TCK')
  __constant__('SC_OPEN_MAX', 'int', '_SC_OPEN_MAX')
  __constant__('SC_PAGESIZE', 'int', '_SC_PAGESIZE')
  __constant__('SC_PAGE_SIZE', 'int', '_SC_PAGE_SIZE')
  __constant__('SC_RE_DUP_MAX', 'int', '_SC_RE_DUP_MAX')
  __constant__('SC_STREAM_MAX', 'int', '_SC_STREAM_MAX')
  __constant__('SC_SYMLOOP_MAX', 'int', '_SC_SYMLOOP_MAX')
  __constant__('SC_TTY_NAME_MAX', 'int', '_SC_TTY_NAME_MAX')
  __constant__('SC_TZNAME_MAX', 'int', '_SC_TZNAME_MAX')
  __constant__('SC_VERSION', 'int', '_SC_VERSION')
  
  __bind_static_method__ :confstr, :Etc_confstr
  __bind_static_method__ :endgrent, :Etc_endgrent
  __bind_static_method__ :endpwent, :Etc_endpwent
  __bind_static_method__ :getgrent, :Etc_getgrent
  __bind_static_method__ :getgrgid, :Etc_getgrgid
  __bind_static_method__ :getgrnam, :Etc_getgrnam
  __bind_static_method__ :getlogin, :Etc_getlogin
  __bind_static_method__ :getpwent, :Etc_getpwent
  __bind_static_method__ :getpwnam, :Etc_getpwnam
  __bind_static_method__ :getpwuid, :Etc_getpwuid
  #__bind_static_method__ :group, :Etc_group
  __bind_static_method__ :nprocessors, :Etc_nprocessors
  #__bind_static_method__ :passwd, :Etc_passwd
  __bind_static_method__ :setgrent, :Etc_setgrent
  __bind_static_method__ :setpwent, :Etc_setpwent
  __bind_static_method__ :sysconf, :Etc_sysconf
  #__bind_static_method__ :sysconfdir, :Etc_sysconfdir
  #__bind_static_method__ :systmpdir, :Etc_systmpdir
  #__bind_static_method__ :uname, :Etc_uname
end

