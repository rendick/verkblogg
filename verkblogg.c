//	verkblogg is a utility for optimizing blogging
//	Copyright (C) 2025 Constantin AKA rendick.

//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.

//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.

//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./utilities.h"

#define COLOR_BOLD "\e[1m"
#define COLOR_OFF "\e[m"

#define MAX_LINE 512

int createPost();
int parsePostsDb();
int updateIndexFile();
int updatePostsFiles();
int updateRssFile();

// Configuration file parser

typedef struct {
  char indexpath[MAX_LINE];
  char articlepath[MAX_LINE];
  char compiledpath[MAX_LINE];
  char dbpath[MAX_LINE];
}
Config;

Config config;

void trim_newline(char * str) {
  size_t len = strlen(str);
  if (len > 0 && str[len - 1] == '\n') {
    str[len - 1] = '\0';
  }
}

int parse_config(const char * filename, Config * config) {
  FILE * openConfigFile = fopen(filename, "r");
  if (!openConfigFile) {
    // perror("Error opening file");
    return 1;
  }

  char line[MAX_LINE];

  while (fgets(line, sizeof(line), openConfigFile)) {
    trim_newline(line);

    char * key = strtok(line, " =");
    char * value = strtok(NULL, " = ");

    if (key && value) {
      if (strcmp(key, "indexpath") == 0) {
        strcpy(config -> indexpath, value);
      } else if (strcmp(key, "articlepath") == 0) {
        strcpy(config -> articlepath, value);
      } else if (strcmp(key, "compiledpath") == 0) {
        strcpy(config -> compiledpath, value);
      } else if (strcmp(key, "dbpath") == 0) {
        strcpy(config -> dbpath, value);
      }
    }
  }

  fclose(openConfigFile);

  return 0;
}

// Article organization

int createPost() {
  char title[30], title_tl[30];
  char main_content[100];

  input("Enter the title", title, 30);
  input("Enter the main text", main_content, 100);

  for (int i = 0; i < (int) sizeof(title) - 1 && title[i] != '\0'; i++) {
    title_tl[i] = tolower(title[i]);
  }
  title_tl[sizeof(title_tl) - 1] = '\0';

  replaceAll(title_tl, " ", "-");

  char * forbiddenPathSymbols[13] = {
    "!",
    ",",
    ".",
    ";",
    ":",
    "#",
    "^",
    "/",
    "*",
    "(",
    ")",
    "<",
    ">"
  };

  for (int i = 0; i < sizeof(forbiddenPathSymbols) / sizeof(forbiddenPathSymbols[0]); i++) {
    replaceAll(title_tl, forbiddenPathSymbols[i], "");
  }

  FILE * wttf;
  if (openFile( & wttf, config.dbpath, "a") != 0) {
    return 1;
  }

  char dateBuffer[11];
  currentTime(dateBuffer, sizeof(dateBuffer));

  fprintf(wttf, "%s;%s;%s;%s\n", title, main_content, title_tl, dateBuffer);
  fclose(wttf);

  parsePostsDb();

  return 0;
}

int parsePostsDb() {
  if (access(config.dbpath, 0) != 0) {
    printf("DB file is missing. Re-run command to create it.\n");
    exit(1);
  }

  FILE * pfpn;
  if (openFile( & pfpn, config.dbpath, "r") != 0) {
    return 1;
  }

  char buffer[1024];
  char * lines[300] = {
    NULL
  };

  int x = 0;
  while (fgets(buffer, sizeof(buffer), pfpn)) {
    lines[x] = strdup(buffer);
    x++;
  }
  fclose(pfpn);

  char allPosts[5000] = "";

  for (int i = 0; i < x; i++) {
    replaceAll(lines[i], "\n", "");

    char * splitted_title = NULL;
    char * splitted_main = NULL;
    char * splitted_date = NULL;
    char * splitted_path = NULL;

    char * splitDb = strtok(lines[i], ";");
    int idx = 0;

    while (splitDb != NULL) {
      if (idx == 0) {
        splitted_title = strdup(splitDb);
      } else if (idx == 1) {
        splitted_main = strdup(splitDb);
      } else if (idx == 2) {
        splitted_path = strdup(splitDb);
      } else if (idx == 3) {
        splitted_date = strdup(splitDb);
      }
      splitDb = strtok(NULL, ";");
      idx++;
    }

    if (splitted_title && splitted_path) {
      char articleInformation[1024];
      snprintf(articleInformation, sizeof(articleInformation),
        "<li><i>%s</i><br><a href='./%s/%s.html'>%s</a></li>\n",
        splitted_date, config.compiledpath, splitted_path, splitted_title);
      strncat(allPosts, articleInformation,
        sizeof(allPosts) - strlen(allPosts) - 1);
    }

    free(splitted_title);
    free(splitted_main);
    free(splitted_path);
    free(splitted_date);
    free(lines[i]);
  }

  return updateIndexFile(allPosts);
}

