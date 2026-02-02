# ctidh512

Rust FFI for **dCTIDH 512-bit** (CTIDH key exchange). Easy to use from any Rust project.

## Requirements

- Rust toolchain
- C build (make, ar, clang or gcc) – used at compile time to build the C library from `scripts/new_primes/genSteps`
- Run **once** from repo root: `cd scripts/new_primes/genSteps && python3 autogen` (generates Makefile and 512 sources)

## Usage in your project

### As a path dependency (inside dCTIDH repo)

```toml
[dependencies]
ctidh512 = { path = "path/to/dCTIDH/rust/ctidh512" }
```

### Example

```rust
use ctidh512::{keygen, derive, PublicKey, SecretKey, SharedSecret};

fn main() {
    let (pk_alice, sk_alice) = keygen();
    let (pk_bob, sk_bob) = keygen();

    let ss_alice = derive(&pk_bob, &sk_alice).expect("derive alice");
    let ss_bob = derive(&pk_alice, &sk_bob).expect("derive bob");
    assert_eq!(ss_alice, ss_bob);

    println!("Key exchange OK, shared secret = {} bytes", ss_alice.len());
}
```

## API

- **Constants:** `SK_SIZE` (74), `PK_SIZE` (64), `SS_SIZE` (64)
- **Types:** `SecretKey`, `PublicKey`, `SharedSecret` (fixed-size byte arrays)
- **keygen()** → `(PublicKey, SecretKey)`
- **derive(pk, sk)** → `Result<SharedSecret, ()>` (errors if `pk` is invalid)
- **keygen_raw**, **derive_raw** – same, but write into provided buffers

## Build

The crate’s `build.rs` runs `make -C scripts/new_primes/genSteps ctidh-512.main`, then builds a single static library and links it. Ensure `autogen` has been run in `genSteps` before the first `cargo build`.

## License

Same as dCTIDH (CC0-1.0).
