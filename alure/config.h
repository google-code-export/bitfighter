/* Define to the version of the library being built */
#define ALURE_VER_MAJOR 1
#define ALURE_VER_MINOR 1

#if defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
#define WIN
#endif

#ifndef WIN
/* Define if we have GCC's constructor attribute */
#define HAVE_GCC_CONSTRUCTOR

/* Define if we have GCC's visibility attribute */
#define HAVE_GCC_VISIBILITY

/* Define if we have dlfcn.h */
#define HAVE_DLFCN_H

/* Define if we have sys/wait.h */
#define HAVE_SYS_WAIT_H

/* Define if we have nanosleep */
#define HAVE_NANOSLEEP

/* Define if we have fseeko */
#define HAVE_FSEEKO
#endif

#ifdef WIN
/* Define if we have windows.h */
#define HAVE_WINDOWS_H

/* Define if we have _fseeki64 */
#define HAVE__FSEEKI64
#endif


/* Define if we have sys/types.h */
#define HAVE_SYS_TYPES_H

/* Define if we have signal.h */
#define HAVE_SIGNAL_H


/* Define if we have pthread_np.h */
/* #undef HAVE_PTHREAD_NP_H */

/* Define the supported backends */
/* #undef HAS_VORBISFILE */
/* #undef HAS_FLAC */
/* #undef HAS_SNDFILE */
/* #undef HAS_FLUIDSYNTH */
/* #undef HAS_DUMB */
/* #undef HAS_MODPLUG */
/* #undef HAS_MPG123 */
