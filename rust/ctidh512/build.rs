use std::env;
use std::fs;
use std::path::Path;
use std::path::PathBuf;
use std::process::Command;

/// How the C library is built.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Backend {
    /// The Makefile, with the x86-64 assembly for the field arithmetic. The
    /// assembly uses ELF section directives, unprefixed symbol names and the
    /// System V calling convention, so it only builds for x86-64 ELF systems
    /// such as Linux.
    MakeAsm,
    /// The Makefile, with the portable C field arithmetic (`ARCH=arm64`).
    MakePortable,
    /// The portable C sources, compiled with the `cc` crate. For Windows,
    /// which has neither make nor ar.
    Cc,
}

/// Picks the backend for the target. `CTIDH512_BUILD` (`make-asm`,
/// `make-portable` or `cc`) overrides the choice, which lets the portable
/// backends be checked against the assembly on x86-64 Linux.
fn select_backend(target_os: &str, target_arch: &str) -> Backend {
    println!("cargo:rerun-if-env-changed=CTIDH512_BUILD");
    if let Ok(forced) = env::var("CTIDH512_BUILD") {
        return match forced.as_str() {
            "make-asm" => Backend::MakeAsm,
            "make-portable" => Backend::MakePortable,
            "cc" => Backend::Cc,
            other => panic!("CTIDH512_BUILD={other}: expected make-asm, make-portable or cc"),
        };
    }
    match (target_os, target_arch) {
        ("windows", _) => Backend::Cc,
        // Apple's assembler rejects the ELF-only `.section .rodata`.
        ("macos", _) => Backend::MakePortable,
        (_, "aarch64") => Backend::MakePortable,
        _ => Backend::MakeAsm,
    }
}

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR");
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap_or_default();
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();

    let backend = select_backend(&target_os, &target_arch);
    if backend == Backend::Cc {
        // Not canonicalized: on Windows that yields a `\\?\` path, which
        // C compilers do not reliably accept as an include directory.
        let gensteps_src = Path::new(&manifest_dir).join("../../scripts/new_primes/genSteps");
        build_with_cc(&gensteps_src, &target_os, &target_env);
        return;
    }

    let repo_root = Path::new(&manifest_dir)
        .join("../..")
        .canonicalize()
        .expect("repo root");
    let gensteps_src = repo_root.join("scripts/new_primes/genSteps");

    if !gensteps_src.join("Makefile").exists() {
        panic!(
            "genSteps not found at {}. Run 'python3 autogen' in scripts/new_primes/genSteps first.",
            gensteps_src.display()
        );
    }

    let out_dir = env::var("OUT_DIR").expect("OUT_DIR");

    let build_dir = Path::new(&out_dir).join("genSteps_build");
    fs::create_dir_all(&build_dir).expect("create build_dir");

    const SOURCE_EXTS: &[&str] = &["c", "h", "S", "py", "exp", "in", "txt"];
    for entry in fs::read_dir(&gensteps_src).expect("read gensteps_src") {
        let entry = entry.expect("dir entry");
        let path = entry.path();
        if !path.is_file() {
            continue;
        }
        let fname = path.file_name().unwrap().to_str().unwrap_or("");
        let ext = path.extension().and_then(|e| e.to_str()).unwrap_or("");
        if fname == "Makefile" || SOURCE_EXTS.contains(&ext) {
            let dest = build_dir.join(fname);
            fs::copy(&path, &dest).expect("copy source file");
        }
    }

    let mut make_cmd = Command::new("make");
    make_cmd.arg("-C").arg(&build_dir);

    match target_os.as_str() {
        "ios" => {
            make_cmd.arg("ARCH=arm64");
            let sysroot = ios_sysroot();
            let min_ios = env::var("IPHONEOS_DEPLOYMENT_TARGET")
                .unwrap_or_else(|_| "14.0".to_string());
            let triple = format!("arm64-apple-ios{min_ios}");
            let ios_cc = format!(
                "clang -target {triple} -isysroot {sysroot} \
                 -O2 -std=gnu99 -pedantic -Wall -Wextra \
                 -Wno-language-extension-token -fwrapv -DTIMECOP -DGETRANDOM"
            );
            let ios_scc = format!("clang -target {triple} -isysroot {sysroot}");
            make_cmd.arg(format!("CC={ios_cc}"));
            make_cmd.arg(format!("SCC={ios_scc}"));
        }
        "android" => {
           
            make_cmd.arg("ARCH=arm64");
            let ndk_cc = android_ndk_cc(&target_arch);
            let android_cc = format!(
                "{ndk_cc} -O2 -std=gnu99 -pedantic -Wall -Wextra \
                 -Wno-language-extension-token -fwrapv -DTIMECOP"
            );
            make_cmd.arg(format!("CC={android_cc}"));
            make_cmd.arg(format!("SCC={ndk_cc}"));
        }
        _ => {
            if backend == Backend::MakePortable {
                make_cmd.arg("ARCH=arm64");
            }
        }
    }

    make_cmd.args([
        "ctidh_api512.o",
        "libhighctidh_512.a",
        "libhighctidh_base.a",
        "libhighctidh_untuned.a",
        "libtest.a",
    ]);

    let status = make_cmd.status().expect("failed to run make");
    if !status.success() {
        panic!("make (ctidh512 libs) failed");
    }

    let ffi_dir = Path::new(&out_dir).join("ctidh512_ffi");
    fs::create_dir_all(&ffi_dir).expect("create ffi dir");

    // Extract object files from each static lib into ffi_dir.
    for lib in [
        "libhighctidh_512.a",
        "libhighctidh_base.a",
        "libhighctidh_untuned.a",
        "libtest.a",
    ] {
        let lib_path = build_dir.join(lib);
        if !lib_path.exists() {
            panic!("missing {}", lib);
        }
        let status = Command::new("ar")
            .args(["x", lib_path.to_str().unwrap()])
            .current_dir(&ffi_dir)
            .status()
            .expect("ar x failed");
        if !status.success() {
            panic!("ar x {} failed", lib);
        }
    }

    // Copy ctidh_api512.o (provides secsidh_keygen, secsidh_derive, secsidh_sk_to_pk).
    let api_o = build_dir.join("ctidh_api512.o");
    if !api_o.exists() {
        panic!("missing ctidh_api512.o");
    }
    fs::copy(&api_o, ffi_dir.join("ctidh_api512.o")).expect("copy ctidh_api512.o");

    // Collect all .o files and create a single static library for Rust to link.
    let mut o_files: Vec<_> = fs::read_dir(&ffi_dir)
        .expect("read_dir")
        .filter_map(|e| e.ok())
        .filter(|e| e.path().extension().map_or(false, |ext| ext == "o"))
        .map(|e| e.path().file_name().unwrap().to_owned())
        .collect();
    o_files.sort();

    let mut ar_cmd = Command::new("ar");
    ar_cmd.arg("cr").arg(ffi_dir.join("libctidh512ffi.a"));
    for o in &o_files {
        ar_cmd.arg(ffi_dir.join(o));
    }
    let status = ar_cmd.status().expect("ar cr failed");
    if !status.success() {
        panic!("ar cr libctidh512ffi.a failed");
    }

    println!("cargo:rustc-link-search=native={}", ffi_dir.display());
    println!("cargo:rustc-link-lib=static=ctidh512ffi");
    println!("cargo:rustc-link-lib=dylib=c");
    println!("cargo:rerun-if-changed=build.rs");
    println!(
        "cargo:rerun-if-changed={}/ctidh_api.c",
        gensteps_src.display()
    );
}

