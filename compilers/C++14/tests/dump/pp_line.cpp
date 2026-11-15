int a = __LINE__;
#line 100
int b = __LINE__;
#line 200 "other.cpp"
const char *f = __FILE__; int c = __LINE__;
int d = __COUNTER__ + __COUNTER__;
# 300 "third.cpp"
int e = __LINE__ __FILE__;
#define L __LINE__
int g = L;
int h = L
  + L;
