#include "natalie.hpp"

#include <signal.h>

namespace Natalie {

Value SignalModule::list(Env *env) {
    return HashObject::create(env, {
                                       StringObject::create("EXIT"),
                                       Value::integer(static_cast<nat_int_t>(0)),
                                       StringObject::create("HUP"),
                                       Value::integer(static_cast<nat_int_t>(SIGHUP)),
                                       StringObject::create("INT"),
                                       Value::integer(static_cast<nat_int_t>(SIGINT)),
                                       StringObject::create("QUIT"),
                                       Value::integer(static_cast<nat_int_t>(SIGQUIT)),
                                       StringObject::create("ILL"),
                                       Value::integer(static_cast<nat_int_t>(SIGILL)),
#ifdef SIGTRAP
                                       StringObject::create("TRAP"),
                                       Value::integer(static_cast<nat_int_t>(SIGTRAP)),
#endif
                                       StringObject::create("ABRT"),
                                       Value::integer(static_cast<nat_int_t>(SIGABRT)),
#ifdef SIGIOT
                                       StringObject::create("IOT"),
                                       Value::integer(static_cast<nat_int_t>(SIGIOT)),
#endif
#ifdef SIGEMT
                                       StringObject::create("EMT"),
                                       Value::integer(static_cast<nat_int_t>(SIGEMT)),
#endif
                                       StringObject::create("FPE"),
                                       Value::integer(static_cast<nat_int_t>(SIGFPE)),
                                       StringObject::create("KILL"),
                                       Value::integer(static_cast<nat_int_t>(SIGKILL)),
#ifdef SIGBUS
                                       StringObject::create("BUS"),
                                       Value::integer(static_cast<nat_int_t>(SIGBUS)),
#endif
                                       StringObject::create("SEGV"),
                                       Value::integer(static_cast<nat_int_t>(SIGSEGV)),
                                       StringObject::create("SYS"),
                                       Value::integer(static_cast<nat_int_t>(SIGSYS)),
                                       StringObject::create("PIPE"),
                                       Value::integer(static_cast<nat_int_t>(SIGPIPE)),
                                       StringObject::create("ALRM"),
                                       Value::integer(static_cast<nat_int_t>(SIGALRM)),
                                       StringObject::create("TERM"),
                                       Value::integer(static_cast<nat_int_t>(SIGTERM)),
#ifdef SIGURG
                                       StringObject::create("URG"),
                                       Value::integer(static_cast<nat_int_t>(SIGURG)),
#endif
                                       StringObject::create("STOP"),
                                       Value::integer(static_cast<nat_int_t>(SIGSTOP)),
                                       StringObject::create("TSTP"),
                                       Value::integer(static_cast<nat_int_t>(SIGTSTP)),
                                       StringObject::create("CONT"),
                                       Value::integer(static_cast<nat_int_t>(SIGCONT)),
                                       StringObject::create("CHLD"),
                                       Value::integer(static_cast<nat_int_t>(SIGCHLD)),
                                       StringObject::create("CLD"),
#ifdef SIGCLD
                                       Value::integer(static_cast<nat_int_t>(SIGCLD)),
#else
                                       Value::integer(static_cast<nat_int_t>(SIGCHLD)),
#endif
                                       StringObject::create("TTIN"),
                                       Value::integer(static_cast<nat_int_t>(SIGTTIN)),
                                       StringObject::create("TTOU"),
                                       Value::integer(static_cast<nat_int_t>(SIGTTOU)),
#ifdef SIGIO
                                       StringObject::create("IO"),
                                       Value::integer(static_cast<nat_int_t>(SIGIO)),
#endif
#ifdef SIGXCPU
                                       StringObject::create("XCPU"),
                                       Value::integer(static_cast<nat_int_t>(SIGXCPU)),
#endif
#ifdef SIGXFSZ
                                       StringObject::create("XFSZ"),
                                       Value::integer(static_cast<nat_int_t>(SIGXFSZ)),
#endif
#ifdef SIGVTALRM
                                       StringObject::create("VTALRM"),
                                       Value::integer(static_cast<nat_int_t>(SIGVTALRM)),
#endif
#ifdef SIGPROF
                                       StringObject::create("PROF"),
                                       Value::integer(static_cast<nat_int_t>(SIGPROF)),
#endif
#ifdef SIGWINCH
                                       StringObject::create("WINCH"),
                                       Value::integer(static_cast<nat_int_t>(SIGWINCH)),
#endif
                                       StringObject::create("USR1"),
                                       Value::integer(static_cast<nat_int_t>(SIGUSR1)),
                                       StringObject::create("USR2"),
                                       Value::integer(static_cast<nat_int_t>(SIGUSR2)),
#ifdef SIGLOST
                                       StringObject::create("LOST"),
                                       Value::integer(static_cast<nat_int_t>(SIGLOST)),
#endif
#ifdef SIGMSG
                                       StringObject::create("MSG"),
                                       Value::integer(static_cast<nat_int_t>(SIGMSG)),
#endif
#ifdef SIGPWR
                                       StringObject::create("PWR"),
                                       Value::integer(static_cast<nat_int_t>(SIGPWR)),
#endif
#ifdef SIGPOLL
                                       StringObject::create("POLL"),
                                       Value::integer(static_cast<nat_int_t>(SIGPOLL)),
#endif
#ifdef SIGDANGER
                                       StringObject::create("DANGER"),
                                       Value::integer(static_cast<nat_int_t>(SIGDANGER)),
#endif
#ifdef SIGMIGRATE
                                       StringObject::create("MIGRATE"),
                                       Value::integer(static_cast<nat_int_t>(SIGMIGRATE)),
#endif
#ifdef SIGPRE
                                       StringObject::create("PRE"),
                                       Value::integer(static_cast<nat_int_t>(SIGPRE)),
#endif
#ifdef SIGGRANT
                                       StringObject::create("GRANT"),
                                       Value::integer(static_cast<nat_int_t>(SIGGRANT)),
#endif
#ifdef SIGRETRACT
                                       StringObject::create("RETRACT"),
                                       Value::integer(static_cast<nat_int_t>(SIGRETRACT)),
#endif
#ifdef SIGSOUND
                                       StringObject::create("SOUND"),
                                       Value::integer(static_cast<nat_int_t>(SIGSOUND)),
#endif
#ifdef SIGINFO
                                       StringObject::create("INFO"),
                                       Value::integer(static_cast<nat_int_t>(SIGINFO)),
#endif
                                   });
}

Value SignalModule::signame(Env *env, Value signal) {
    signal = signal.to_int(env);
    return list(env).send(env, "key"_s, { signal });
}

}
