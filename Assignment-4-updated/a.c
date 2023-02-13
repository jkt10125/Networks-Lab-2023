#include <stdio.h>
int main() {
    int a[] = {7, 8, 5, 6, 4};
    int *b = &a[3];
    a[4] = 0;
    printf("%d", b[1]);
}