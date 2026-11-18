// read lines from stdin with read_line: a tiny calculator, one "a op b" command per line; exit status = number of errors

fn eval(line: &str) -> Result<i64, String> {
    let parts: Vec<&str> = line.split_whitespace().collect();
    if parts.len() != 3 {
        return Err(format!("expected 3 fields, got {}", parts.len()));
    }
    let a: i64 = parts[0].parse().map_err(|_| format!("bad number {}", parts[0]))?;
    let b: i64 = parts[2].parse().map_err(|_| format!("bad number {}", parts[2]))?;
    match parts[1] {
        "+" => Ok(a + b),
        "-" => Ok(a - b),
        "*" => Ok(a * b),
        "/" => {
            if b == 0 {
                Err("division by zero".to_string())
            } else {
                Ok(a / b)
            }
        }
        op => Err(format!("unknown operator {}", op)),
    }
}

fn main() {
    let mut errors = 0;
    let mut n = 0;
    loop {
        let mut line = String::new();
        if std::io::stdin().read_line(&mut line).unwrap() == 0 {
            break;
        }
        n += 1;
        let line = line.trim_end();
        match eval(line) {
            Ok(v) => println!("{}: {} = {}", n, line, v),
            Err(e) => {
                println!("{}: error: {}", n, e);
                errors += 1;
            }
        }
    }
    if errors > 0 {
        std::process::exit(errors);
    }
}