/// Builds the library from the portable C sources with the `cc` crate,
/// compiling the same objects as the Makefile does with `ARCH=arm64`.
fn build_with_cc(src: &Path, target_os: &str, target_env: &str) {
    // Compiled with the 512-bit namespace: libhighctidh_512.a and ctidh_api512.o.
    const SIZED: &[&str] = &[
        "uintbig512_c.c",
        "fp512_c.c",
        "fp_inv512.c",
        "fp_sqrt512.c",
        "primes512.c",
        "poly.c",
        "mont.c",
        "elligator.c",
        "skgen.c",
        "validate.c",
        "csidh.c",
        "ctidh_api.c",
    ];
    // Compiled with the generic namespace only: libhighctidh_base.a,
    // libhighctidh_untuned.a and libtest.a.
    const GENERIC: &[&str] = &[
        "steps.c",
        "random.c",
        "steps_untuned.c",
        "randombytes.c",
        "int32_sort_portable.c",
    ];

    let new_build = || {
        let mut build = cc::Build::new();
        build
            .include(src)
            .define("NAMESPACEGENERIC(x)", Some("highctidh_##x"))
            // As in the Makefile. randombytes.c ignores it on Windows.
            .define("GETRANDOM", None)
            // The Makefile compiles with -O0 on every other target.
            .opt_level(0)
            .debug(false)
            .warnings(false);
        if target_env == "msvc" {
            // cl.exe has no 128-bit integer type, which the portable field
            // arithmetic needs. clang-cl has one and uses the same CRT.
            build.compiler(clang_cl()).flag("/clang:-fwrapv");
        } else {
            build.flag("-std=gnu99").flag("-fwrapv");
        }
        build
    };

    let mut sized = new_build();
    sized
        .define("BITS", Some("512"))
        .define("NAMESPACEBITS(x)", Some("highctidh_512_##x"));
    for file in SIZED {
        sized.file(src.join(file));
    }
    let mut generic = new_build();
    for file in GENERIC {
        generic.file(src.join(file));
    }

    // One archive, named as `#[link(name = "ctidh512ffi")]` in lib.rs expects.
    new_build()
        .objects(sized.compile_intermediates())
        .objects(generic.compile_intermediates())
        .compile("ctidh512ffi");

    if target_os == "windows" {
        // randombytes.c draws from BCryptGenRandom.
        println!("cargo:rustc-link-lib=bcrypt");
    }
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed={}", src.display());
}

