#include "settings.h"

#include <assert.h>
#include <string.h>

// Callback for use with ini_browse()
int test_cb(const mTCHAR *Section, const mTCHAR *Key, const mTCHAR *Value, void *UserData)
{
    printf("found: [%s] = %s\n", Key, Value);
    // return 1 to continue processing, or 0 to stop
    return 1;
}

// Get number of settings
int get_count(void)
{
    int result;

    // We don't care about the actual key here, just that one exists.
    // 2 seems to be the minimum number of bytes required by the library.
    char key[2];

    for (unsigned i=0; true; i++)
    {
        if (ini_getkey(SETTINGS_SECTION, i, key, sizeof(key), SETTINGS_FILE))
        {
            // found an entry, keep going
        }
        else
        {
            // reached the end, return the count
            return i;
        }
    }

    return 0;
}

int main(void)
{
    long test_long;
    float test_float;
    char test_buf[32];
    int result;
    unsigned num = 0;

    assert(0 == get_count());

    // Create a long
    result = settings_set_long("test long", 11223344);
    assert(0 == result);
    num++;
    assert(num == get_count());
    result = settings_get_long("test long", &test_long);
    assert(0 == result);
    assert(11223344 == test_long);

    // Delete an entry
    result = settings_delete("test long");
    assert(0 == result);
    num--;
    assert(num == get_count());
    result = settings_get_long("test long", &test_long);
    assert(0 != result);

    // Create a float
    result = settings_set_float("test float", 3.14f);
    assert(0 == result);
    num++;
    result = settings_get_float("test float", &test_float);
    assert(0 == result);
    assert(test_float == 3.14f);
    assert(num == get_count());

    // Create a string
    result = settings_set_string("test string", "super awesome jelly beans");
    assert(0 == result);
    num++;
    assert(num == get_count());

    // Check if we can use the same buffer for key and value
    sprintf(test_buf, "test string");
    result = settings_get_string(test_buf, test_buf, sizeof(test_buf));
    assert(0 == result);
    assert(0 == strcmp(test_buf, "super awesome jelly beans"));

    // A float can actually be created as a string
    result = settings_set_string("test-actually-a-float", "4.321");
    assert(0 == result);
    num++;
    assert(num == get_count());
    result = settings_get_float("test-actually-a-float", &test_float);
    assert(0 == result);
    assert(test_float == 4.321f);

    // Same for longs
    result = settings_set_string("test-actually-a-long", "42");
    assert(0 == result);
    num++;
    assert(num == get_count());
    result = settings_get_long("test-actually-a-long", &test_long);
    assert(0 == result);
    assert(test_long == 42);

    // If we made it this far, things are good.
    printf("test passed.\n");

    return 0;
}