// https://github.com/Tiny-C-Compiler/tinycc-mirror-repository/blob/mob/lib/va_list.c

void* __stdarg_memcpy(void* dest, const void* src, unsigned long n)
{
    char* pdest = (char*)dest;
    const char* psrc = (const char*)src;

    for (unsigned long i = 0; i < n; i++)
        pdest[i] = psrc[i];

    return dest;
}

// This should be in sync with our include/stdarg.h
enum __va_arg_type
{
    __va_gen_reg,
    __va_float_reg,
    __va_stack
};

void* __va_arg(__builtin_va_list ap, int arg_type, int size, int align)
{
    size = (size + 7) & ~7;
    align = (align + 7) & ~7;

    switch ((enum __va_arg_type)arg_type)
    {
    case __va_gen_reg:
        if (ap->gp_offset + size <= 48)
        {
            ap->gp_offset += size;
            return ap->reg_save_area + ap->gp_offset - size;
        }
        goto use_overflow_area;

    case __va_float_reg:
        if (ap->fp_offset < 128 + 48)
        {
            ap->fp_offset += 16;

            if (size == 8)
                return ap->reg_save_area + ap->fp_offset - 16;

            if (ap->fp_offset < 128 + 48)
            {
                __stdarg_memcpy(ap->reg_save_area + ap->fp_offset - 8, ap->reg_save_area + ap->fp_offset, 8);
                ap->fp_offset += 16;

                return ap->reg_save_area + ap->fp_offset - 32;
            }
        }
        goto use_overflow_area;

    case __va_stack:
    use_overflow_area:

        ap->overflow_arg_area += size;
        ap->overflow_arg_area = (char*)((long long)(ap->overflow_arg_area + align - 1) & -align);

        return ap->overflow_arg_area - size;

    default: // should never happen
        // abort();
        return 0;
    }
}

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_copy __builtin_va_copy
#define va_end __builtin_va_end

// fix a buggy dependency on GCC in libio.h
typedef va_list __gnuc_va_list;
#define _VA_LIST_DEFINED
