// matrix multiply: fixed arrays, Vec<Vec<>>, a Matrix struct, transpose, power
const N: usize = 3;

fn mul_fixed(a: &[[i64; N]; N], b: &[[i64; N]; N]) -> [[i64; N]; N] {
    let mut c = [[0i64; N]; N];
    for i in 0..N {
        for j in 0..N {
            for k in 0..N {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    c
}

#[derive(Clone, Debug, PartialEq)]
struct Matrix {
    r: usize,
    c: usize,
    d: Vec<f64>,
}

impl Matrix {
    fn zeros(r: usize, c: usize) -> Matrix {
        Matrix { r, c, d: vec![0.0; r * c] }
    }
    fn identity(n: usize) -> Matrix {
        let mut m = Matrix::zeros(n, n);
        for i in 0..n {
            m.set(i, i, 1.0);
        }
        m
    }
    fn from_rows(rows: &[Vec<f64>]) -> Matrix {
        let r = rows.len();
        let c = rows[0].len();
        let mut m = Matrix::zeros(r, c);
        for i in 0..r {
            for j in 0..c {
                m.set(i, j, rows[i][j]);
            }
        }
        m
    }
    fn get(&self, i: usize, j: usize) -> f64 {
        self.d[i * self.c + j]
    }
    fn set(&mut self, i: usize, j: usize, v: f64) {
        self.d[i * self.c + j] = v;
    }
    fn mul(&self, o: &Matrix) -> Option<Matrix> {
        if self.c != o.r {
            return None;
        }
        let mut m = Matrix::zeros(self.r, o.c);
        for i in 0..self.r {
            for j in 0..o.c {
                let mut s = 0.0;
                for k in 0..self.c {
                    s += self.get(i, k) * o.get(k, j);
                }
                m.set(i, j, s);
            }
        }
        Some(m)
    }
    fn transpose(&self) -> Matrix {
        let mut m = Matrix::zeros(self.c, self.r);
        for i in 0..self.r {
            for j in 0..self.c {
                m.set(j, i, self.get(i, j));
            }
        }
        m
    }
    fn pow(&self, mut e: u32) -> Matrix {
        let mut base = self.clone();
        let mut acc = Matrix::identity(self.r);
        while e > 0 {
            if e & 1 == 1 {
                acc = acc.mul(&base).unwrap();
            }
            base = base.mul(&base).unwrap();
            e >>= 1;
        }
        acc
    }
    fn print(&self) {
        for i in 0..self.r {
            let row: Vec<String> = (0..self.c).map(|j| format!("{:7.2}", self.get(i, j))).collect();
            println!("{}", row.join(" "));
        }
    }
}

fn main() {
    let a = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
    let b = [[9, 8, 7], [6, 5, 4], [3, 2, 1]];
    let c = mul_fixed(&a, &b);
    for row in c.iter() {
        println!("{:?}", row);
    }
    let id = [[1, 0, 0], [0, 1, 0], [0, 0, 1]];
    println!("{}", mul_fixed(&a, &id) == a);

    let m = Matrix::from_rows(&[vec![1.0, 2.0, 3.0], vec![4.0, 5.0, 6.0]]);
    let t = m.transpose();
    m.mul(&t).unwrap().print();
    println!("{}", m.mul(&m).is_none());
    t.mul(&m).unwrap().print();

    let fib = Matrix::from_rows(&[vec![1.0, 1.0], vec![1.0, 0.0]]);
    let p = fib.pow(30);
    println!("{}", p.get(0, 1));
    println!("{}", Matrix::identity(2) == Matrix::identity(2).pow(5));
}