/// Locates clang-cl: `CTIDH512_CLANG_CL`, then `PATH`, then LLVM's default
/// install directory, which GitHub's Windows runners provide.
fn clang_cl() -> PathBuf {
    println!("cargo:rerun-if-env-changed=CTIDH512_CLANG_CL");
    if let Some(path) = env::var_os("CTIDH512_CLANG_CL") {
        return PathBuf::from(path);
    }
    let on_path = env::var_os("PATH")
        .map(|paths| env::split_paths(&paths).collect::<Vec<_>>())
        .unwrap_or_default()
        .into_iter()
        .map(|dir| dir.join("clang-cl.exe"))
        .find(|candidate| candidate.is_file());
    if let Some(path) = on_path {
        return path;
    }
    let default = PathBuf::from(r"C:\Program Files\LLVM\bin\clang-cl.exe");
    if default.is_file() {
        return default;
    }
    panic!(
        "clang-cl not found. Install LLVM, or set CTIDH512_CLANG_CL to clang-cl.exe. \
         MSVC's cl.exe cannot build this library: it lacks a 128-bit integer type."
    );
}

/// Return the path to the iPhoneOS SDK sysroot via xcrun.
fn ios_sysroot() -> String {
    let out = Command::new("xcrun")
        .args(["--sdk", "iphoneos", "--show-sdk-path"])
        .output()
        .expect("xcrun failed – is Xcode installed?");
    if !out.status.success() {
        panic!(
            "xcrun --sdk iphoneos --show-sdk-path failed:\n{}",
            String::from_utf8_lossy(&out.stderr)
        );
    }
    String::from_utf8(out.stdout)
        .expect("xcrun output not UTF-8")
        .trim()
        .to_string()
}

fn android_ndk_cc(target_arch: &str) -> String {
    let clang_name = match target_arch {
        "aarch64" => "aarch64-linux-android24-clang",
        "x86_64" => "x86_64-linux-android24-clang",
        other => panic!("unsupported Android arch: {other}"),
    };

    let ndk = env::var("ANDROID_NDK_HOME")
        .or_else(|_| env::var("NDK_HOME"))
        .or_else(|_| env::var("ANDROID_NDK_ROOT"))
        .unwrap_or_default();

    if !ndk.is_empty() {
        for host in ["darwin-x86_64", "darwin-arm64", "linux-x86_64"] {
            let cc = Path::new(&ndk)
                .join("toolchains/llvm/prebuilt")
                .join(host)
                .join("bin")
                .join(clang_name);
            if cc.exists() {
                return cc.to_str().unwrap().to_string();
            }
        }
    }

    clang_name.to_string()
}
