#ifndef LIBVCC_WORD_H
#define LIBVCC_WORD_H

/*
 * The two halves of a 64-bit value, and the way back.
 *
 * The shift helpers are written in terms of these rather than in terms of a
 * 64-bit shift. On a machine where a 64-bit shift is one instruction the
 * compiler folds them back into it; on a machine where it is not, the helpers
 * are the only thing there is to call, so they cannot be written with the
 * operation they provide.
 *
 * These are macros and not functions on purpose. A static function in a
 * header that a file does not use is an unused-function warning, and a
 * non-static one is a symbol in the archive for no reason.
 */

#define LIBVCC_LO64(x) ((unsigned int)(unsigned long long)(x))
#define LIBVCC_HI64(x) ((unsigned int)((unsigned long long)(x) >> 32))
#define LIBVCC_PACK64(hi, lo) \
	(((unsigned long long)(unsigned int)(hi) << 32) | \
	 (unsigned long long)(unsigned int)(lo))

#endif /* LIBVCC_WORD_H */
