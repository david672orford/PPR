#include "gu.h"
#undef strlcat
#define strlcat gu_strlcat
#include "../nonppr_misc/openbsd/strlcat.c"
