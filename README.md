# libvcc

VCC's runtime library. These are the routines a compiled program calls when the
machine has no instruction for the operation it was given, or when the compiler
decides a call is smaller than the code it would inline. The names and
signatures are libgcc's, because a compiler emits `__divdi3` and not a name of
its own.

libvcc is also the first part of VCC to live outside the compiler tree. A helper
here can be built by three different C compilers and tested without building a
compiler, so the runtime can grow, change, or be left out of a target's archive
without the parser or the back end changing with it. That separation is the
point of the repository, and it is why the helper set is small.

## Status

Pre-0.1. The integer helpers are here and tested by every compiler in the
matrix. Neither the archive name nor the symbol set is a promise yet.

## What is here

| Family | Symbols | Source |
|---|---|---|
| 64-bit division | `__udivmoddi4`, `__udivdi3`, `__umoddi3`, `__divdi3`, `__moddi3` | `src/div64.c` |
| 32-bit division | `__udivmodsi4`, `__udivsi3`, `__umodsi3`, `__divsi3`, `__modsi3` | `src/div32.c` |
| 64-bit shifts | `__ashldi3`, `__ashrdi3`, `__lshrdi3` | `src/shift.c` |
| bit counts and byte order | `__clzsi2`, `__clzdi2`, `__ctzsi2`, `__ctzdi2`, `__popcountsi2`, `__popcountdi2`, `__paritysi2`, `__paritydi2`, `__bswapsi2`, `__bswapdi2` | `src/bits.c` |
| 64-bit multiply | `__muldi3` | `src/mul.c` |
| 128-bit division | `__udivmodti4`, `__udivti3`, `__umodti3`, `__divmodti4`, `__divti3`, `__modti3` | `src/div128.c` |
| 128-bit shifts | `__ashlti3`, `__ashrti3`, `__lshrti3` | `src/shift128.c` |
| 128-bit bit counts | `__clzti2`, `__ctzti2`, `__popcountti2`, `__parityti2` | `src/bits128.c` |
| 128-bit multiply | `__multi3` | `src/mul128.c` |

A 128-bit value is libgcc's TImode. `libvcc_u128` in the ABI header is the two
64-bit words, and every 128-bit helper works a word at a time, so none of them
is written with the operation it provides. That is also the machine ABI: on
x86-64 SysV the pair and `unsigned __int128` travel in the same registers, in
the same order. `test/abi128.c` is the measurement. It declares these symbols
with the compiler's own type, in every lane, and checks the answers against
instructions the library does not use: a shift, a multiply, and divisions whose
divisor is a power of two.

`include/libvcc/builtins.h` is the ABI: the declarations, the count ranges, and
the one case the ABI does not define. `include/libvcc/compiler.h` says which
compiler is reading the tree and which extension it gets.

The archive calls nothing. There is no libc in it, no system header, no abort.
`nm -u` names only calls between objects in the same archive: the clz helpers,
and the 64-bit counts the 128-bit ones are built from.

## Build

V runs the build script; a C compiler from the matrix and `ar` do the work.

```sh
v run build.vsh                 # vcc if it is found, otherwise gcc, clang
v run build.vsh --cc gcc        # one compiler
v run build.vsh check           # every available compiler, both clz paths
v run build.vsh clean
```

`check` is the gate. It builds and runs the tests for each compiler twice: once
with `__builtin_clzll` and once with `-DLIBVCC_PORTABLE=1`, so the fallback is
compiled and executed rather than assumed. It exits non-zero if any lane fails,
and a compiler named with `--require` that is missing is a failure too.

Artifacts land in `target/<lane>/` as `libvcc.a` and a test binary. The directory
is not called `build` because `v run build.vsh` names the executable it produces
after the script, which is `build`, and a directory of that name is a collision.

The test binary prints its own count. There is no total in this document to go
stale.

## The language

ANSI C89, plus the GNU extensions VCC implements. In practice:

- `long long` is the one extension used throughout. It is C99's and a GNU
  extension in C89; all three compilers take it.
- `__builtin_clzll` is behind `LIBVCC_HAVE_BUILTIN_CLZLL`, with a portable loop
  beside it. Both paths are built and run by `check`.
- Anything else is gated in `compiler.h` by compiler identity, or it is not
  used. An unknown compiler is an error there, not a guess.
- No C99 headers, no `//` comments, no declarations after statements, no
  `inline`. The compiler this library is for is still growing into C99, and the
  less it has to grow before it can build its own runtime, the better.

## The matrix

Measured with vcc 0.0.3, gcc 16.2.1 and clang 23.1.3, the compilers this tree is
bootstrapped on. `v run build.vsh check` prints the versions it finds.

- **vcc** identifies itself with `__VCC__`. That is asked first, because vcc also
  defines `__GNUC__` in a GNU dialect. Its lane is built at `-O2` like the other
  two, which needs a vcc built after the optimizer fix in vanadiume/vcc#10.
  It is also the compiler the 128-bit helpers had to wait for: 0.0.2 computed a
  division or a right shift of `unsigned __int128` with something other than the
  value's high word (vanadiume/vcc#13, fixed by the unsigned widening in PR
  #14), so `test/abi128.c` sat behind a gate until 0.0.3. It inlines a 128-bit
  division, so a program it compiles does not call these helpers yet.
- **gcc** and **clang** are built at `-O2` with `-ffreestanding
  -fno-stack-protector`, so the archive stays free of libc. Distribution
  defaults would otherwise put `__stack_chk_fail` in two objects.

The flag is `-std=gnu89`, not `-std=c89`: `long long` is the extension the tree
uses everywhere, and `-pedantic` under c89 would report the language the project
chose to write in. `-Wall -Wextra -Werror` are on for every compiler that takes
them.

## Linking

Link `libvcc.a` instead of `libgcc.a`. The two define the same symbols, and the
linker takes the first definition it finds, so they are not linked together. The
archive is built by one compiler and can be linked by another, because nothing in
it depends on which compiler produced it.

A compiler that emits one of these calls uses the libgcc name. This archive is
what VCC will link against when it has a target or an operation that needs one.

## What is not here yet

- **Soft float and the conversions** (`__fixdfdi`, `__floatdidf`, and the rest).
- **The helpers that trap.** `__addvti3`, `__subvti3`, `__mulvti3`, `__negvti2`
  and `__absvti2` in libgcc call `abort` when the result does not fit, and this
  archive calls nothing. A version that handed the caller a flag instead would
  be a different ABI wearing the same name, so these wait for a decision rather
  than a file.
- **Complex helpers.** VCC currently emits the arithmetic for `__muldc3`,
  `__divdc3` and their `long double` relatives inline, because there was no
  library to call. Those are the obvious next things to move here, and moving
  them changes what the compiler emits, so each one comes with its own
  measurement.
- **Unwinding, the stack protector, sanitizers, `__clear_cache`.** Each needs a
  decision about what the library may call, and this one calls nothing.

## Why the names are libgcc's

The ABI is the compiler's, not this repository's. A program compiled by gcc,
clang or vcc expects `__divdi3` when it divides a 64-bit value on a target
without the instruction; a `libvcc_` prefix would make the archive useless to
every one of them. The prefix is kept for names that are not part of that ABI.

## Licence

BSD 3-Clause. See `LICENSE`.
