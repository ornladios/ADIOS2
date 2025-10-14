#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <direct.h>
#include <io.h>
#include <windows.h>
#define chdir(x) _chdir(x)
#define open(x, y) _open(x, y)
#define lseek(x, y, z) _lseek(x, y, z)
#define write(x, y, z) _write(x, y, z)
#define close(x) _close(x)
#endif

int errflg = 0;

unsigned file_checksum(char *filename)
{
    unsigned int sum;
    unsigned int nbytes = 0;
    int c;
    FILE *f;
    if ((f = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "sum: Can't open %s\n", filename);
        errflg += 10;
        return 0xffffffff;
    }
    sum = 0;
    while ((c = getc(f)) != EOF)
    {
        nbytes++;
        if (sum & 01)
            sum = (sum >> 1) + 0x8000;
        else
            sum >>= 1;
        sum += c;
        sum &= 0xFFFF;
    }
    if (ferror(f))
    {
        errflg++;
        fprintf(stderr, "sum: read error on %s\n", filename);
    }
    fclose(f);
    return sum | (nbytes << 16);
}

int main(int argc, char **argv)
{
    unsigned int base_sum, sum;
    int unique = 0, version_overwrite = 0;
    int i = 1, found_match = 0;
    int verbose = 0;

    while (argv[i])
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            verbose++;
            argv++;
            argc--;
        }
        else if (strcmp(argv[i], "-test_unique") == 0)
        {
            unique++;
            argv++;
            argc--;
        }
        else if (strcmp(argv[i], "-make_version_neutral") == 0)
        {
            version_overwrite++;
            argv++;
            argc--;
        }
        else if (argv[i][0] != '-')
        {
            break;
        }
    }
    if (unique && version_overwrite)
    {
        printf("Only one of -unique and -make_version_neutral can be specified\n");
        exit(1);
    }
    else if ((unique + version_overwrite) == 0)
    {
        printf("Must specify one of -unique or -version_overwrite\n");
        exit(1);
    }
    else if (unique)
    {
        base_sum = file_checksum(argv[1]);
        i++;
        if (verbose)
            printf(" %s %x\n", argv[1], base_sum);
        while (i < argc)
        {
            sum = file_checksum(argv[i]);
            if (sum == base_sum)
            {
                found_match++;
            }
            if (verbose)
                printf(" %s %x\n", argv[i], sum);
            i++;
        }
        if (!found_match)
        {
            printf("The file %s has a unique size and checksum value and should be placed in the "
                   "zipped_output directory.\n",
                   argv[1]);
            printf("Name the file appropriately using the basename, ADIOS version, enging name and "
                   "any "
                   "relevant architecture information.\n");
            return 1;
        }
        else
        {
            if (verbose)
                printf("The file %s duplicates size and checksum values already represented in the "
                       "zipped_output directory.\n",
                       argv[1]);
        }
    }
    else
    {
        char buf[36];
        printf("chdir to %s\n", argv[1]);
        int ret = chdir(argv[1]);
        if (ret)
            perror("chdir");
        int fd = open("md.idx", O_WRONLY);
        if (fd == -1)
        {
            fprintf(stderr, "Open failed\n");
            exit(1);
        }
        ret = lseek(fd, 0, SEEK_SET);
        if (ret == -1)
        {
            fprintf(stderr, "Lseek failed\n");
            exit(1);
        }
        memset(buf, 0, sizeof(buf));
        sprintf(&buf[0], "This is dummy ADIOS metadata");
        ret = write(fd, buf, sizeof(buf));
        if (ret == -1)
        {
            fprintf(stderr, "Write failed\n");
            exit(1);
        }
        close(fd);
    }
    return 0;
}
