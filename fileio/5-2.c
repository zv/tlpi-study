/* Exercise 5-2
 * Write a program that opens an existing file for wrtiting with the `O_APPEND`
 * flag, and then seeks to the beginning of the file before writing some data.
 * Where does the data appear in the file? Why?
 */

/*
 * Answer: A file opened with `O_APPEND` can only append data, never seek back to a point that
 */

#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/tlpi_hdr.h"

int main(int argc, char *argv[])
{
    int fd;

    if (argc < 2) {
        usageErr("%s file data\n", argv[0]);
    }

    // Assign our `data` to `buf`
    char *buf = argv[2];
    int len = strlen(buf);

    fd = open(argv[1], O_WRONLY  | O_APPEND);

    if (fd == -1) errExit("open failed");

    // seek to 'beginning'
    if (lseek(fd, 0, SEEK_SET) == -1) errExit("lseek failed");

    // and write our buffer data
    if (write(fd, buf, len) != len) fatal("partial/failed write");

    return 0;
}


