#ifndef ASM_H_
#define ASM_H_

struct format {
    const char *name;
    int (*impl_in )(FILE *, struct instruction *);
    int (*impl_out)(FILE *, struct instruction *);
};

int find_format_by_name(const void *_a, const void *_b);

extern const struct format formats[];
extern const size_t formats_count;
#endif

