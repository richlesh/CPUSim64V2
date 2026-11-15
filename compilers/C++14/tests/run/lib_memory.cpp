#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <utility>

struct Res {
    int id;
    Res(int i) : id(i) { printf("Res(%d)\n", id); }
    ~Res() { printf("~Res(%d)\n", id); }
    virtual int kind() const { return 0; }
};
struct SubRes : Res {
    SubRes(int i) : Res(i) {}
    int kind() const { return 1; }
};

std::unique_ptr<Res> make(int i) { return std::unique_ptr<Res>(new Res(i)); }

struct Node {
    int v;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;
    Node(int x) : v(x) {}
    ~Node() { printf("~Node %d\n", v); }
};

int main()
{
    {
        std::unique_ptr<Res> a(new Res(1));
        std::unique_ptr<Res> b = make(2);
        printf("%d %d\n", a->id, (*b).id);
        std::unique_ptr<Res> c = std::move(a);
        printf("%d %d\n", (int)(a == nullptr), c->id);
        b.reset(new Res(3));
        printf("after reset\n");
        auto d = std::make_unique<SubRes>(4);
        std::unique_ptr<Res> base = std::move(d);
        printf("%d\n", base->kind());
        Res *raw = c.release();
        printf("released %d\n", raw->id);
        delete raw;
    }
    printf("--\n");
    {
        std::vector<std::unique_ptr<Res>> v;
        v.push_back(std::make_unique<Res>(10));
        v.push_back(std::make_unique<Res>(11));
        v.emplace_back(new Res(12));
        printf("size %d\n", (int)v.size());
        v.erase(v.begin());
        printf("erased\n");
    }
    printf("--\n");
    {
        std::shared_ptr<Res> p = std::make_shared<Res>(20);
        std::shared_ptr<Res> q = p;
        printf("%ld\n", p.use_count());
        {
            std::shared_ptr<Res> r(q);
            printf("%ld\n", p.use_count());
        }
        printf("%ld\n", p.use_count());
        q.reset();
        printf("%ld %d\n", p.use_count(), (int)(q == nullptr));
        std::weak_ptr<Res> w = p;
        printf("%d\n", (int)w.expired());
        p.reset();
        printf("%d\n", (int)w.expired());
        std::shared_ptr<Res> sub = std::make_shared<SubRes>(21);
        std::shared_ptr<Res> asbase = sub;
        printf("%d\n", asbase->kind());
        std::shared_ptr<SubRes> back = std::dynamic_pointer_cast<SubRes>(asbase);
        printf("%d\n", back ? back->kind() : -1);
    }
    printf("--\n");
    {
        std::shared_ptr<Node> n1 = std::make_shared<Node>(1);
        std::shared_ptr<Node> n2 = std::make_shared<Node>(2);
        n1->next = n2;
        n2->prev = n1;
        printf("%ld %ld\n", n1.use_count(), n2.use_count());
    }
    return 0;
}
