#include <stdio.h>
//
// Generate conversion tables for half precision floating point format.
// From "Fast Half Float Conversions" by Jeroen van der Zijp
// November 2008 (Revised September 2010)
// ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
//
// Generated tables go in vt_convert.inl
//
unsigned int convert_mantissa(unsigned int i)
{
    unsigned int m = i << 13;   // Zero pad mantissa bits
    unsigned int e = 0;         // Zero exponent
    while (!(m & 0x00800000))   // While not normalized
    {
        e -= 0x00800000;        // Decrement exponent (1<<23)
        m <<= 1;                // Shift mantissa
    }
    m &= ~0x00800000;           // Clear leading 1 bit
    e +=  0x38800000;           // Adjust bias ((127-14)<<23)
    return m | e;               // Return combined number
}

void print_table(unsigned int * p, unsigned int n)
{
    printf("{\n");
    for (int j = 0; j < n; j += 8)
    {
        printf("\t");
        for (int i = j; i < j + 8; i++)
	        printf("0x%08x, ", p[i]);
        printf("\n");
    }
    printf("};\n");
}

void print_table(unsigned short * p, unsigned int n)
{
    printf("{\n");
    for (int j = 0; j < n; j += 16)
    {
        printf("\t");
        for (int i = j; i < j + 16; i++)
	        printf("0x%04x, ", p[i]);
        printf("\n");
    }
    printf("};\n");
}

void print_table(unsigned char * p, unsigned int n)
{
    printf("{\n");
    for (int j = 0; j < n; j += 32)
    {
        printf("\t");
        for (int i = j; i < j + 32; i++)
	        printf("%2d, ", p[i]);
        printf("\n");
    }
    printf("};\n");
}

#define HALF_FLOAT_ROUND

void generate_tables()
{
    unsigned short base_table[512];
    unsigned char shift_table[512];
    unsigned char round_table[512];
    unsigned int i;
    int e;
    for (i = 0; i < 256; ++i)
    {
        e = i - 127;
        if (e < -25)
        {
            // Very small numbers map to zero
            base_table[i | 0x000] = 0x0000;
            base_table[i | 0x100] = 0x8000;
            shift_table[i | 0x000] = 24;
            shift_table[i | 0x100] = 24;
            round_table[i | 0x000] = 24;
            round_table[i | 0x100] = 24;
        }
        else if (e < -14)
        {
            // Small numbers map to denorms
            base_table[i | 0x000] = (0x0400 >> (-e - 14));
            base_table[i | 0x100] = (0x0400 >> (-e - 14)) | 0x8000;
            shift_table[i | 0x000] = -e - 1;
            shift_table[i | 0x100] = -e - 1;
            round_table[i | 0x000] = -e - 2;
            round_table[i | 0x100] = -e - 2;
        }
        else if (e <= 15)
        {
            // Normal numbers just lose precision
            base_table[i | 0x000] = ((e + 15) << 10);
            base_table[i | 0x100] = ((e + 15) << 10) | 0x8000;
            shift_table[i | 0x000] = 13;
            shift_table[i | 0x100] = 13;
            round_table[i | 0x000] = 12;
            round_table[i | 0x100] = 12;
        }
        else if (e < 128)
        {
            // Large numbers map to Infinity
            base_table[i | 0x000] = 0x7C00;
            base_table[i | 0x100] = 0xFC00;
            shift_table[i | 0x000] = 24;
            shift_table[i | 0x100] = 24;
            round_table[i | 0x000] = 24;
            round_table[i | 0x100] = 24;
        }
        else
        {
            // Infinity and NaN's stay Infinity and NaN's
            base_table[i | 0x000] = 0x7C00;
            base_table[i | 0x100] = 0xFC00;
            shift_table[i | 0x000] = 13;
            shift_table[i | 0x100] = 13;
            round_table[i | 0x000] = 24;
            round_table[i | 0x100] = 24;
        }
    }

    printf("static unsigned short g_base_table[512] =\n");
    print_table(base_table, 512);

    printf("static unsigned char g_shift_table[512] =\n");
    print_table(shift_table, 512);

    printf("static unsigned char g_round_table[512] =\n");
    print_table(round_table, 512);
}

void main(int argc, char ** argv)
{
    unsigned int mantissa_table[2048];
    mantissa_table[0] = 0;
    for (int i = 1; i < 1024; i++)
        mantissa_table[i] = convert_mantissa(i);
    for (int i = 1024; i < 2048; i++)
        mantissa_table[i] = 0x38000000 + ((i - 1024) << 13);

    printf("static unsigned int g_mantissa_table[2048] =\n");
    print_table(mantissa_table, 2048);

    unsigned int exponent_table[64];
    for (int i = 0; i < 31; i++)
        exponent_table[i] = i << 23;
    exponent_table[31] = 0x47800000;
    for (int i = 32; i < 63; i++)
        exponent_table[i] = 0x80000000 + ((i - 32) << 23);
    exponent_table[63] = 0xC7800000;

    printf("static unsigned int g_exponent_table[64] =\n");
    print_table(exponent_table, 64);

    unsigned short offset_table[64];
    offset_table[0] = 0;
    for (int i = 1; i < 32; i++)
        offset_table[i] = 1024;
    offset_table[32] = 0;
    for (int i = 33; i < 64; i++)
        offset_table[i] = 1024;

    printf("static unsigned short g_offset_table[64] =\n");
    print_table(offset_table, 64);

    generate_tables();
}