#pragma once

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Asserts should be able to be stripped if that is wanted for production */
#ifdef WI_STRIP_ASSERTS

#define wiAssert(PREDICATE, ...)
#define wiAssertCallback(PREDICATE, CALLBACK, ...)

#else

#define wiAssert(PREDICATE, ...) \
{ \
	if (!(PREDICATE)) { \
		char* oldLocale = strdup(setlocale(LC_ALL, NULL)); \
		setlocale(LC_ALL, ""); \
		fprintf(stderr, "\033[91;1mAssertion\033[0m: %s\n", #PREDICATE); \
		\
		char* varArgs[] = { __VA_ARGS__ "", "" }, **n; \
		n = varArgs; \
		if (strcmp(*n, "") != 0) { \
			fprintf(stderr, "\u2502 Message:\n"); \
		} \
		while (strcmp(*n, "") != 0) { \
			fprintf(stderr, "\u2502 \t%s\n", *n); \
			n++; \
		} \
		\
		fprintf(stderr, \
			"\u2570 At function '%s', file '%s', line %d\n\n", \
			__func__, __FILE__, __LINE__ \
		   ); \
		setlocale(LC_ALL, oldLocale); \
		free(oldLocale); \
		abort(); \
	} \
}

#define wiAssertCallback(PREDICATE, CALLBACK, ...) { \
	if (!(PREDICATE)) { \
		char* oldLocale = strdup(setlocale(LC_ALL, NULL)); \
		setlocale(LC_ALL, ""); \
		fprintf(stderr, "\033[91;1mAssertion\033[0m: %s\n", #PREDICATE); \
		\
		char* varArgs[] = { __VA_ARGS__ "", "" }, **n; \
		n = varArgs; \
		if (strcmp(*n, "") != 0) { \
			fprintf(stderr, "\u2502 Message:\n"); \
		} \
		while (strcmp(*n, "") != 0) { \
			fprintf(stderr, "\u2502 \t%s\n", *n); \
			n++; \
		} \
		\
		fprintf(stderr, \
			"\u2570 At function '%s', file '%s', line %d\n\n", \
			__func__, __FILE__, __LINE__ \
		); \
		setlocale(LC_ALL, oldLocale); \
		CALLBACK; \
		free(oldLocale); \
		abort(); \
	} \
}

#endif // WI_STRIP_ASSERTS
