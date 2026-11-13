static int foo_helper(int x) { return x * FOO_SCALE; }
int foo_used(int x) { return foo_helper(x) + 1; }
int foo_unused(int x) { return x - 12345; }
