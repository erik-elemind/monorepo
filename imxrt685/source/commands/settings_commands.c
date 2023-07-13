#include "loglevels.h"
#include "settings.h"

static const char* TAG = "settings_commands";

void settings_get_command(int argc, char **argv) {
  char str[32];
  int result;

  if (argc == 1) {
    // No key given. Print them all.
    for (unsigned i=0; true; i++) {
      result = settings_get_next_key(i, str, sizeof(str));
      if (result) {
        // Reached end of list, done.
        return;
      }

      // Print the key
      printf("%s = ", str);

      // Read the value as a string.
      // Note we are using the same buffer for key and value here.
      // this has been tested in the test code.
      result = settings_get_string(str, str, sizeof(str));
      if (result) {
        LOGE(TAG, "read failed for key %s\n", str);
        continue;
      }

      // Print the value
      printf("%s\n", str);
    }
  }
  else if (argc == 2) {
    if (0 == settings_get_string(argv[1], str, sizeof(str))) {
      printf("%s = %s\n", argv[1], str);
    }
    else {
      printf("no setting found: %s\n", argv[1]);
    }
  }
  else {
    printf("invalid num of args. expecting 0 or 1\n");
  }
}

void settings_set_command(int argc, char **argv) {
  if (argc != 3) {
    LOGE(TAG, "invalid num of args. expecting 2\n");
    return;
  }

  // Set the value as a string regardless of the input type.
  // The settings library does the conversion for us upon reading back
  (void)settings_set_string(argv[1], argv[2]);
}

void settings_delete_command(int argc, char **argv) {
  if (argc != 2) {
    LOGE(TAG, "invalid num of args. expecting 1\n");
    return;
  }

  (void)settings_delete(argv[1]);
}
