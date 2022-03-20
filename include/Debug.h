#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

#ifndef NO_DEBUG
#	define debug(x) puts("Passed "#x)
#else
#	define debug(x)
#endif 

#endif