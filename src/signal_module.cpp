#include "natalie.hpp"

#include <signal.h>

namespace Natalie {

Value SignalModule::list(Env *env) {
    return new HashObject { env, {
                                     new StringObject { "EXIT" },
                                     Value::integer(static_cast<nat_int_t>(0)),
                                     new StringObject { "HUP" },
                                     Value::integer(static_cast<nat_int_t>(SIGHUP)),
                                     new StringObject { "INT" },
                                     Value::integer(static_cast<nat_int_t>(SIGINT)),
                                     new StringObject { "QUIT" },
                                     Value::integer(static_cast<nat_int_t>(SIGQUIT)),
                                     new StringObject { "ILL" },
                                     Value::integer(static_cast<nat_int_t>(SIGILL)),
#ifdef SIGTRAP
                                     new StringObject { "TRAP" },
                                     Value::integer(static_cast<nat_int_t>(SIGTRAP)),
#endif
                                     new StringObject { "ABRT" },
                                     Value::integer(static_cast<nat_int_t>(SIGABRT)),
#ifdef SIGIOT
                                     new StringObject { "IOT" },
                                     Value::integer(static_cast<nat_int_t>(SIGIOT)),
#endif
#ifdef SIGEMT
                                     new StringObject { "EMT" },
                                     Value::integer(static_cast<nat_int_t>(SIGEMT)),
#endif
                                     new StringObject { "FPE" },
                                     Value::integer(static_cast<nat_int_t>(SIGFPE)),
                                     new StringObject { "KILL" },
                                     Value::integer(static_cast<nat_int_t>(SIGKILL)),
#ifdef SIGBUS
                                     new StringObject { "BUS" },
                                     Value::integer(static_cast<nat_int_t>(SIGBUS)),
#endif
                                     new StringObject { "SEGV" },
                                     Value::integer(static_cast<nat_int_t>(SIGSEGV)),
                                     new StringObject { "SYS" },
                                     Value::integer(static_cast<nat_int_t>(SIGSYS)),
                                     new StringObject { "PIPE" },
                                     Value::integer(static_cast<nat_int_t>(SIGPIPE)),
                                     new StringObject { "ALRM" },
                                     Value::integer(static_cast<nat_int_t>(SIGALRM)),
                                     new StringObject { "TERM" },
                                     Value::integer(static_cast<nat_int_t>(SIGTERM)),
#ifdef SIGURG
                                     new StringObject { "URG" },
                                     Value::integer(static_cast<nat_int_t>(SIGURG)),
#endif
                                     new StringObject { "STOP" },
                                     Value::integer(static_cast<nat_int_t>(SIGSTOP)),
                                     new StringObject { "TSTP" },
                                     Value::integer(static_cast<nat_int_t>(SIGTSTP)),
                                     new StringObject { "CONT" },
                                     Value::integer(static_cast<nat_int_t>(SIGCONT)),
                                     new StringObject { "CHLD" },
                                     Value::integer(static_cast<nat_int_t>(SIGCHLD)),
                                     new StringObject { "CLD" },
#ifdef SIGCLD
                                     Value::integer(static_cast<nat_int_t>(SIGCLD)),
#else
                                     Value::integer(static_cast<nat_int_t>(SIGCHLD)),
#endif
                                     new StringObject { "TTIN" },
                                     Value::integer(static_cast<nat_int_t>(SIGTTIN)),
                                     new StringObject { "TTOU" },
                                     Value::integer(static_cast<nat_int_t>(SIGTTOU)),
#ifdef SIGIO
                                     new StringObject { "IO" },
                                     Value::integer(static_cast<nat_int_t>(SIGIO)),
#endif
#ifdef SIGXCPU
                                     new StringObject { "XCPU" },
                                     Value::integer(static_cast<nat_int_t>(SIGXCPU)),
#endif
#ifdef SIGXFSZ
                                     new StringObject { "XFSZ" },
                                     Value::integer(static_cast<nat_int_t>(SIGXFSZ)),
#endif
#ifdef SIGVTALRM
                                     new StringObject { "VTALRM" },
                                     Value::integer(static_cast<nat_int_t>(SIGVTALRM)),
#endif
#ifdef SIGPROF
                                     new StringObject { "PROF" },
                                     Value::integer(static_cast<nat_int_t>(SIGPROF)),
#endif
#ifdef SIGWINCH
                                     new StringObject { "WINCH" },
                                     Value::integer(static_cast<nat_int_t>(SIGWINCH)),
#endif
                                     new StringObject { "USR1" },
                                     Value::integer(static_cast<nat_int_t>(SIGUSR1)),
                                     new StringObject { "USR2" },
                                     Value::integer(static_cast<nat_int_t>(SIGUSR2)),
#ifdef SIGLOST
                                     new StringObject { "LOST" },
                                     Value::integer(static_cast<nat_int_t>(SIGLOST)),
#endif
#ifdef SIGMSG
                                     new StringObject { "MSG" },
                                     Value::integer(static_cast<nat_int_t>(SIGMSG)),
#endif
#ifdef SIGPWR
                                     new StringObject { "PWR" },
                                     Value::integer(static_cast<nat_int_t>(SIGPWR)),
#endif
#ifdef SIGPOLL
                                     new StringObject { "POLL" },
                                     Value::integer(static_cast<nat_int_t>(SIGPOLL)),
#endif
#ifdef SIGDANGER
                                     new StringObject { "DANGER" },
                                     Value::integer(static_cast<nat_int_t>(SIGDANGER)),
#endif
#ifdef SIGMIGRATE
                                     new StringObject { "MIGRATE" },
                                     Value::integer(static_cast<nat_int_t>(SIGMIGRATE)),
#endif
#ifdef SIGPRE
                                     new StringObject { "PRE" },
                                     Value::integer(static_cast<nat_int_t>(SIGPRE)),
#endif
#ifdef SIGGRANT
                                     new StringObject { "GRANT" },
                                     Value::integer(static_cast<nat_int_t>(SIGGRANT)),
#endif
#ifdef SIGRETRACT
                                     new StringObject { "RETRACT" },
                                     Value::integer(static_cast<nat_int_t>(SIGRETRACT)),
#ifdef SIGSOUND
#endif
                                     new StringObject { "SOUND" },
                                     Value::integer(static_cast<nat_int_t>(SIGSOUND)),
#endif
#ifdef SIGINFO
                                     new StringObject { "INFO" },
                                     Value::integer(static_cast<nat_int_t>(SIGINFO)),
#endif
                                 } };
}

Value SignalModule::signame(Env *env, Value signal) {
    signal = signal.to_int(env);
    return list(env).send(env, "key"_s, { signal });
}

}
