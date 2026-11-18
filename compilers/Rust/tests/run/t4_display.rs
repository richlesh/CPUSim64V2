// Display impls with padding, Debug, to_string, nested Display, write! to a String
use std::fmt;
use std::fmt::Write;

struct Temp(f64);

impl fmt::Display for Temp {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:.1}C", self.0)
    }
}

struct Point {
    x: i32,
    y: i32,
}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "({}, {})", self.x, self.y)
    }
}

impl fmt::Debug for Point {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Point<{},{}>", self.x, self.y)
    }
}

enum Token {
    Num(i64),
    Op(char),
    LParen,
    RParen,
}

impl fmt::Display for Token {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Token::Num(n) => write!(f, "{}", n),
            Token::Op(c) => write!(f, "{}", c),
            Token::LParen => f.write_str("("),
            Token::RParen => f.write_str(")"),
        }
    }
}

struct Matrix {
    rows: Vec<Vec<i32>>,
}

impl fmt::Display for Matrix {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for (i, row) in self.rows.iter().enumerate() {
            if i > 0 {
                writeln!(f)?;
            }
            for (j, v) in row.iter().enumerate() {
                if j > 0 {
                    write!(f, " ")?;
                }
                write!(f, "{:3}", v)?;
            }
        }
        Ok(())
    }
}

struct Name(&'static str);

impl fmt::Display for Name {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.pad(self.0)
    }
}

fn main() {
    println!("{}", Temp(21.55));
    let p = Point { x: 3, y: -4 };
    println!("{} {:?}", p, p);
    println!("{}", p.to_string().len());
    let toks = vec![Token::LParen, Token::Num(12), Token::Op('+'), Token::Num(3), Token::RParen];
    let mut s = String::new();
    for t in &toks {
        s.push_str(&t.to_string());
    }
    println!("{}", s);
    s.push_str("!\n");
    print!("{}", s);
    let m = Matrix { rows: vec![vec![1, 20, 300], vec![-4, 5, 6]] };
    println!("{}", m);
    println!("[{:>6}] [{:<6}] [{:^6}]", Name("ab"), Name("cd"), Name("ef"));
    println!("[{:>10}]", format!("{}", p));
    println!("{:?}", vec![Point { x: 1, y: 2 }]);
    println!("{:?}", Some(Point { x: 0, y: 0 }));
    println!("{:#?}", Point { x: 1, y: 1 });
}
