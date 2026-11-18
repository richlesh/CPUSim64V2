// tokenizer for a tiny language: identifiers, numbers, strings, operators, comments
#[derive(Debug, Clone, PartialEq)]
enum Token {
    Ident(String),
    Int(i64),
    Float(f64),
    Str(String),
    Op(String),
    Punct(char),
    Keyword(String),
}

struct Lexer<'a> {
    src: &'a [u8],
    pos: usize,
    line: u32,
}

const KEYWORDS: [&str; 5] = ["let", "if", "else", "while", "fn"];

impl<'a> Lexer<'a> {
    fn new(s: &'a str) -> Lexer<'a> {
        Lexer { src: s.as_bytes(), pos: 0, line: 1 }
    }

    fn peek(&self) -> Option<u8> {
        if self.pos < self.src.len() {
            Some(self.src[self.pos])
        } else {
            None
        }
    }

    fn peek2(&self) -> Option<u8> {
        if self.pos + 1 < self.src.len() {
            Some(self.src[self.pos + 1])
        } else {
            None
        }
    }

    fn skip_space(&mut self) {
        while let Some(c) = self.peek() {
            if c == b'\n' {
                self.line += 1;
                self.pos += 1;
            } else if c == b' ' || c == b'\t' || c == b'\r' {
                self.pos += 1;
            } else if c == b'#' {
                while let Some(d) = self.peek() {
                    if d == b'\n' {
                        break;
                    }
                    self.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    fn next_token(&mut self) -> Result<Option<Token>, String> {
        self.skip_space();
        let c = match self.peek() {
            Some(c) => c,
            None => return Ok(None),
        };
        if (c as char).is_ascii_alphabetic() || c == b'_' {
            let start = self.pos;
            while let Some(d) = self.peek() {
                if (d as char).is_ascii_alphanumeric() || d == b'_' {
                    self.pos += 1;
                } else {
                    break;
                }
            }
            let word = String::from_utf8(self.src[start..self.pos].to_vec()).unwrap();
            if KEYWORDS.contains(&word.as_str()) {
                return Ok(Some(Token::Keyword(word)));
            }
            return Ok(Some(Token::Ident(word)));
        }
        if (c as char).is_ascii_digit() {
            let start = self.pos;
            let mut is_float = false;
            while let Some(d) = self.peek() {
                if (d as char).is_ascii_digit() {
                    self.pos += 1;
                } else if d == b'.' && !is_float {
                    is_float = true;
                    self.pos += 1;
                } else {
                    break;
                }
            }
            let text = String::from_utf8(self.src[start..self.pos].to_vec()).unwrap();
            if is_float {
                return match text.parse::<f64>() {
                    Ok(f) => Ok(Some(Token::Float(f))),
                    Err(_) => Err(format!("line {}: bad float {}", self.line, text)),
                };
            }
            return match text.parse::<i64>() {
                Ok(n) => Ok(Some(Token::Int(n))),
                Err(_) => Err(format!("line {}: bad int {}", self.line, text)),
            };
        }
        if c == b'"' {
            self.pos += 1;
            let mut s = String::new();
            loop {
                match self.peek() {
                    None => return Err(format!("line {}: unterminated string", self.line)),
                    Some(b'"') => {
                        self.pos += 1;
                        break;
                    }
                    Some(b'\\') => {
                        self.pos += 1;
                        match self.peek() {
                            Some(b'n') => s.push('\n'),
                            Some(b't') => s.push('\t'),
                            Some(other) => s.push(other as char),
                            None => return Err("eof in escape".to_string()),
                        }
                        self.pos += 1;
                    }
                    Some(b) => {
                        s.push(b as char);
                        self.pos += 1;
                    }
                }
            }
            return Ok(Some(Token::Str(s)));
        }
        let two = match (c, self.peek2()) {
            (b'=', Some(b'=')) => Some("=="),
            (b'!', Some(b'=')) => Some("!="),
            (b'<', Some(b'=')) => Some("<="),
            (b'>', Some(b'=')) => Some(">="),
            (b'&', Some(b'&')) => Some("&&"),
            (b'|', Some(b'|')) => Some("||"),
            _ => None,
        };
        if let Some(op) = two {
            self.pos += 2;
            return Ok(Some(Token::Op(op.to_string())));
        }
        self.pos += 1;
        match c {
            b'+' | b'-' | b'*' | b'/' | b'=' | b'<' | b'>' | b'!' => {
                Ok(Some(Token::Op((c as char).to_string())))
            }
            b'(' | b')' | b'{' | b'}' | b';' | b',' => Ok(Some(Token::Punct(c as char))),
            _ => Err(format!("line {}: unexpected '{}'", self.line, c as char)),
        }
    }
}

fn tokenize(src: &str) -> Result<Vec<Token>, String> {
    let mut lx = Lexer::new(src);
    let mut out = Vec::new();
    while let Some(t) = lx.next_token()? {
        out.push(t);
    }
    Ok(out)
}

fn main() {
    let prog = "let x = 42;\n# a comment\nif x >= 10 && x != 3.5 {\n  print(\"big\\n\", x_1);\n}\n";
    match tokenize(prog) {
        Ok(toks) => {
            println!("{} tokens", toks.len());
            for t in &toks {
                println!("{:?}", t);
            }
        }
        Err(e) => println!("error {}", e),
    }
    println!("{:?}", tokenize("a = \"oops"));
    println!("{:?}", tokenize("x\ny\n@"));
    println!("{:?}", tokenize("1.2.3"));
    println!("{:?}", tokenize("   ").map(|v| v.len()));
}
