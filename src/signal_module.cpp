#include "natalie.hpp"

#include <signal.h>

namespace Natalie {

Value SignalModule::list(Env *env) {
    return new HashObject { env, {
                                     new StringObject { "EXIT" },
                                     IntegerObject::create(static_cast<nat_int_t>(0)),
                                     new StringObject { "HUP" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGHUP)),
                                     new StringObject { "INT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGINT)),
                                     new StringObject { "QUIT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGQUIT)),
                                     new StringObject { "ILL" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGILL)),
#ifdef SIGTRAP
                                     new StringObject { "TRAP" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGTRAP)),
#endif
                                     new StringObject { "ABRT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGABRT)),
#ifdef SIGIOT
                                     new StringObject { "IOT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGIOT)),
#endif
#ifdef SIGEMT
                                     new StringObject { "EMT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGEMT)),
#endif
                                     new StringObject { "FPE" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGFPE)),
                                     new StringObject { "KILL" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGKILL)),
#ifdef SIGBUS
                                     new StringObject { "BUS" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGBUS)),
#endif
                                     new StringObject { "SEGV" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGSEGV)),
                                     new StringObject { "SYS" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGSYS)),
                                     new StringObject { "PIPE" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGPIPE)),
                                     new StringObject { "ALRM" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGALRM)),
                                     new StringObject { "TERM" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGTERM)),
#ifdef SIGURG
                                     new StringObject { "URG" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGURG)),
#endif
                                     new StringObject { "STOP" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGSTOP)),
                                     new StringObject { "TSTP" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGTSTP)),
                                     new StringObject { "CONT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGCONT)),
                                     new StringObject { "CHLD" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGCHLD)),
                                     new StringObject { "CLD" },
#ifdef SIGCLD
                                     IntegerObject::create(static_cast<nat_int_t>(SIGCLD)),
#else
                                     IntegerObject::create(static_cast<nat_int_t>(SIGCHLD)),
#endif
                                     new StringObject { "TTIN" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGTTIN)),
                                     new StringObject { "TTOU" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGTTOU)),
#ifdef SIGIO
                                     new StringObject { "IO" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGIO)),
#endif
#ifdef SIGXCPU
                                     new StringObject { "XCPU" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGXCPU)),
#endif
#ifdef SIGXFSZ
                                     new StringObject { "XFSZ" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGXFSZ)),
#endif
#ifdef SIGVTALRM
                                     new StringObject { "VTALRM" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGVTALRM)),
#endif
#ifdef SIGPROF
                                     new StringObject { "PROF" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGPROF)),
#endif
#ifdef SIGWINCH
                                     new StringObject { "WINCH" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGWINCH)),
#endif
                                     new StringObject { "USR1" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGUSR1)),
                                     new StringObject { "USR2" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGUSR2)),
#ifdef SIGLOST
                                     new StringObject { "LOST" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGLOST)),
#endif
#ifdef SIGMSG
                                     new StringObject { "MSG" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGMSG)),
#endif
#ifdef SIGPWR
                                     new StringObject { "PWR" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGPWR)),
#endif
#ifdef SIGPOLL
                                     new StringObject { "POLL" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGPOLL)),
#endif
#ifdef SIGDANGER
                                     new StringObject { "DANGER" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGDANGER)),
#endif
#ifdef SIGMIGRATE
                                     new StringObject { "MIGRATE" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGMIGRATE)),
#endif
#ifdef SIGPRE
                                     new StringObject { "PRE" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGPRE)),
#endif
#ifdef SIGGRANT
                                     new StringObject { "GRANT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGGRANT)),
#endif
#ifdef SIGRETRACT
                                     new StringObject { "RETRACT" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGRETRACT)),
#ifdef SIGSOUND
#endif
                                     new StringObject { "SOUND" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGSOUND)),
#endif
#ifdef SIGINFO
                                     new StringObject { "INFO" },
                                     IntegerObject::create(static_cast<nat_int_t>(SIGINFO)),
#endif
                                 } };
}

Value SignalModule::signame(Env *env, Value signal) {
    signal = signal->to_int(env);
    return list(env)->send(env, "key"_s, { signal });
}

}
