//! Validation harness: runs every `cpusim64/validation/*.asm` (except the
//! nondeterministic race test `T105_SYNC`, which is excluded by design) through
//! the Rust interpreter and compares output against the committed Java
//! `reference/*.out` files.
//!
//! Methodology mirrors `cpusim64/validation/runtests.py`:
//!   * The simulator is invoked the way `debug.sh` does: `--verbose --debug
//!     --mem=16M --stack=8K --mem=1M` (the last `--mem` wins = 1,000,000 words;
//!     `--stack=8K` = 8,000 words).
//!   * The compared region is the text strictly between the line
//!     `Read N words from <name>.o64` and the line `User CPU Cycles:`.
//!   * In addition (parity requirement), the `User CPU Cycles:` value is checked
//!     against the reference as a separate assertion.
//!
//! Object files: uses the committed `validation/obj/<name>.o64` (+ `.sym1`). The
//! Java assembler is not required at test time.

use std::path::PathBuf;
use std::process::Command;
use std::sync::{Mutex, MutexGuard, OnceLock};

/// Serializes tests that share the `instructions.data` file next to the test
/// binary (cargo runs integration tests in parallel by default).
fn override_lock() -> MutexGuard<'static, ()> {
    static L: OnceLock<Mutex<()>> = OnceLock::new();
    L.get_or_init(|| Mutex::new(()))
        .lock()
        .unwrap_or_else(|e| e.into_inner())
}

/// T105_SYNC is a deliberate data-race test whose output is nondeterministic
/// (even under Java); byte-comparing it against one captured run is unsound, so
/// it is excluded (see README "Known deviations").
const EXCLUDED: &[&str] = &["T105_SYNC"];

fn validation_dir() -> PathBuf {
    let mut p = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    p.pop();
    p.push("cpusim64");
    p.push("validation");
    p
}

fn bin_path() -> PathBuf {
    // CARGO_BIN_EXE_<name> is set by cargo for integration tests.
    PathBuf::from(env!("CARGO_BIN_EXE_cpusim64_native"))
}

/// Extract the region between the `Read ... <name>.o64` line and the
/// `User CPU Cycles` line (ports runtests.py `extract_relevant`).
fn extract_region(text: &str, name: &str) -> Vec<String> {
    let mut lines = Vec::new();
    let mut capturing = false;
    for line in text.lines() {
        if !capturing && line.starts_with("Read ") && line.contains(&format!("{name}.o64")) {
            capturing = true;
            continue;
        }
        if capturing {
            if line.starts_with("User CPU Cycles") {
                break;
            }
            lines.push(line.to_string());
        }
    }
    lines
}

fn cycles_line(text: &str) -> Option<String> {
    text.lines()
        .find(|l| l.starts_with("User CPU Cycles"))
        .map(|s| s.to_string())
}

struct Outcome {
    name: String,
    region_ok: bool,
    cycles_ok: bool,
    got_cycles: String,
    want_cycles: String,
}

/// Reference-era `instructions.data` overrides: the committed `reference/*.out`
/// files were generated with these two values (the built-in defaults now match
/// the committed `instructions.data`: load:1, fdivide:19). Installing this file
/// next to the executable exercises the runtime override path and reproduces the
/// reference cycle counts for everything except the RECIP cycle-key difference
/// (RECIP now charges `fdivide` to match Simulator.java, whereas the references
/// charged it as `divide` — not expressible in the data file).
const REFERENCE_OVERRIDE: &str = "load:0\nfdivide:9\n";

fn override_path() -> PathBuf {
    bin_path().parent().unwrap().join("instructions.data")
}

fn install_override() {
    std::fs::write(override_path(), REFERENCE_OVERRIDE).expect("write instructions.data override");
}

fn remove_override() {
    let _ = std::fs::remove_file(override_path());
}

fn run_all(with_override: bool) -> Vec<Outcome> {
    remove_override();
    if with_override {
        install_override();
    }
    let vdir = validation_dir();
    let bin = bin_path();
    let mut names: Vec<String> = std::fs::read_dir(&vdir)
        .expect("read validation dir")
        .filter_map(|e| e.ok())
        .filter_map(|e| {
            let p = e.path();
            if p.extension().map(|x| x == "asm").unwrap_or(false) {
                Some(p.file_stem().unwrap().to_string_lossy().to_string())
            } else {
                None
            }
        })
        .filter(|n| !EXCLUDED.contains(&n.as_str()))
        .collect();
    names.sort();

    let mut outcomes = Vec::new();
    for name in names {
        let o64 = vdir.join("obj").join(format!("{name}.o64"));
        let refp = vdir.join("reference").join(format!("{name}.out"));
        if !o64.is_file() || !refp.is_file() {
            continue;
        }
        let out = Command::new(&bin)
            .arg(&o64)
            .args(["--verbose", "--debug", "--mem=16M", "--stack=8K", "--mem=1M"])
            .output()
            .expect("run interpreter");
        let mut combined = String::from_utf8_lossy(&out.stdout).into_owned();
        combined.push_str(&String::from_utf8_lossy(&out.stderr));
        let ref_text = std::fs::read_to_string(&refp).unwrap();

        let got_region = extract_region(&combined, &name);
        let want_region = extract_region(&ref_text, &name);
        let region_ok = got_region == want_region;

        let got_cycles = cycles_line(&combined).unwrap_or_default();
        let want_cycles = cycles_line(&ref_text).unwrap_or_default();
        let cycles_ok = got_cycles == want_cycles;

        outcomes.push(Outcome {
            name,
            region_ok,
            cycles_ok,
            got_cycles,
            want_cycles,
        });
    }
    remove_override();
    outcomes
}

