#!/usr/bin/env -S v run

// libvcc's build front door. One script, because the tree is small enough
// that a generated Makefile would be a second thing to keep in step.
//
// `build` compiles src/ into target/<lane>/libvcc.a and links a second set of
// objects into target/<lane>/libvcc.so.0.1, with the libvcc.so.0 and
// libvcc.so symlinks beside it: the archive for a program that links
// statically, the shared object for one that does not. The archive's objects
// are compiled once and the shared object's a second time with -fpic, the way
// libgcc splits libgcc.a and libgcc_s.so. The shared object takes no soname
// from a compiler that does not record one; that is probed, named, and not a
// failure.
//
// `test` builds, then compiles and runs the tests against the archive and
// again against the shared object, and `check` does both for every compiler of
// the matrix, twice: once with the __builtin_clzll path and once with
// -DLIBVCC_PORTABLE=1, so the fallback is built and run rather than assumed.
// `check` is the gate; a missing compiler is reported, and one named with
// --require is a failure.
//
// The version the tree publishes is in VERSION, one line, and the banner and
// the shared object's name are both read from it.
//
// vcc is built at -O2 like the rest of the matrix. It used to be pinned to
// -O0 over a bug that mistyped an arithmetic operand of a bitwise operator;
// vanadiume/vcc#9 was fixed in PR #10, and the lane moved back.
//
// The compilers are found by name. vcc is looked for through $VCC, then PATH,
// then ../vcc/vcc, which is where it sits in a vanadium checkout.

import os

const matrix = ['vcc', 'gcc', 'clang']
const steps = ['build', 'test', 'check', 'clean', 'list']
const archive_name = 'libvcc.a'
const shared_base = 'libvcc.so'

struct Options {
mut:
	root     string
	version  string
	step     string
	cc       string
	require  []string
	release  bool = true
	portable bool
	verbose  bool
}

struct Compiler {
	name    string
	path    string
	version string
}

struct Outcome {
mut:
	total  int
	failed int
}

fn main() {
	mut opts := parse(os.args[1..]) or {
		usage()
		exit(2)
	}
	opts.root = os.dir(@FILE)
	os.chdir(opts.root) or {
		|e| eprintln('build: cannot enter ${opts.root}: ${e}')
		exit(2)
	}
	opts.version = read_version()
	match opts.step {
		'version' {
			println('libvcc ${opts.version}')
			exit(0)
		}
		'check' {
			outcome := run_check(opts)
			exit(summary('check', outcome))
		}
		'clean' {
			exit(clean())
		}
		'list' {
			usage()
			exit(0)
		}
		else {}
	}

	compiler := pick(opts) or {
		eprintln('build: no C compiler from the matrix is available')
		eprintln('       name one with --cc, or set VCC to a vcc binary')
		exit(2)
	}
	println('libvcc ${opts.version} (${compiler.version})')
	if opts.step == '' {
		if !run_lane(opts, compiler) {
			exit(1)
		}
		exit(0)
	}
	if opts.step == 'build' {
		build_library(opts, compiler) or { exit(1) }
		build_shared(opts, compiler) or { exit(1) }
		exit(0)
	}
	if !run_lane(opts, compiler) {
		exit(1)
	}
	exit(0)
}

fn usage() {
	println('libvcc build')
	println('')
	println('Usage: v run build.vsh [step] [options]')
	println('')
	println('Steps:')
	println('  build     compile src/ to target/<lane>/libvcc.a and libvcc.so.0.1')
	println('  test      build, then run the tests against the archive and the .so')
	println('  check     build and test every available compiler, both clz paths')
	println('  clean     remove target/')
	println('  list      this text')
	println('')
	println('With no step, build then test with one compiler.')
	println('')
	println('Options:')
	println('  --cc NAME       vcc, gcc or clang (default: vcc, gcc, clang)')
	println('  --require NAME  with check, a compiler whose absence is a failure')
	println('  --portable      use the portable clz path (-DLIBVCC_PORTABLE=1)')
	println('  --debug         -O0 -g on every compiler')
	println('  --release       -O2 on every compiler (default)')
	println('  --version       print the version and exit')
	println('  -v, --verbose   print each command before it runs')
}

