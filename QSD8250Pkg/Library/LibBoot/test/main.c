#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

#include <lib/boot.h>

static off_t fdsize(int fd)
{
    off_t off;

    off = lseek(fd, 0L, SEEK_END);
    lseek(fd, 0L, SEEK_SET);

    return off;
}

static void print_error_stack(void)
{
    // print errors
    uint32_t i;
    char **error_stack = libboot_error_stack_get();
    for (i=0; i<libboot_error_stack_count(); i++)
        fprintf(stderr, "[%d] %s\n", i, error_stack[i]);
    libboot_error_stack_reset();
}

static void do_boot(void* data, size_t sz) {
    int rc;

    // init
    libboot_init();

    // setup context
    bootimg_context_t context;
    libboot_init_context(&context);

    // identify type
    rc = libboot_identify_memory(data, sz, &context);
    if (!rc) {
        // load image
        rc = libboot_load(&context);
        if (!rc) {
            fprintf(stderr, "SUCCESS\n");
            rc = 0;
        }
    }

    // print errors
    print_error_stack();

    // cleanup
    libboot_free_context(&context);
    libboot_uninit();

    if(rc) {
        fprintf(stderr, "FAILED\n");
    }
}

int main(int argc, char** argv) {
    int rc;
    off_t off;
    ssize_t ssize;
    void* mem = NULL;

    // check arguments
    if(argc<2) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    // open file
    const char* filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if(fd<0) {
        fprintf(stderr, "can't open %s: %s\n", filename, strerror(errno));
        return 1;
    }

    // get filesize
    off = fdsize(fd);
    if (off<0) {
        fprintf(stderr, "Can't get size of file %s\n", filename);
        rc = 1;
        goto close_file;
    }

    // allocate buffer
    mem = malloc(off);
    if (!mem) {
        fprintf(stderr, "Can't allocate buffer of size %lu\n", off);
        rc = 1;
        goto close_file;
    }

    // read file into memory
    ssize = read(fd, mem, off);
    if (ssize!=off) {
        fprintf(stderr, "Can't read file %s into buffer\n", filename);
        rc = 1;
        goto free_buffer;
    }

    do_boot(mem, off);

    rc = 0;

free_buffer:
    free(mem);
close_file:
    close(fd);

    return rc;
}
