require 'natalie/inline'
require 'etc.cpp'

module Etc
  Passwd = Struct.new(:name, :passwd, :uid, :gid, :gecos, :dir, :shell)
  
  #__bind_static_method__ :confstr, :Etc_confstr
  #__bind_static_method__ :endgrent, :Etc_endgrent
  #__bind_static_method__ :endpwent, :Etc_endpwent
  #__bind_static_method__ :getgrent, :Etc_getgrent
  #__bind_static_method__ :getgrgid, :Etc_getgrgid
  #__bind_static_method__ :getgrnam, :Etc_getgrnam
  __bind_static_method__ :getlogin, :Etc_getlogin
  __bind_static_method__ :getpwent, :Etc_getpwent
  __bind_static_method__ :getpwnam, :Etc_getpwnam
  __bind_static_method__ :getpwuid, :Etc_getpwuid
  #__bind_static_method__ :group, :Etc_group
  #__bind_static_method__ :nprocessors, :Etc_nprocessors
  #__bind_static_method__ :passwd, :Etc_passwd
  __bind_static_method__ :setgrent, :Etc_setgrent
  __bind_static_method__ :setpwent, :Etc_setpwent
  #__bind_static_method__ :sysconf, :Etc_sysconf
  #__bind_static_method__ :sysconfdir, :Etc_sysconfdir
  #__bind_static_method__ :systmpdir, :Etc_systmpdir
  #__bind_static_method__ :uname, :Etc_uname
end

