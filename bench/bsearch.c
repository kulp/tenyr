#include <stddef.h>

void __attribute__((noinline)) *search(const void *key, const void *base, size_t count, size_t width, int (*compar)(const void *, const void *))
{
    while (count) {
        count >>= 1;
        const char *ptr = base + count * width;
        int val = compar(key, ptr);
        if (val == 0)
            return (void*)ptr;
        else if (val > 0)
            base = ptr + width;
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
    for (int key = 0; key < 150; key++) {
        struct element *d = (struct element*)search(&key, data, sizeof data / sizeof data[0], sizeof data[0], cmp);
        if (d)
            printf("%d is %s ---\n", key, d->str);
        else
            printf("%d not found\n", key);
    }

    return 0;
}
#endif

