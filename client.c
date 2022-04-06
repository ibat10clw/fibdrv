#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

int main()
{
    long long sz;
    char buf[110];
    char write_buf[110] = "testing writing";
    int offset = 500; /* TODO: try test something bigger than the limit */
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(buf));
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
        /*
        sz = write(fd, write_buf, strlen(write_buf));
        printf("use %lld ns in kernel\n", sz);
        printf("use %ld ns in client\n", t2.tv_nsec - t1.tv_nsec);
        printf("%d %lld %ld %lld\n", i, sz, t2.tv_nsec - t1.tv_nsec, (t2.tv_nsec
        - t1.tv_nsec) - sz);//k, u, k2u
        */
    }


    close(fd);
    return 0;
}
