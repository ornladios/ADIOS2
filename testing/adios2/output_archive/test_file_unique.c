#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int i, found_match = 0;
    int verbose = 0;

    i = 2;
    if (strcmp(argv[1], "-v") == 0)
    {
        verbose++;
        argv++;
        argc--;
    }
    base_sum = file_checksum(argv[1]);
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
        printf("Name the file appropriately using the basename, ADIOS version, enging name and any "
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
    return 0;
}
