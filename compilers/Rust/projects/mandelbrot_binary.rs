//! mandelbrot_binary.rs
//!
//! Mandelbrot set image generator.  It renders the set as a grayscale PGM image
//! in the binary (P5) format, one byte for each pixel.  Rust version of
//! documentation/projects/mandelbrot_binary.asm.
//!
//! CPUSim64's Rust library has no processes, threads or files: the program
//! computes the image in one thread, and writes it to standard output.  The
//! arguments are the first line of standard input.
//!
//! Input (first line of standard input):
//!     x y radius escape_limit image_size
//!
//! Example (the image of the documentation):
//!     -0.748127681290498 0.09408613781852182 1e-6 10000 128

use std::io::Write;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("mandelbrot_binary")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

struct View {
    x_min: f64,
    x_max: f64,
    y_min: f64,
    y_max: f64,
    max_iteration: i64,
}

impl View {
    /// the escape time of the point (x0, y0): iterations / max_iteration, 0 if it never escapes
    fn compute_escape(&self, x0: f64, y0: f64) -> f64 {
        let (mut x, mut y, mut x2, mut y2) = (0.0f64, 0.0f64, 0.0f64, 0.0f64);
        let mut iteration = 0;
        loop {
            y = y * x * 2.0 + y0;
            x = x2 - y2 + x0;
            x2 = x * x;
            y2 = y * y;
            iteration += 1;
            if !(x2 + y2 <= 4.0 && iteration < self.max_iteration) {
                break;
            }
        }
        if iteration == self.max_iteration {
            return 0.0;
        }
        iteration as f64 / self.max_iteration as f64
    }

    /// the image as bytes
    fn render(&self, w: i64) -> Vec<u8> {
        let x_width = self.x_max - self.x_min;
        let y_height = self.y_max - self.y_min;
        let mut out = Vec::new();
        for j in 0..w {
            for i in 0..w {
                let x0 = (i as f64 + 0.5) * x_width / w as f64 + self.x_min;
                let y0 = self.y_max - (j as f64 + 0.5) * y_height / w as f64;
                out.push((self.compute_escape(x0, y0) * 256.0) as i64 as u8);
            }
        }
        out
    }
}

fn main() {
    let argv = args();
    if argv.len() != 6 {
        println!("Syntax: mandelbrot_binary x y radius escape_limit image_size");
        std::process::exit(1);
    }
    let x: f64 = argv[1].parse().unwrap_or(0.0);
    let y: f64 = argv[2].parse().unwrap_or(0.0);
    let radius: f64 = argv[3].parse().unwrap_or(0.0);
    let max_iteration: i64 = argv[4].parse().unwrap_or(0);
    let size: i64 = argv[5].parse().unwrap_or(0);
    let view = View { x_min: x - radius, x_max: x + radius, y_min: y - radius, y_max: y + radius, max_iteration };
    let mut out = format!("P5\n{} {}\n255\n", size, size).into_bytes();
    out.extend(view.render(size));
    std::io::stdout().write_all(&out).unwrap();
}
