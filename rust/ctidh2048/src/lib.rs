//! # ctidh2048 – Rust FFI for dCTIDH 2048-bit
//!
//! CTIDH (Commutative Supersingular Isogeny Diffie–Hellman) key exchange, 2048-bit parameter set.
//!
//! ## Usage
//!
//! ```ignore
//! use ctidh2048::{keygen, derive, PublicKey, SecretKey, SharedSecret};
//!
//! let (pk_a, sk_a) = keygen();
//! let (pk_b, sk_b) = keygen();
//!
//! let ss_a = derive(&pk_b, &sk_a).expect("derive a");
//! let ss_b = derive(&pk_a, &sk_b).expect("derive b");
//! assert_eq!(ss_a, ss_b);
//! ```

#![no_std]

use core::result::Result;

/// Secret key size in bytes (2048-bit parameter set).
pub const SK_SIZE: usize = 226;
/// Public key size in bytes (2048-bit parameter set).
pub const PK_SIZE: usize = 256;
/// Shared secret size in bytes (2048-bit parameter set).
pub const SS_SIZE: usize = 256;

/// Secret key (226 bytes).
pub type SecretKey = [u8; SK_SIZE];
/// Public key (256 bytes).
pub type PublicKey = [u8; PK_SIZE];
/// Shared secret (256 bytes).
pub type SharedSecret = [u8; SS_SIZE];

/// Success return from C API.
const SECSIDH_SUCCESS: i32 = 0;

#[link(name = "ctidh2048ffi")]
extern "C" {
    fn secsidh_keygen(pk: *mut u8, sk: *mut u8) -> i32;
    fn secsidh_sk_to_pk(pk: *mut u8, sk: *const u8) -> i32;
    fn secsidh_derive(ss: *mut u8, pk: *const u8, sk: *const u8) -> i32;
}

/// Raw FFI: generate key pair into buffers.
///
/// Returns `Ok(())` on success, `Err(())` on failure.
#[inline]
pub fn keygen_raw(pk: &mut PublicKey, sk: &mut SecretKey) -> Result<(), ()> {
    let ret = unsafe { secsidh_keygen(pk.as_mut_ptr(), sk.as_mut_ptr()) };
    if ret == SECSIDH_SUCCESS {
        Ok(())
    } else {
        Err(())
    }
}

/// Raw FFI: compute public key from secret key (sk -> pk).
///
/// Returns `Ok(())` on success.
#[inline]
pub fn sk_to_pk_raw(pk: &mut PublicKey, sk: &SecretKey) -> Result<(), ()> {
    let ret = unsafe { secsidh_sk_to_pk(pk.as_mut_ptr(), sk.as_ptr()) };
    if ret == SECSIDH_SUCCESS {
        Ok(())
    } else {
        Err(())
    }
}

/// Compute public key from secret key.
///
/// # Example
///
/// ```
/// use ctidh2048::{keygen, sk_to_pk, PublicKey, SecretKey};
/// let (pk, sk) = keygen();
/// let pk2 = sk_to_pk(&sk).expect("sk_to_pk");
/// assert_eq!(pk, pk2);
/// ```
#[inline]
pub fn sk_to_pk(sk: &SecretKey) -> Result<PublicKey, ()> {
    let mut pk = [0u8; PK_SIZE];
    sk_to_pk_raw(&mut pk, sk)?;
    Ok(pk)
}

/// Raw FFI: derive shared secret.
///
/// Returns `Ok(())` on success, `Err(())` if `pk` is invalid (e.g. not supersingular).
#[inline]
pub fn derive_raw(ss: &mut SharedSecret, pk: &PublicKey, sk: &SecretKey) -> Result<(), ()> {
    let ret = unsafe { secsidh_derive(ss.as_mut_ptr(), pk.as_ptr(), sk.as_ptr()) };
    if ret == SECSIDH_SUCCESS {
        Ok(())
    } else {
        Err(())
    }
}

/// Generate a new key pair.
///
/// # Example
///
/// ```
/// use ctidh2048::{keygen, PublicKey, SecretKey};
/// let (pk, sk): (PublicKey, SecretKey) = keygen();
/// ```
#[inline]
pub fn keygen() -> (PublicKey, SecretKey) {
    let mut pk = [0u8; PK_SIZE];
    let mut sk = [0u8; SK_SIZE];
    keygen_raw(&mut pk, &mut sk).expect("keygen failed");
    (pk, sk)
}

/// Derive shared secret from peer's public key and own secret key.
///
/// Returns `Err(())` if `pk` is invalid (e.g. not a valid supersingular curve).
///
/// # Example
///
/// ```
/// use ctidh2048::{keygen, derive, PublicKey, SecretKey};
/// let (pk_a, sk_a) = keygen();
/// let (pk_b, sk_b) = keygen();
/// let ss_a = derive(&pk_b, &sk_a).unwrap();
/// let ss_b = derive(&pk_a, &sk_b).unwrap();
/// assert_eq!(ss_a, ss_b);
/// ```
#[inline]
pub fn derive(pk: &PublicKey, sk: &SecretKey) -> Result<SharedSecret, ()> {
    let mut ss = [0u8; SS_SIZE];
    derive_raw(&mut ss, pk, sk)?;
    Ok(ss)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_key_exchange() {
        let (pk_a, sk_a) = keygen();
        let (pk_b, sk_b) = keygen();
        let ss_a = derive(&pk_b, &sk_a).expect("derive a");
        let ss_b = derive(&pk_a, &sk_b).expect("derive b");
        assert_eq!(ss_a, ss_b);
    }

    #[test]
    fn test_sizes() {
        let (pk, sk) = keygen();
        assert_eq!(pk.len(), PK_SIZE);
        assert_eq!(sk.len(), SK_SIZE);
        let ss = derive(&pk, &sk).unwrap();
        assert_eq!(ss.len(), SS_SIZE);
    }

    #[test]
    fn test_sk_to_pk() {
        let (pk, sk) = keygen();
        let pk_from_sk = sk_to_pk(&sk).expect("sk_to_pk");
        assert_eq!(pk, pk_from_sk);
    }
}