fn parse(raw []string) ?Options {
	mut opts := Options{}
	for i := 0; i < raw.len; i++ {
		a := raw[i]
		if a == '-h' || a == '--help' || a == '--list' || a == '-l' {
			opts.step = 'list'
		} else if a == '--version' {
			opts.step = 'version'
		} else if a == '--debug' {
			opts.release = false
		} else if a == '--release' {
			opts.release = true
		} else if a == '--portable' {
			opts.portable = true
		} else if a == '-v' || a == '--verbose' {
			opts.verbose = true
		} else if a == '--cc' || a == '-c' {
			i++
			if i >= raw.len {
				eprintln('build: ${a} needs a name')
				return none
			}
			opts.cc = check_name(raw[i])?
		} else if a.starts_with('--cc=') {
			opts.cc = check_name(a.all_after('--cc='))?
		} else if a == '--require' {
			i++
			if i >= raw.len {
				eprintln('build: --require needs a name')
				return none
			}
			for name in raw[i].split(',') {
				if name != '' {
					opts.require << check_name(name)?
				}
			}
		} else if a in steps {
			if opts.step != '' && opts.step != 'list' {
				eprintln('build: one step at a time')
				return none
			}
			opts.step = a
		} else {
			eprintln('build: unknown argument ${a}')
			return none
		}
	}
	return opts
}

fn check_name(name string) ?string {
	if name !in matrix {
		eprintln('build: ${name} is not one of: ${matrix.join(', ')}')
		return none
	}
	return name
}

fn pick(opts Options) ?Compiler {
	if opts.cc != '' {
		return resolve(opts.cc)
	}
	for name in matrix {
		if compiler := resolve(name) {
			return compiler
		}
	}
	return none
}

fn resolve(name string) ?Compiler {
	path := locate(name) or { return none }
	return Compiler{
		name:    name
		path:    path
		version: version_of(path)
	}
}

fn locate(name string) ?string {
	mut candidates := []string{}
	if name == 'vcc' {
		env := os.getenv('VCC')
		if env != '' {
			candidates << env
		}
		candidates << 'vcc'
		candidates << os.join_path('..', 'vcc', 'vcc')
	} else {
		candidates << name
	}
	for candidate in candidates {
		if candidate.contains('/') {
			if os.exists(candidate) {
				return candidate
			}
		} else if found := on_path(candidate) {
			return found
		}
	}
	return none
}

fn on_path(name string) ?string {
	for dir in os.getenv('PATH').split(':') {
		if dir == '' {
			continue
		}
		path := os.join_path(dir, name)
		if os.exists(path) {
			return path
		}
	}
	return none
}

fn version_of(path string) string {
	result := os.exec([path, '--version'])
	for line in result.output.split('\n') {
		text := line.trim_space()
		if text != '' {
			return text
		}
	}
	return path
}

fn read_version() string {
	text := os.read_file('VERSION') or {
		eprintln('build: VERSION is missing')
		exit(2)
	}
	return text.trim_space()
}

fn run_lane(opts Options, compiler Compiler) bool {
	lane := lane_dir(opts, compiler)
	archive := build_library(opts, compiler) or { return false }
	build_shared(opts, compiler) or { return false }
	test_binary := build_tests(opts, compiler, archive) or { return false }
	if !run(opts, [test_binary]) {
		return false
	}
	shared_test := build_shared_tests(opts, compiler, lane) or { return false }
	if !run_shared(opts, shared_test, lane) {
		return false
	}
	println('${compiler.name}: ok')
	return true
}

fn build_library(opts Options, compiler Compiler) ?string {
	lane := lane_dir(opts, compiler)
	objects_dir := os.join_path(lane, 'obj')
	os.mkdir_all(objects_dir) or {
		|e| eprintln('build: cannot create ${objects_dir}: ${e}')
		return none
	}
	mut objects := []string{}
	for source in sources('src') {
		object := os.join_path(objects_dir, os.file_name(source).replace('.c', '.o'))
		mut command := [compiler.path]
		command << flags(opts, compiler)
		command << ['-c', source, '-o', object]
		if !run(opts, command) {
			eprintln('build: ${source} did not compile')
			return none
		}
		objects << object
	}
	archive := os.join_path(lane, archive_name)
	mut ar_command := ['ar', 'rcs', archive]
	ar_command << objects
	if !run(opts, ar_command) {
		eprintln('build: ar did not write ${archive}')
		return none
	}
	println('  ${archive} (${objects.len} objects)')
	return archive
}

