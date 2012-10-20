#include <stddef.h>
#include <string.h>

void __attribute__((noinline)) quicksort(void *base, size_t nel, size_t width, int (*compar)(const void *, const void *))
{
    if (nel < 2)
        return;

    // partition
    size_t pi = nel >> 1; // partition index
    const size_t li = nel - 1; // last index

#define elem(Base, Index) \
    (((char *)Base)[(Index) * width])

#define swap(i0, i1)                                    \
    do {                                                \
        struct { char data[width]; } temp;              \
        memcpy(&temp, &elem(base,i0), width);           \
        memcpy(&elem(base,i0), &elem(base,i1), width);  \
        memcpy(&elem(base,i1), &temp, width);           \
    } while (0)                                         \
    //

    // swap pivot to end
    swap(pi, li);

    size_t si = 0;  // si : store index
    for (size_t i = 0; i < li; i++) {
        if (compar(&elem(base,i), &elem(base,li)) < 0) {
            swap(i, si);
            si++;
        }
    }

    swap(si, li);

    pi = si;

    quicksort(&elem(base,     0), pi - 0 , width, compar);
    quicksort(&elem(base,pi + 1), li - pi, width, compar);
}

int __attribute__((noinline)) cmp(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

#if TEST
#include <stdio.h>

static struct __attribute__((packed)) element {
    int key;
    const char *str;
    char dummy;
} data[] = {
    {   21, "twenty-one", 0 },
    {   34, "thirty-four", 0 },
    {   55, "fifty-five", 0 },
    {  144, "one hundred forty-four", 0 },
    {    1, "one", 0 },
    {    2, "two", 0 },
    {   89, "eighty-nine", 0 },
    {    3, "three", 0 },
    {    5, "five", 0 },
    {    8, "eight", 0 },
    {   13, "thirteen", 0 },
};

int main(int argc, const char *argv[])
{
    size_t count = sizeof data / sizeof data[0];
    quicksort(data, count, sizeof data[0], cmp);
    printf("sizeof data[0] = %zd\n", sizeof data[0]);
    for (int i = 0; i < count; i++)
        printf("%3d = %s\n", data[i].key, data[i].str);

    return 0;
}
#endif

