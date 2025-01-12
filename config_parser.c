#include <stdio.h>
#include <string.h>

#define MAX_LINES 10000
#define BUFFER_SIZE 5000

typedef struct
{
  char indexpath[MAX_LINES];
  char articlepath[MAX_LINES];
  char articlecompiledpath[MAX_LINES];
  char rsspath[MAX_LINES];
  char rsscompiledpath[MAX_LINES];
  char dbpath[MAX_LINES];
  char websitelink[MAX_LINES];

} Config;

Config config;

void
trim_newline(char* str)
{
  size_t len = strlen(str);
  if (len > 0 && str[len - 1] == '\n') {
    str[len - 1] = '\0';
  }
}

int
parse_config(const char* filename, Config* config)
{
  FILE* openConfigFile = fopen(filename, "r");
  if (!openConfigFile) {
    // perror("Error opening file");
    return 1;
  }

  char line[MAX_LINES];

  while (fgets(line, sizeof(line), openConfigFile)) {
    trim_newline(line);

    char* key = strtok(line, " =");
    char* value = strtok(NULL, " = ");

    if (key && value) {
      if (strcmp(key, "indexpath") == 0) {
        strcpy(config->indexpath, value);

      } else if (strcmp(key, "articlepath") == 0) {
        strcpy(config->articlepath, value);

      } else if (strcmp(key, "articlecompiledpath") == 0) {
        strcpy(config->articlecompiledpath, value);

      } else if (strcmp(key, "rsspath") == 0) {
        strcpy(config->rsspath, value);

      } else if (strcmp(key, "rsscompiledpath") == 0) {
        strcpy(config->rsscompiledpath, value);

      } else if (strcmp(key, "dbpath") == 0) {
        strcpy(config->dbpath, value);

      } else if (strcmp(key, "websitelink") == 0) {
        strcpy(config->websitelink, value);
      }
    }
  }

  fclose(openConfigFile);

  return 0;
}
