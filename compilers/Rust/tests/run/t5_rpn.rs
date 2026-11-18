// stack-based (RPN) expression evaluator and shunting-yard infix to RPN conversion
#[derive(Debug, Clone, PartialEq)]
enum Tok {
    Num(i64),
    Plus,
    Minus,
    Star,
    Slash,
    LP,
    RP,
}

#[derive(Debug)]
enum EvalError {
    StackUnderflow,
    DivByZero,
    BadToken(String),
    Leftover(usize),
    Paren,
}

fn lex(s: &str) -> Result<Vec<Tok>, EvalError> {
    let mut out = Vec::new();
    let chars: Vec<char> = s.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        if c.is_whitespace() {
            i += 1;
        } else if c.is_ascii_digit() {
            let mut n: i64 = 0;
            while i < chars.len() && chars[i].is_ascii_digit() {
                n = n * 10 + chars[i].to_digit(10).unwrap() as i64;
                i += 1;
            }
            out.push(Tok::Num(n));
        } else {
            let t = match c {
                '+' => Tok::Plus,
                '-' => Tok::Minus,
                '*' => Tok::Star,
                '/' => Tok::Slash,
                '(' => Tok::LP,
                ')' => Tok::RP,
                other => return Err(EvalError::BadToken(other.to_string())),
            };
            out.push(t);
            i += 1;
        }
    }
    Ok(out)
}

fn prec(t: &Tok) -> u8 {
    match t {
        Tok::Plus | Tok::Minus => 1,
        Tok::Star | Tok::Slash => 2,
        _ => 0,
    }
}

fn to_rpn(toks: &[Tok]) -> Result<Vec<Tok>, EvalError> {
    let mut out: Vec<Tok> = Vec::new();
    let mut ops: Vec<Tok> = Vec::new();
    for t in toks {
        match t {
            Tok::Num(_) => out.push(t.clone()),
            Tok::LP => ops.push(Tok::LP),
            Tok::RP => {
                loop {
                    match ops.pop() {
                        Some(Tok::LP) => break,
                        Some(op) => out.push(op),
                        None => return Err(EvalError::Paren),
                    }
                }
            }
            op => {
                while let Some(top) = ops.last() {
                    if *top != Tok::LP && prec(top) >= prec(op) {
                        out.push(ops.pop().unwrap());
                    } else {
                        break;
                    }
                }
                ops.push(op.clone());
            }
        }
    }
    while let Some(op) = ops.pop() {
        if op == Tok::LP {
            return Err(EvalError::Paren);
        }
        out.push(op);
    }
    Ok(out)
}

fn eval_rpn(rpn: &[Tok]) -> Result<i64, EvalError> {
    let mut st: Vec<i64> = Vec::new();
    for t in rpn {
        if let Tok::Num(n) = t {
            st.push(*n);
            continue;
        }
        let b = st.pop().ok_or(EvalError::StackUnderflow)?;
        let a = st.pop().ok_or(EvalError::StackUnderflow)?;
        let r = match t {
            Tok::Plus => a + b,
            Tok::Minus => a - b,
            Tok::Star => a * b,
            Tok::Slash => {
                if b == 0 {
                    return Err(EvalError::DivByZero);
                }
                a / b
            }
            _ => return Err(EvalError::BadToken(format!("{:?}", t))),
        };
        st.push(r);
    }
    match st.len() {
        1 => Ok(st[0]),
        0 => Err(EvalError::StackUnderflow),
        n => Err(EvalError::Leftover(n)),
    }
}

fn calc(s: &str) -> Result<i64, EvalError> {
    let toks = lex(s)?;
    let rpn = to_rpn(&toks)?;
    eval_rpn(&rpn)
}

fn rpn_only(s: &str) -> Result<i64, EvalError> {
    eval_rpn(&lex(s)?)
}

fn main() {
    for e in ["1 + 2 * 3", "(1 + 2) * 3", "100 / 7 - 2", "2 * (3 + 4) * 5", "8 / (4 - 4)", "1 +", "3 $ 4", "(1 + 2", "10 - 2 - 3"] {
        match calc(e) {
            Ok(v) => println!("{} = {}", e, v),
            Err(err) => println!("{} -> {:?}", e, err),
        }
    }
    println!("{:?}", rpn_only("3 4 + 2 *").ok());
    println!("{:?}", rpn_only("1 2 3").is_err());
    println!("{:?}", to_rpn(&lex("1+2*3").unwrap()).unwrap());
}