int updateIndexFile(char * info) {
  char rssContent[5000];
  char updatedRssContent[5000] = "";

  FILE * openIndexFile;
  if (openFile( & openIndexFile, config.indexpath, "r") != 0) {
    return 1;
  }

  while (fgets(rssContent, 1000, openIndexFile)) {
    replaceAll(rssContent, "${indexTitle}", info);
    strncat(updatedRssContent, rssContent,
      sizeof(updatedRssContent) - strlen(updatedRssContent) - 1);
  }

  fclose(openIndexFile);

  FILE * writeUpdatedIndexFile;
  if (openFile( & writeUpdatedIndexFile, "index.html", "w") != 0) {
    return 1;
  }

  fprintf(writeUpdatedIndexFile, "%s", updatedRssContent);

  fclose(writeUpdatedIndexFile);

  return updatePostsFiles();
}

int updatePostsFiles() {
  DIR * openPostDir = opendir(config.compiledpath);
  if (!(openPostDir)) {
    mkdir(config.compiledpath, 0777);
  }

  FILE * pfpn;
  if (openFile( & pfpn, config.dbpath, "r") != 0) {
    return 1;
  }

  char buffer[256];
  char * lines[100] = {
    NULL
  };

  int x = 0;
  while (fgets(buffer, sizeof(buffer), pfpn)) {
    lines[x] = strdup(buffer);
    x++;
  }
  fclose(pfpn);

  for (int i = 0; i < x; i++) {
    replaceAll(lines[i], "\n", "");

    char * splitted_title = NULL;
    char * splitted_main = NULL;
    char * splitted_path = NULL;
    char * splitted_date = NULL;

    char * splitDb = strtok(lines[i], ";");
    int idx = 0;

    while (splitDb != NULL) {
      if (idx == 0) {
        splitted_title = strdup(splitDb);
      } else if (idx == 1) {
        splitted_main = strdup(splitDb);
      } else if (idx == 2) {
        splitted_path = strdup(splitDb);
      } else if (idx == 3) {
        splitted_date = strdup(splitDb);
      }
      splitDb = strtok(NULL, ";");
      idx++;
    }

    if (splitted_title && splitted_path && splitted_main && splitted_date) {
      FILE * tpl;
      if (openFile( & tpl, config.articlepath, "r") != 0) {
        return 1;
      }

      char templateContent[5000] = "";
      char line[256];
      while (fgets(line, sizeof(line), tpl)) {
        strncat(templateContent, line,
          sizeof(templateContent) - strlen(templateContent) - 1);
      }
      fclose(tpl);

      replaceAll(templateContent, "${indexTitle}", splitted_title);
      replaceAll(templateContent, "${articleTitle}", splitted_title);
      replaceAll(templateContent, "${articleTitle}", splitted_title);
      replaceAll(templateContent, "${main}", splitted_main);
      replaceAll(templateContent, "${date}", splitted_date);

      char filePath[1024];
      snprintf(filePath, sizeof(filePath), "%s/%s.html",
        config.compiledpath, splitted_path);

      FILE * postFile;
      if (openFile( & postFile, filePath, "w") != 0) {
        return 1;
      }

      fprintf(postFile, "%s", templateContent);
      fclose(postFile);
    }

    free(splitted_title);
    free(splitted_main);
    free(splitted_path);
    free(lines[i]);
  }

  return 0;
}

int checkPosts() {
  if (access(config.dbpath, 0) != 0) {
    printf("DB file is missing.\n");
    exit(1);
  }

  FILE * openPostsFile;
  if (openFile( & openPostsFile, config.dbpath, "r") != 0) {
    return 1;
  }

  char buffer[256];
  char * lines[100] = {
    NULL
  };

  int x = 0;
  while (fgets(buffer, sizeof(buffer), openPostsFile)) {
    lines[x] = strdup(buffer);
    x++;
  }

  fclose(openPostsFile);

  for (int i = 0; i < x; i++) {
    replaceAll(lines[i], "\n", "");

    char * splitted_title = NULL;
    char * splitted_main = NULL;
    char * splitted_path = NULL;
    char * splitted_date = NULL;

    int idx = 0;

    char * token = strtok(lines[i], ";");

    while (token != NULL) {
      if (idx == 0) {
        splitted_title = strdup(token);
      } else if (idx == 1) {
        splitted_main = strdup(token);
      } else if (idx == 2) {
        splitted_path = strdup(token);
      } else if (idx == 3) {
        splitted_date = strdup(token);
      }

      token = strtok(NULL, ";");
      idx++;
    }

    printf("%d. %s%s%s - %s - %s/%s.html - %s\n", i + 1, COLOR_BOLD,
      splitted_title, COLOR_OFF, splitted_main, config.compiledpath,
      splitted_path, splitted_date);
  }

  return 0;
}

