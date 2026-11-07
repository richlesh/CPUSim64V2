//! Build script: propagate the project version into the compiled binary.
//!
//! The Maven wrapper (`pom.xml`) passes the project version via the
//! `CPUSIM64_VERSION` environment variable when it runs `cargo build`. We read
//! it here and re-export it as `CPUSIM64_BUILD_VERSION` so `main.rs` can pick it
//! up with `env!(...)` at compile time.
//!
//! `rerun-if-env-changed` ensures Cargo rebuilds when the injected version
//! changes (Cargo does not otherwise track external env vars used via
//! `option_env!`/`env!`). When the variable is absent (a plain `cargo build`
//! outside Maven), we fall back to the crate's own `Cargo.toml` version.

fn main() {
    println!("cargo:rerun-if-env-changed=CPUSIM64_VERSION");

    let version = std::env::var("CPUSIM64_VERSION")
        .ok()
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| env!("CARGO_PKG_VERSION").to_string());

    println!("cargo:rustc-env=CPUSIM64_BUILD_VERSION={version}");
}
