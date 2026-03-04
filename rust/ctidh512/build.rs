use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR");
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

    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap_or_default();
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
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
            if target_arch == "aarch64" {
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
