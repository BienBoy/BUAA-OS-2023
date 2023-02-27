#include <stdio.h>
#include <string.h>

int ispalindrome(char *s) {
    for (int a = 0, b = strlen(s) - 1; a <= b; a++, b--)
        if (s[a] != s[b])
            return 0;
    return 1;
}

int main() {
    int n, i = 0;
    char s[20];
    scanf("%d", &n);
    while (n) {
        s[i++] = (char)(n % 10 + '0');
        n /= 10;
    }
    s[i] = '\0';

    if (ispalindrome(s)) {
        printf("Y\n");
    } else {
        printf("N\n");
    }
    return 0;
}
