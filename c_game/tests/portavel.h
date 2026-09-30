/*
 * portavel.h - o que as ferramentas de teste (make curva, make robos, make test-save) usam e o Windows (MinGW)
 * não tem igual ao POSIX: a contagem de núcleos, mkdir, getpid e a pasta temporária. Vai depois dos <includes>
 * do sistema, porque redefine mkdir.
 */
#ifndef APARA_TESTE_PORTAVEL_H
#define APARA_TESTE_PORTAVEL_H

#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#define mkdir(caminho, modo) _mkdir(caminho)
#define getpid _getpid
static inline long nucleos_online(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (long)si.dwNumberOfProcessors;
}
static inline const char *pasta_do_sistema(void) {
    const char *t = getenv("TEMP");
    return t ? t : (getenv("TMP") ? getenv("TMP") : ".");
}
#else
#include <unistd.h>
static inline long nucleos_online(void) { return sysconf(_SC_NPROCESSORS_ONLN); }
static inline const char *pasta_do_sistema(void) { return getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp"; }
#endif

#endif
