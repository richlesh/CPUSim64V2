int f(void) { return 3; }
_Thread_local int x = f();
int main(void) { return x; }
