#include "inc/a.h"
#include "inc/a.h"
#include "inc/b.h"
#include <sys.h>
#define HDR "inc/b.h"
#include HDR
#define SYSH <sys.h>
#include SYSH
int v = SYS_VERSION;
