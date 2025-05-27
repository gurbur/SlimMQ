#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUN_TEST(fn) do {\
	printf("[TEST] %s\n", #fn);\
	fn();\
	printf("[PASS] %s\n\n", #fn);\
} while(0)

#define ASSERT_EQ(a, b) do {\
	if ((a) != (b)) {\
		fprintf(stderr, "[FAIL] ASSERT_EQ failed: %s != %s (%d != %d)\n", #a, #b, (int)(a), (int)(b));\
		exit(1);\
	}\
} while(0)

#define ASSERT_STR_EQ(a, b) do {\
	if (strcmp(a, b) != 0) {\
		fprintf(stderr, "[FAIL] ASSERT_STR_EQ failed: '%s' != '%s'\n", (a), (b));\
		exit(1);\
	}\
} while(0)

#define ASSERT_NOT_NULL(p) do {\
	if ((p) == NULL) {\
		fprintf(stderr, "[FAIL] ASSERT_NOT_NULL failed: %s is NULL\n", #p);\
		exit(1);\
	}\
} while(0)

#define ASSERT_TRUE(cond) do {\
	if (!(cond)) {\
		fprintf(stderr, "[FAIL] ASSERT_TRUE failed: %s\n", #cond);\
		exit(1);\
	}\
} while(0)