int generateConfigFile() {
  if (access("verkblogg.conf", 0) == 0) {
    printf("Configuration file exists! Exiting...\n");
    exit(1);
  }

  FILE * configFile;
  if (openFile( & configFile, "verkblogg.conf", "a") != 0) {
    return 1;
  }

  fprintf(configFile,
    "indexpath = public/index.html\narticlepath = "
    "public/article.html\ncompiledpath = articles\ndbpath = "
    "verkblogg.db\n");
  fclose(configFile);

  printf(
    "The configuration file was successfully generated.\nDo not forget to "
    "fill it with the needed information!\n");

  exit(0);
}

int rssGenerator() {
  if (access(config.dbpath, 0) != 0) {
    printf("DB file is missing.\n");
    exit(1);
  }

  DIR * openRssDir = opendir("rss");
  if (!(openRssDir)) {
    mkdir("rss", 0777);
  }

  FILE * openPostsFile;
  if (openFile( & openPostsFile, config.dbpath, "r") != 0) {
    return 1;
  }

  char buffer[256];
  char * lines[100] = {
    NULL
  };

  int x = 0;
  while (fgets(buffer, sizeof(buffer), openPostsFile)) {
    lines[x] = strdup(buffer);
    x++;
  }

  fclose(openPostsFile);

  char rssItems[5000] = "";

  for (int i = 0; i < x; i++) {
    replaceAll(lines[i], "\n", "");

    char * splitted_title = NULL;
    char * splitted_main = NULL;
    char * splitted_path = NULL;
    char * splitted_date = NULL;

    int idx = 0;

    char * token = strtok(lines[i], ";");

    while (token != NULL) {
      if (idx == 0) {
        splitted_title = strdup(token);
      } else if (idx == 1) {
        splitted_main = strdup(token);
      } else if (idx == 2) {
        splitted_path = strdup(token);
      } else if (idx == 3) {
        splitted_date = strdup(token);
      }

      token = strtok(NULL, ";");
      idx++;
    }

    if (splitted_title && splitted_main && splitted_path && splitted_date) {
      char rssInformation[1024];
      snprintf(rssInformation, sizeof(rssInformation), "<item>\n\t<title>%s</title>\n\t<description>%s</description>\n\t<link>%s</link>\n\t<pubDate>%s</pubDate>\n</item>\n", splitted_title, splitted_main, splitted_path, splitted_date); // Title - Main - Link - Dtae
      strncat(rssItems, rssInformation, sizeof(rssItems) - strlen(rssItems) - 1);
    }

    free(splitted_title);
    free(splitted_main);
    free(splitted_path);
    free(splitted_date);
    free(lines[i]);

  }

  return updateRssFile(rssItems);
}

int updateRssFile(char * info) {
  char rssContent[5000];
  char updatedRssContent[5000] = "";

  FILE * openIndexFile;
  if (openFile( & openIndexFile, "public/template.rss", "r") != 0) {
    return 1;
  }

  while (fgets(rssContent, 1000, openIndexFile)) {
    replaceAll(rssContent, "${items}", info);
    strncat(updatedRssContent, rssContent,
      sizeof(updatedRssContent) - strlen(updatedRssContent) - 1);
  }

  fclose(openIndexFile);

  FILE * writeUpdatedIndexFile;
  if (openFile( & writeUpdatedIndexFile, "rss/articles.rss", "w") != 0) {
    return 1;
  }

  fprintf(writeUpdatedIndexFile, "%s", updatedRssContent);

  fclose(writeUpdatedIndexFile);

  return 0;

}

int main(int argc, char * argv[]) {
  if (parse_config("verkblogg.conf", & config) != 0 && strcmp(argv[1], "-c") &&
    strcmp(argv[1], "-h") && strcmp(argv[1], "-v")) {
    printf(
      "Configuraion file does not exist!\nTry 'verkblogg -h for more "
      "information\n");
    exit(1);
  }

  if (argc < 2) {
    printf("Usage: %s -n|-p|-c|-h|-v\nTry '-h' for more information.\n",
      argv[0]);
    return 1;
  }

  if (!strcmp(argv[1], "-n")) {
    createPost();
  } else if (!strcmp(argv[1], "-p")) {
    checkPosts();
  } else if (!strcmp(argv[1], "-c")) {
    generateConfigFile();
  } else if (!strcmp(argv[1], "-u")) {
    parsePostsDb();
  } else if (!strcmp(argv[1], "-r")) {
    rssGenerator();
  } else if (!strcmp(argv[1], "-h")) {
    printf(
      "Usage: verkblogg [OPTION]\n"
      "  -n\tfor creating articles\n"
      "  -p\tfor viewing DB\n"
      "  -c\tfor generating configuration file\n"
      "  -r\tfor updating RSS file\n\n"
      "  -h\tdisplay help menu\n"
      "  -v\tdisplay current version\n");

  } else if (!strcmp(argv[1], "-v")) {
    printf("verkblogg v1.1.2\n");
  } else {
    printf("verkblogg: %s: Invalid argument.\n", argv[1]);
  }

  return 0;
}
