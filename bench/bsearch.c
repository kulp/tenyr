#include <stddef.h>

void __attribute__((noinline)) *search(const void *key, const void *base, size_t nel, size_t width, int (*compar)(const void *, const void *))
{
    int count = nel >> 1;
    const char *start = (const char*)base, *end = (const char*)base + nel * width;
    while (end > start) {
        const char *ptr = start + count * width;
        int val = compar(key, ptr);
        if (val < 0) {
            count = count >> 1;
            end = ptr;
        } else if (val > 0) {
            count = (count + 1) >> 1;
            start = ptr;
        } else {
            return (void*)ptr;
        }
    }

    return NULL;
}

int __attribute__((noinline)) cmp(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

#if TEST
#include <stdio.h>

static const struct element {
    int key;
    const char *str;
} data[] = {
      1, "one",
      2, "two",
      3, "three",
      5, "five",
      8, "eight",
     13, "thirteen",
     21, "twenty-one",
     34, "thirty-four",
     55, "fifty-five",
     89, "eighty-nine",
    144, "one hundred forty-four",
};

int main(int argc, const char *argv[])
{
    int key = 55;//strtol(argv[1], 0, 0);
    struct element *d = (struct element*)search(&key, data, sizeof data / sizeof data[0], sizeof data[0], cmp);
    if (d)
        puts(d->str);
    else
        puts("error : not found");

    return 0;
}
#endif

