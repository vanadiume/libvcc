# AGENTS.md

Instructions for coding agents working in this repository. They are the same
rules a person follows, written where a tool will read them. `CONTRIBUTING.md`
is the human-facing version.

## What this repository is

libvcc is VCC's runtime library: the helpers a compiled program calls when the
target has no instruction for an operation. It is also where VCC starts being
more than one tree. A helper here is built and tested by three C compilers
without building a compiler, so the runtime can change without the parser or the
back end changing with it.

The helper set is deliberately small. A function belongs here when a program
links against it, not because the compiler happens to have a function with the
same job.

## The rules

1. **ANSI C89 plus the GNU extensions vcc implements.** `long long` is the one
   used throughout and needs no gate. `__builtin_clzll` is behind
   `LIBVCC_HAVE_BUILTIN_CLZLL` in `include/libvcc/compiler.h`, with a portable
   fallback that `-DLIBVCC_PORTABLE=1` selects and that `check` builds and runs.
   Any further extension gets the same treatment: a gate in that header, a
   fallback beside it, and a lane that exercises both. An extension that cannot
   be built and run on every compiler in the matrix stays out.
2. **The archive calls nothing.** No libc, no system header, no `abort`. A helper
   that needs one is a design discussion before it is a link flag. The check is
   `nm -u target/gcc/libvcc.a`: every name it prints is defined by another object
   in the same archive, and nothing else is.
3. **The symbol names and signatures are libgcc's.** `__divdi3`, not
   `libvcc_divdi3`; a rename breaks every compiler that emits the call. Internal
   names carry `libvcc_` or are `static`. For TImode the C spelling is the
   two-word pair in `builtins.h`, because no compiler here has a value of that
   width to declare; the machine ABI is still libgcc's, and `test/abi128.c`
   measures it with the compiler's own type.
4. **A helper is not tested against itself.** Division is compared with the
   machine's division, shifts with the machine's shifts, and counts and byte
   swaps with a reference written a different way. A second copy of the
   algorithm proves only that the copy agrees. 128-bit division has no machine
   division on this target, so it is checked against the definition instead:
   `n == q * d + r` with `r < d` fixes the pair, and the product in that
   identity is one the library does not compute.

## Commands

```sh
v run build.vsh                    # build and test with the first compiler found
v run build.vsh --cc gcc           # name one: vcc, gcc or clang
v run build.vsh test --portable    # the fallback path of the clz gate
v run build.vsh check              # every compiler, both paths; the gate
v run build.vsh check --require gcc,clang
v run build.vsh clean
ar t target/gcc/libvcc.a           # read the archive back
nm -u target/gcc/libvcc.a          # the archive's dependencies
```

`check` exits non-zero if any lane fails. No lane is allowed to fail, and a
compiler named with `--require` that is missing is a failure rather than a skip.

## Where things go

| Path | Rule |
|---|---|
| `include/libvcc/compiler.h` | compiler identity and every feature gate |
| `include/libvcc/builtins.h` | the ABI: declarations, count ranges, undefined cases |
| `src/` | one file per family; a static helper stays in the file that uses it |
| `src/word.h` | the two halves of a 64-bit value, as macros |
| `test/` | one file per family, a shared harness, references written independently; `abi128.c` is the caller's side of the 128-bit ABI |
| `build.vsh` | the whole build; there is no Makefile and no generated input |

## What costs time

Measured on vcc 0.0.2, gcc 16.2.1 and clang 23.1.3.

- **vcc defines `__GNUC__` in a GNU dialect.** Asking `__GNUC__` first reads vcc
  as gcc. `compiler.h` asks `__VCC__` first for that reason.
- **vcc's `__STDC_VERSION__` is 199901L in every mode.** It cannot tell you
  which `-std=` was selected.
- **vcc refuses an implicit narrowing store whose value is 64 bits.** In
  `test/div32.c`, `d = (unsigned int)1 << j` with `unsigned long j` is
  `unsupported: the value is one this back end cannot size, so it cannot be
  stored`; a 32-bit loop counter fixes it. This is the same family as the -O1
  bug, and why that loop declares `k`.
- **vcc has `__int128` as a type and not a value.** It parses the type and sizes
  it; a division of two values above 2^64 does not answer the quotient, and a
  right shift by 64 or more answers the wrong half, both measured on 0.0.2. That
  is why the 128-bit helpers take the two-word pair and why `src/` never spells
  the type. `test/abi128.c` does, behind `LIBVCC_HAVE_INT128`, because a caller
  compiled by gcc or clang is the only thing that can show the pair and the type
  are the same object on the wire.
- **gcc and clang need `-fno-stack-protector` here.** With the distribution
  defaults measured, `__udivmoddi4`'s address-taken remainder gets a canary and
  the object gains an undefined `__stack_chk_fail`. `build.vsh` passes
  `-ffreestanding -fno-stack-protector` so the archive stays free of libc.

<!-- omh:agent-instructions:begin -->
## Verification

- `v run build.vsh check` is the whole gate: both clz paths, every available
  compiler, the tests run. It exits non-zero on any failure.
- `nm -u target/<lane>/libvcc.a` names only symbols defined elsewhere in the
  same archive. A new name is a new dependency and has to be explained.
- `ar t` shows one object per source file. The archive's defined symbols are the
  ABI in `include/libvcc/builtins.h`, and nothing else is exported.
- Do not write counts, test totals, or line numbers into this file or the
  README. The commands print them, and the numbers drift.
- When a test fails, fix the code, or change the expectation and say in the
  commit why the behavior change was intended. Never edit a test to quiet it.
<!-- omh:agent-instructions:end -->
