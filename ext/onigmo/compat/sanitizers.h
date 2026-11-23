/* MRI's regparse.c needs only this declaration annotation. */
#ifndef CORA_ONIGMO_SANITIZERS_H
#define CORA_ONIGMO_SANITIZERS_H
#define NO_SANITIZE(kind, decl) decl __attribute__((no_sanitize(kind)))
#endif