fn build_tests(opts Options, compiler Compiler, archive string) ?string {
	lane := lane_dir(opts, compiler)
	binary := os.join_path(lane, 'test')
	mut command := [compiler.path]
	command << flags(opts, compiler)
	command << '-Itest'
	command << sources('test')
	command << archive
	command << ['-o', binary]
	if !run(opts, command) {
		eprintln('build: the tests did not link')
		return none
	}
	return binary
}

// build_shared writes target/<lane>/libvcc.so.<version> from a second set of
// objects compiled with -fpic, and the libvcc.so.<major> and libvcc.so
// symlinks beside it. The archive is compiled without -fpic on purpose: a
// static link should not pay the position-independence a shared object needs,
// which is the split libgcc makes between libgcc.a and libgcc_s.so.
//
// Nothing is imported: -nostdlib keeps the C library out, and the helpers name
// no symbol the objects beside them do not define.
fn build_shared(opts Options, compiler Compiler) ?string {
	lane := lane_dir(opts, compiler)
	objects_dir := os.join_path(lane, 'pic')
	os.mkdir_all(objects_dir) or {
		|e| eprintln('build: cannot create ${objects_dir}: ${e}')
		return none
	}
	mut objects := []string{}
	for source in sources('src') {
		object := os.join_path(objects_dir, os.file_name(source).replace('.c', '.o'))
		mut command := [compiler.path]
		command << flags(opts, compiler)
		command << ['-fpic', '-c', source, '-o', object]
		if !run(opts, command) {
			eprintln('build: ${source} did not compile as a shared object')
			return none
		}
		objects << object
	}
	name := '${shared_base}.${opts.version}'
	major := '${shared_base}.${opts.version.split('.').first()}'
	target := os.join_path(lane, name)
	mut command := [compiler.path, '-shared', '-nostdlib']
	if records_soname(opts, compiler, major) {
		command << '-Wl,-soname,${major}'
	} else {
		println('  ${compiler.name} does not record -Wl,-soname,${major}; ${name} links without a soname')
	}
	command << objects
	command << ['-o', target]
	if !run(opts, command) {
		eprintln('build: the shared object did not link')
		return none
	}
	link_shared_names(lane, name, major) or { return none }
	println('  ${target} (${objects.len} objects)')
	return target
}

// records_soname asks the compiler instead of assuming: it links a throwaway
// object into a shared object with the soname flag and looks for the soname in
// the bytes that came out. A compiler that refuses the flag fails the link; vcc
// 0.0.4-beta accepts it and writes no dynamic entry, which the byte search is
// what catches. The probe's source names nothing the soname contains, so a hit
// is the recorded entry and not a stray string.
fn records_soname(opts Options, compiler Compiler, major string) bool {
	probe_dir := os.join_path(os.temp_dir(), 'libvcc-soname-${compiler.name}')
	os.mkdir_all(probe_dir) or { return false }
	source := os.join_path(probe_dir, 'probe.c')
	os.write_file(source, '/* a name the probe does not otherwise hold */\nint libvcc_soname_probe;\n') or {
		return false
	}
	out := os.join_path(probe_dir, 'probe.so')
	command := [compiler.path, '-shared', '-nostdlib', '-Wl,-soname,${major}', source, '-o', out]
	if opts.verbose {
		println('+ ${command.join(' ')}')
	}
	if os.system_args(command) != 0 {
		return false
	}
	bytes := os.read_file(out) or { return false }
	return bytes.contains(major)
}

