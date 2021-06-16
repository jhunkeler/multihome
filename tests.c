#include "multihome.h"

#ifdef ENABLE_TESTING
void test_split() {
    puts("split()");
    char **result;
    size_t result_alloc;

    result = split("one two three", " ", &result_alloc);
    assert(strcmp(result[0], "one") == 0 && strcmp(result[1], "two") == 0 && strcmp(result[2], "three") == 0);
    assert(result_alloc != 0);
    free_array((void *)result, result_alloc);
}

void test_count_substrings() {
    puts("count_substrings()");
    size_t result;
    result = count_substrings("one two three", " ");
    assert(result == 2);
}

void test_mkdirs() {
    puts("mkdirs()");
    int result;
    char *input = "this/is/a/test";

    if (access(input, F_OK) == 0) {
        assert(remove("this/is/a/test") == 0);
        assert(remove("this/is/a") == 0);
        assert(remove("this/is") == 0);
        assert(remove("this") == 0);
    }

    result = mkdirs(input);
    assert(result == 0);
    assert(access(input, F_OK) == 0);
}

void test_shell() {
    puts("shell()");
    assert(shell((char *[]){"/bin/echo", "testing", NULL}) == 0);
    assert(shell((char *[]){"/bin/date", NULL}) == 0);
    assert(shell((char *[]){"/bin/unlikelyToExistAnywhere", NULL}) != 0);
}

void test_touch() {
    puts("touch()");
    char *input = "touched_file.txt";

    if (access(input, F_OK) == 0) {
        remove(input);
    }

    assert(touch(input) == 0);
    assert(access(input, F_OK) == 0);
}

void test_strip_domainname() {
    puts("strip_domainname()");
    char *input = strdup("subdomain.domain.tld");
    char *truth = "subdomain";
    char *result;

    result = strip_domainname(input);
    assert(strcmp(result, truth) == 0);
}

void test_main() {
    test_count_substrings();
    test_split();
    test_mkdirs();
    test_shell();
    test_touch();
    test_strip_domainname();
    exit(0);
}
#endif
