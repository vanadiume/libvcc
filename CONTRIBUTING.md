# Contributing to libvcc

Thanks for wanting to help. This file covers what a change has to bring with it,
how the tree is written, and the rules that come before the rest.

- The library is ANSI C89 plus the GNU extensions VCC implements. An extension
  that is not already gated in `include/libvcc/compiler.h` needs a gate, a
  fallback, and a lane that runs both.
- The archive calls nothing. No libc, no system header, no `abort`. A change
  that adds an undefined symbol has to say why.

## Getting set up

You need V to run `build.vsh`, a C compiler from the matrix, and `ar`. VCC is
found through `$VCC`, then `PATH`, then `../vcc/vcc`; any of the three can be
named with `--cc`.

```sh
git clone https://github.com/vanadiume/libvcc
cd libvcc
v run build.vsh check
```

The gate is `check`: every available compiler, both clz paths, the tests run. It
is all or nothing, and a compiler named with `--require` that is missing is a
failure rather than a skip. Run it before asking for review, not after a machine
tells you.

## Adding a helper

1. Declare it in `include/libvcc/builtins.h`, with the libgcc name and the
   signature a compiler emits. If the ABI does not define a case, such as
   division by zero, say what the helper does with it there.
2. Put it in the file for its family in `src/`, or add a file when the family is
   new. Include the public header so the compiler checks the two against each
   other.
3. Write a test that would fail if the helper were wrong. Compare against the
   machine's own operation where the machine has one; otherwise write the
   reference a different way from the implementation. A second copy of the
   algorithm is not a reference.
4. Run `v run build.vsh check`. Both clz paths build and run; a helper that only
   works with the builtin is not finished.
5. Run `nm -u target/gcc/libvcc.a`. A new undefined name is a new dependency.

## The language, in practice

C89 means declarations at the start of a block, `/* */` comments and not `//`, no
`inline`, no `stdint.h`, and no declarations after statements. `long long` is the
one GNU extension used everywhere and every compiler in the matrix takes it.
`unsigned long long` and `unsigned int` are the types the ABI uses.

A helper that implements an operation cannot use that operation. `src/mul.c`
does not multiply 64-bit values; it multiplies 16-bit halves. `src/shift.c` does
not shift 64-bit values; it shifts two words. On a 32-bit target those are the
functions the operation compiles to, so a helper written the obvious way would
call itself.

`src/bits.c` has both shapes of one loop: the builtin path and the portable
path. When you change one, change the other and run `--portable` as well.

## Commits

Subjects are `area: the thing that changed`, lowercase, no trailing period. One
logical change per commit, each one building and passing `v run build.vsh check`
on its own. The body says why the change happened and what the old behavior was.

Areas in use: `int`, `bits`, `shift`, `mul`, `tree`, `docs`.

## Prose

Comments, commit bodies and pull request text here are read by people. Write them
the way you would say them, and leave these shapes out:

- "Serves as", "stands as" and "features" where `is`, `has` or `does` would do.
- The three-item list assembled to sound thorough, and the bolded label at the
  start of every bullet.
- Signposting before the point, and the upbeat close. Stop when the point is
  made.
- Measurements without their number and the command that produced it. An
  adjective is not evidence.

## Review

A reviewer reads for the failure first: division by zero, the most negative
value, a shift count at the edge, a target where the operation is the helper
itself. Then the ABI: is the name libgcc's, is the signature the one a compiler
emits. Then the dependencies: does `nm -u` still name only symbols defined by
objects in the same archive.

## Where to ask

Bugs and questions: <https://github.com/vanadiume/libvcc/issues>. How people are
expected to behave: `CODE_OF_CONDUCT.md`.
