// exit from nested calls, status chosen by validating data; exit 0 is not used
fn validate(v: &[i32]) -> Result<i32, String> {
    let mut total = 0;
    for &x in v {
        if x < 0 {
            return Err(format!("negative value {}", x));
        }
        total += x;
    }
    Ok(total)
}

fn run() -> i32 {
    match validate(&[1, 2, 3]) {
        Ok(t) => println!("total {}", t),
        Err(e) => {
            eprintln!("{}", e);
            return 1;
        }
    }
    match validate(&[4, -5, 6]) {
        Ok(t) => println!("total {}", t),
        Err(e) => {
            println!("failed: {}", e);
            return 3;
        }
    }
    0
}

fn main() {
    let code = run();
    if code != 0 {
        std::process::exit(code);
    }
    println!("done");
}
