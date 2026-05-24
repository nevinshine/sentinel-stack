use ed25519_dalek::{Signer, SigningKey};
use rand::rngs::OsRng;
use std::fs;
use std::process::Command;

pub fn get_or_generate_key() -> SigningKey {
    let key_path = "/home/nevin/.telos/pcc.key";
    let pub_path = "/home/nevin/.telos/pcc.pub";

    if let Ok(key_bytes) = fs::read(key_path) {
        if key_bytes.len() == 32 {
            let mut bytes = [0u8; 32];
            bytes.copy_from_slice(&key_bytes);
            return SigningKey::from_bytes(&bytes);
        }
    }

    // Generate new key
    let mut csprng = OsRng;
    let signing_key = SigningKey::generate(&mut csprng);

    // Save private key (32 bytes)
    fs::write(key_path, signing_key.to_bytes()).expect("Failed to save private key");

    // Save public key (32 bytes)
    fs::write(pub_path, signing_key.verifying_key().to_bytes()).expect("Failed to save public key");

    println!("[PASS] Generated new static Ed25519 keypair in /home/nevin/.telos/");
    signing_key
}

pub fn sign_and_inject_elf(binary_path: &str, output_path: &str) {
    println!("\nInitiating Cryptographic Binary Injection...");

    // 1. Get or Generate Ed25519 Keypair
    let signing_key = get_or_generate_key();

    // 2. Read the raw ELF binary (emitted by LLVM or GCC)
    let elf_bytes = fs::read(binary_path).expect("Failed to read raw ELF binary");

    // 3. Sign the binary bytes
    let signature = signing_key.sign(&elf_bytes);
    let sig_bytes = signature.to_bytes();

    // 4. Write signature to a temporary file for objcopy
    let tmp_sig_path = "telos_pcc.sig";
    fs::write(tmp_sig_path, sig_bytes).expect("Failed to write signature temp file");

    println!("[PASS] Ed25519 Signature generated successfully.");

    // 5. Inject the signature into the custom .telos_pcc ELF section
    let status = Command::new("objcopy")
        .arg("--add-section")
        .arg(".telos_pcc=telos_pcc.sig")
        .arg("--set-section-flags")
        .arg(".telos_pcc=readonly,data")
        .arg(binary_path)
        .arg(output_path)
        .status()
        .expect("Failed to execute objcopy. Is binutils installed?");

    if status.success() {
        println!("[PASS] Policy-Carrying Code injected into ELF section '.telos_pcc'");
        println!("       Signed binary ready for deployment: {}", output_path);
    } else {
        eprintln!("[FAIL] ELF section injection failed.");
        std::process::exit(1);
    }

    // Cleanup temp file
    let _ = fs::remove_file(tmp_sig_path);
}
