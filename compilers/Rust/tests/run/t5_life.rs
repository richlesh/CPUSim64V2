// Conway's Game of Life on a fixed 10x10 torus: blinker, glider, block
const W: usize = 10;
const H: usize = 10;

type Grid = [[bool; W]; H];

fn neighbors(g: &Grid, r: usize, c: usize) -> u32 {
    let mut n = 0;
    for dr in [H - 1, 0, 1] {
        for dc in [W - 1, 0, 1] {
            if dr == 0 && dc == 0 {
                continue;
            }
            if g[(r + dr) % H][(c + dc) % W] {
                n += 1;
            }
        }
    }
    n
}

fn step(g: &Grid) -> Grid {
    let mut out = [[false; W]; H];
    for r in 0..H {
        for c in 0..W {
            let n = neighbors(g, r, c);
            out[r][c] = match (g[r][c], n) {
                (true, 2) | (true, 3) => true,
                (false, 3) => true,
                _ => false,
            };
        }
    }
    out
}

fn show(g: &Grid) {
    for row in g.iter() {
        let line: String = row.iter().map(|&b| if b { '#' } else { '.' }).collect();
        println!("{}", line);
    }
}

fn count(g: &Grid) -> usize {
    g.iter().map(|row| row.iter().filter(|&&b| b).count()).sum()
}

fn place(g: &mut Grid, cells: &[(usize, usize)]) {
    for &(r, c) in cells {
        g[r][c] = true;
    }
}

fn main() {
    let mut g = [[false; W]; H];
    place(&mut g, &[(1, 2), (2, 3), (3, 1), (3, 2), (3, 3)]);
    place(&mut g, &[(7, 6), (7, 7), (7, 8)]);
    place(&mut g, &[(8, 1), (8, 2), (9, 1), (9, 2)]);
    println!("gen 0 ({} alive)", count(&g));
    show(&g);
    for gen in 1..=4 {
        g = step(&g);
        println!("gen {} ({} alive)", gen, count(&g));
        if gen == 1 || gen == 4 {
            show(&g);
        }
    }
    let start = g;
    let mut period = 0;
    for i in 1..=200 {
        g = step(&g);
        if g == start {
            period = i;
            break;
        }
    }
    println!("returns to gen 4 state after {} steps", period);
    println!("{}", count(&g));
}
