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
    let gensteps = repo_root.join("scripts/new_primes/genSteps");

    if !gensteps.join("Makefile").exists() {
        panic!(
            "genSteps not found at {}. Run 'python3 autogen' in scripts/new_primes/genSteps first.",
            gensteps.display()
        );
    }

    // Build the 2048 library and ctidh_api2048.o (building ctidh-2048.main pulls all deps)
    let status = Command::new("make")
        .arg("-C")
        .arg(&gensteps)
        .arg("ctidh-2048.main")
        .status()
        .expect("failed to run make");
    if !status.success() {
        panic!("make ctidh-2048.main failed");
    }

    let out_dir = env::var("OUT_DIR").expect("OUT_DIR");
    let ffi_dir = Path::new(&out_dir).join("ctidh2048_ffi");
    fs::create_dir_all(&ffi_dir).expect("create ffi dir");

    // Extract object files from each static lib into ffi_dir
    for lib in [
        "libhighctidh_2048.a",
        "libhighctidh_base.a",
        "libhighctidh_untuned.a",
        "libtest.a",
    ] {
        let lib_path = gensteps.join(lib);
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

    // Copy ctidh_api2048.o (provides secsidh_keygen, secsidh_derive)
    let api_o = gensteps.join("ctidh_api2048.o");
    if !api_o.exists() {
        panic!("missing ctidh_api2048.o - run make ctidh-2048.main in genSteps");
    }
    fs::copy(&api_o, ffi_dir.join("ctidh_api2048.o")).expect("copy ctidh_api2048.o");

    // Collect all .o files and create a single static library for Rust
    let mut o_files: Vec<_> = fs::read_dir(&ffi_dir)
        .expect("read_dir")
        .filter_map(|e| e.ok())
        .filter(|e| e.path().extension().map_or(false, |ext| ext == "o"))
        .map(|e| e.path().file_name().unwrap().to_owned())
        .collect();
    o_files.sort();

    let mut ar_cmd = Command::new("ar");
    ar_cmd.arg("cr").arg(ffi_dir.join("libctidh2048ffi.a"));
    for o in &o_files {
        ar_cmd.arg(ffi_dir.join(o));
    }
    let status = ar_cmd.status().expect("ar cr failed");
    if !status.success() {
        panic!("ar cr libctidh2048ffi.a failed");
    }

    println!("cargo:rustc-link-search=native={}", ffi_dir.display());
    println!("cargo:rustc-link-lib=static=ctidh2048ffi");
    // C library (for memcpy, memset, etc. used by the C code)
    println!("cargo:rustc-link-lib=dylib=c");
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed={}/ctidh_api.c", gensteps.display());
    println!("cargo:rerun-if-changed={}/ctidh_api2048.o", gensteps.display());
}
