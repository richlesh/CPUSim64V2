use std::cell::RefCell;
use std::rc::Rc;

type Adj = Rc<RefCell<Vec<usize>>>;

struct Graph {
    adj: Vec<Adj>,
}

impl Graph {
    fn new(n: usize) -> Graph {
        let mut adj = Vec::new();
        for _ in 0..n {
            adj.push(Rc::new(RefCell::new(Vec::new())));
        }
        Graph { adj }
    }
    fn edge(&self, a: usize, b: usize) {
        self.adj[a].borrow_mut().push(b);
        self.adj[b].borrow_mut().push(a);
    }
    fn bfs(&self, start: usize) -> Vec<usize> {
        let mut seen = vec![false; self.adj.len()];
        let mut order = Vec::new();
        let mut queue = vec![start];
        seen[start] = true;
        let mut head = 0;
        while head < queue.len() {
            let u = queue[head];
            head += 1;
            order.push(u);
            for &v in self.adj[u].borrow().iter() {
                if !seen[v] {
                    seen[v] = true;
                    queue.push(v);
                }
            }
        }
        order
    }
}

fn main() {
    let g = Graph::new(7);
    g.edge(0, 1);
    g.edge(0, 2);
    g.edge(1, 3);
    g.edge(2, 3);
    g.edge(3, 4);
    g.edge(5, 6);
    println!("bfs 0: {:?}", g.bfs(0));
    println!("bfs 5: {:?}", g.bfs(5));
    let shared = g.adj[3].clone();
    println!("count = {}", Rc::strong_count(&shared));
    shared.borrow_mut().push(6);
    println!("adj3 = {:?}", g.adj[3].borrow());
    let degrees: Vec<usize> = g.adj.iter().map(|a| a.borrow().len()).collect();
    println!("degrees = {:?}", degrees);
    drop(g);
    println!("after drop count = {} {:?}", Rc::strong_count(&shared), shared);
}
