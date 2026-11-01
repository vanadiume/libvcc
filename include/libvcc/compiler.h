#ifndef LIBVCC_COMPILER_H
#define LIBVCC_COMPILER_H

/*
 * Which compiler is reading libvcc, and which extensions it has been measured
 * to support.
 *
 * The tree is ANSI C89. The one extension used everywhere is long long, which
 * is C99's and a GNU extension in C89: vcc, gcc and clang all take it, so it
 * needs no gate. Everything past that goes through a macro here, with a
 * portable path beside it, and a compiler that is not in this list stops the
 * build instead of being guessed at.
 *
 * The order matters. vcc defines __VCC__ and, in a GNU dialect, __GNUC__ as
 * well; -femulation=clang makes it define __clang__ too. Asking __VCC__ first
 * keeps a vcc build from being read as gcc, and __clang__ comes before
 * __GNUC__ because clang defines both.
 *
 * Adding a compiler means adding its identity below, then measuring each
 * feature macro against it. A claim copied from a similar compiler is how a
 * runtime library ends up with a path nobody built.
 */

#if defined(__VCC__)
#define LIBVCC_COMPILER_VCC 1
#elif defined(__clang__)
#define LIBVCC_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define LIBVCC_COMPILER_GCC 1
#else
#error "libvcc: unknown C compiler; add its identity to include/libvcc/compiler.h"
#endif

/*
 * __builtin_clzll counts the leading zero bits of a nonzero unsigned long
 * long. vcc, gcc and clang all emit an instruction or a short sequence for it.
 *
 * The portable path is what a new compiler starts on, so it is not dead code:
 * -DLIBVCC_PORTABLE=1 selects it, and `v run build.vsh check` builds and tests
 * both paths for every compiler it finds.
 */
#if defined(LIBVCC_PORTABLE)
#define LIBVCC_HAVE_BUILTIN_CLZLL 0
#else
#define LIBVCC_HAVE_BUILTIN_CLZLL 1
#endif

/*
 * A 128-bit value, as opposed to a 128-bit type.
 *
 * vcc parses unsigned __int128 and sizes it, and does not compute with it:
 * measured on vcc 0.0.2, a division of two values above 2^64 answers a number
 * that is not the quotient, and a right shift by 64 or more answers the wrong
 * half. gcc and clang compute both.
 *
 * No source file in src/ uses the type: the helpers take the two-word pair in
 * include/libvcc/builtins.h and do their work a word at a time, which is what
 * makes them buildable here. The gate is for test/abi128.c, which has to spell
 * the type the compiler emits a call with, because only a caller's own type can
 * show that the pair and the type are the same object on the wire.
 */
#if defined(LIBVCC_COMPILER_VCC)
#define LIBVCC_HAVE_INT128 0
#else
#define LIBVCC_HAVE_INT128 1
#endif

#endif /* LIBVCC_COMPILER_H */
