#include "gu.h"
#undef strlcpy
#define strlcpy gu_strlcpy
#include "../nonppr_misc/openbsd/strlcpy.c"
