int g = 2;
int main(void)
{
    static _Thread_local int x = 1 + 1;
    static _Thread_local int y = g;
    return x + y;
}
