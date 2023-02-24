#include <blib.h>

size_t strlen(const char *s) {
	size_t len;
	for (len = 0; s[len]; len++)
		;
	return len;
}

char *strcpy(char *dst, const char *src) {
	for (int i = 0; src[i]; i++)
		dst[i] = src[i];
	return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
	char *res = dst;
	while (*src && n--) {
		*dst++ = *src++;
	}
	*dst = '\0';
	return res;
}

char *strcat(char *dst, const char *src) {
	int len = strlen(dst);
	for (int i = 0; src[i]; i++)
		dst[len + i] = src[i];
	return dst;
}

int strcmp(const char *s1, const char *s2) {
	for (int i = 0; s1[i] || s2[i]; i++)
		if (s1[i] != s2[i])
			return s1[i] - s2[i];
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n--) {
		if (*s1 != *s2) {
			return *s1 - *s2;
		}
		if (*s1 == 0) {
			break;
		}
		s1++;
		s2++;
	}
	return 0;
}

void *memset(void *s, int c, size_t n) {
	char* pt = (char*)s;
	while(n--)
		*pt++ = c;
	return s;
}

void *memcpy(void *out, const void *in, size_t n) {
	char *csrc = (char *)in;
	char *cdest = (char *)out;
	for (int i = 0; i < n; i++) {
		cdest[i] = csrc[i];
	}
	return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	char* pt1 = (char*)s1;
	char* pt2 = (char*)s2;
	while (n--)
		if (*pt1++ != *pt2++)
			return *--pt1 - *--pt2;
	return 0;
}