#[test]
fn validation_regions_match_reference() {
    let _g = override_lock();
    // Output is independent of the cycle table, so run with built-in defaults.
    let outcomes = run_all(false);
    assert!(!outcomes.is_empty(), "no validation cases found");
    let mut failures = Vec::new();
    for o in &outcomes {
        if !o.region_ok {
            failures.push(o.name.clone());
        }
    }
    if !failures.is_empty() {
        panic!(
            "Output region mismatches ({} of {}): {:?}",
            failures.len(),
            outcomes.len(),
            failures
        );
    }
    println!(
        "All {} validation regions match reference byte-for-byte (T105_SYNC excluded).",
        outcomes.len()
    );
}

/// Cycle-count parity against the committed `reference/*.out`.
///
/// The built-in cycle table now matches the committed `instructions.data`
/// (`load:1`, `fdivide:19`, `RECIP` charged as `fdivide` per `Simulator.java`).
/// The reference `.out` files, however, were generated with an older table
/// (`load:0`, `fdivide:9`, `RECIP` as `divide`). To reproduce them this test
/// installs a reference-era `instructions.data` next to the executable (which
/// also exercises the runtime override feature). The `load`/`fdivide`
/// differences are fully recovered by the file; the `RECIP`-as-`divide`
/// difference is NOT expressible in the data file, so programs that execute the
/// single-operand RECIP form keep a small, output-invisible cycle surplus.
///
/// All deterministic tests still match the reference OUTPUT byte-for-byte
/// regardless of the cycle table. This test asserts the cycle match rate does
/// not regress below the established baseline.
#[test]
fn validation_cycle_counts_match_reference() {
    let _g = override_lock();
    let outcomes = run_all(true);
    let mut failures = Vec::new();
    for o in &outcomes {
        if !o.cycles_ok {
            failures.push(format!(
                "{}: got '{}' want '{}'",
                o.name, o.got_cycles, o.want_cycles
            ));
        }
    }
    let total = outcomes.len();
    let passed = total - failures.len();
    println!("Cycle counts (with reference-era instructions.data override): {passed}/{total} match reference.");
    for f in &failures {
        println!("  (known cycle delta) {f}");
    }
    // Baseline: with the reference-era override, at least 26 of 35 match.
    // (T12_UNARY no longer matches because RECIP now charges `fdivide` to match
    // Simulator.java, which is not expressible in the override file.)
    assert!(
        passed >= 26,
        "cycle-count parity regressed: only {passed}/{total} match (expected >= 26)"
    );
}

/// Verifies the runtime `instructions.data` override feature: dropping a file
/// next to the executable changes the charged cycle counts.
#[test]
fn instructions_data_override_changes_cycles() {
    let _g = override_lock();
    let vdir = validation_dir();
    let bin = bin_path();
    let o64 = vdir.join("obj").join("T16_DIVIDE.o64");
    let args = ["--verbose", "--debug", "--mem=16M", "--stack=8K", "--mem=1M"];

    let cycles = |text: &str| -> String {
        cycles_line(text).unwrap_or_default()
    };

    // Built-in defaults (no file): fdivide:19 => higher cycle count.
    remove_override();
    let out_default = Command::new(&bin).arg(&o64).args(args).output().unwrap();
    let default_cycles = cycles(&String::from_utf8_lossy(&out_default.stdout));

    // With override fdivide:9 => lower cycle count (matches reference 153).
    install_override();
    let out_over = Command::new(&bin).arg(&o64).args(args).output().unwrap();
    let override_cycles = cycles(&String::from_utf8_lossy(&out_over.stdout));
    remove_override();

    assert_ne!(
        default_cycles, override_cycles,
        "instructions.data override had no effect"
    );
    assert_eq!(
        override_cycles, "User CPU Cycles: 153",
        "override should reproduce the reference T16_DIVIDE cycle count"
    );
}
