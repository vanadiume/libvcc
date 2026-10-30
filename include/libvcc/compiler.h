#ifndef LIBVCC_COMPILER_H
#define LIBVCC_COMPILER_H

/*
 * Which compiler is reading libvcc, and which extensions it has been measured
 * to support.
 *
 * The tree is ANSI C89. The one extension used everywhere is long long, which
 * is C99's and a GNU extension in C89: vcc, gcc, clang and tcc all take it, so
 * it needs no gate. Everything past that goes through a macro here, with a
 * portable path beside it, and a compiler that is not in this list stops the
 * build instead of being guessed at.
 *
 * The order matters. vcc defines __VCC__ and, in a GNU dialect, __GNUC__ as
 * well; -femulation=clang makes it define __clang__ too. Asking __VCC__ first
 * keeps a vcc build from being read as gcc. tcc defines __TINYC__ and no
 * __GNUC__, but it is asked before __GNUC__ anyway for the day that changes.
 *
 * Adding a compiler means adding its identity below, then measuring each
 * feature macro against it. A claim copied from a similar compiler is how a
 * runtime library ends up with a path nobody built.
 */

#if defined(__VCC__)
#define LIBVCC_COMPILER_VCC 1
#elif defined(__clang__)
#define LIBVCC_COMPILER_CLANG 1
#elif defined(__TINYC__)
#define LIBVCC_COMPILER_TCC 1
#elif defined(__GNUC__)
#define LIBVCC_COMPILER_GCC 1
#else
#error "libvcc: unknown C compiler; add its identity to include/libvcc/compiler.h"
#endif

/*
 * __builtin_clzll counts the leading zero bits of a nonzero unsigned long
 * long. vcc, gcc and clang emit an instruction or a short sequence for it.
 * tcc parses the name and emits a call into libtcc1, which is a dependency the
 * archive does not take, so tcc takes the portable loop: the tcc archive's
 * nm -u is what measured that.
 *
 * The portable path is also what a new compiler starts on, so it is not dead
 * code: -DLIBVCC_PORTABLE=1 selects it, and `v run build.vsh check` builds and
 * tests both paths for every compiler it finds.
 */
#if defined(LIBVCC_PORTABLE) || defined(LIBVCC_COMPILER_TCC)
#define LIBVCC_HAVE_BUILTIN_CLZLL 0
#else
#define LIBVCC_HAVE_BUILTIN_CLZLL 1
#endif

#endif /* LIBVCC_COMPILER_H */
