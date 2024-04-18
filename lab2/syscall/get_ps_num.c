#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
    int result;
    syscall(332, &result);
    printf("process number is %d\n", result);
    return 0;
}