// link_shared_names lays the two names a linker and a loader ask for over the
// versioned file: libvcc.so.<major> is the soname a program records, and
// libvcc.so is what -lvcc is looked up as.
fn link_shared_names(lane string, name string, major string) ?bool {
	for pair in [[name, major], [major, shared_base]] {
		link_path := os.join_path(lane, pair[1])
		if os.is_link(link_path) || os.exists(link_path) {
			os.rm(link_path) or {}
		}
		os.symlink(pair[0], link_path) or {
			|e| eprintln('build: cannot link ${link_path}: ${e}')
			return none
		}
	}
	return true
}

fn build_shared_tests(opts Options, compiler Compiler, lane string) ?string {
	binary := os.join_path(lane, 'test-shared')
	mut command := [compiler.path]
	command << flags(opts, compiler)
	command << '-Itest'
	command << sources('test')
	command << ['-L${lane}', '-lvcc']
	command << ['-o', binary]
	if !run(opts, command) {
		eprintln('build: the tests did not link against the shared object')
		return none
	}
	return binary
}

// run_shared runs the shared-object test binary with LD_LIBRARY_PATH pointed at
// the lane, because the loader is told the soname and not where the file is.
fn run_shared(opts Options, binary string, lane string) bool {
	library_dir := os.abs_path(lane)
	previous := os.getenv('LD_LIBRARY_PATH')
	os.setenv('LD_LIBRARY_PATH', library_dir, true)
	defer {
		if previous == '' {
			os.unsetenv('LD_LIBRARY_PATH')
		} else {
			os.setenv('LD_LIBRARY_PATH', previous, true)
		}
	}
	if !run(opts, [binary]) {
		return false
	}
	return true
}

fn flags(opts Options, compiler Compiler) []string {
	mut out := ['-std=gnu89', '-Iinclude']
	if opts.release {
		out << '-O2'
	} else {
		out << ['-O0', '-g']
	}
	out << ['-Wall', '-Wextra', '-Werror']
	if compiler.name == 'gcc' || compiler.name == 'clang' {
		/* A runtime library must not need libc, and both compilers put a
		   __stack_chk_fail reference in an object that has an address-taken
		   local when their distribution defaults turn the protector on. */
		out << ['-ffreestanding', '-fno-stack-protector']
	}
	if opts.portable {
		out << '-DLIBVCC_PORTABLE=1'
	}
	return out
}

fn lane_dir(opts Options, compiler Compiler) string {
	mut label := compiler.name
	if opts.portable {
		label += '-portable'
	}
	if !opts.release {
		label += '-debug'
	}
	return os.join_path('target', label)
}

fn sources(dir string) []string {
	mut found := []string{}
	entries := os.ls(dir) or { return found }
	for entry in entries {
		if entry.ends_with('.c') {
			found << os.join_path(dir, entry)
		}
	}
	found.sort()
	return found
}

fn run(opts Options, command []string) bool {
	if opts.verbose {
		println('+ ${command.join(' ')}')
	}
	code := os.system_args(command)
	if code != 0 {
		eprintln('command exited ${code}: ${command.join(' ')}')
		return false
	}
	return true
}

fn run_check(opts Options) Outcome {
	mut outcome := Outcome{}
	println('libvcc ${opts.version}')
	mut missing := []string{}
	for name in matrix {
		compiler := resolve(name) or {
			missing << name
			continue
		}
		for portable in [false, true] {
			mut lane := opts
			lane.portable = portable
			outcome.total++
			label := name + if portable { ' portable' } else { '' }
			println('== ${label}: ${compiler.version}')
			if !run_lane(lane, compiler) {
				outcome.failed++
			}
		}
	}
	if missing.len > 0 {
		println('missing: ${missing.join(', ')}')
	}
	for name in opts.require {
		if name in missing {
			eprintln('required compiler is missing: ${name}')
			outcome.failed++
		}
	}
	return outcome
}

fn summary(what string, outcome Outcome) int {
	mut text := '${what}: ${outcome.total - outcome.failed} of ${outcome.total} lanes passed'
	if outcome.failed > 0 {
		text += '; ${outcome.failed} failed'
	}
	println(text)
	if outcome.failed > 0 {
		return 1
	}
	return 0
}

fn clean() int {
	if os.exists('target') {
		os.rmdir_all('target') or {
			|e| eprintln('build: cannot remove target/: ${e}')
			return 1
		}
	}
	println('build: removed')
	return 0
}
