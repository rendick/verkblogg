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
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "./config_parser.c"
#include "./utilities.h"

#define COLOR_BOLD "\e[1m"
#define COLOR_OFF "\e[m"

#define MAX_LINES 10000
#define MAX_ARTICLES 10000
#define BUFFER_SIZE 5000
#define MAX_HTML_SIZE 65536
#define MAX_SIZE 1024 * 1024

#define COLOR_BOLD "\e[1m"
#define COLOR_OFF "\e[m"

Config config;

int
generate_rss();
int
write_index();

int article_count = 0;

int
create_post()
{
  char title[256], title_tl[256];
  char main_content[BUFFER_SIZE];

  input("Enter the title", title, 256);
  input("Enter the main text", main_content, 5000);

  for (int i = 0; i < (int)sizeof(title) - 1 && title[i] != '\0'; i++) {
    title_tl[i] = tolower(title[i]);
  }
  title_tl[sizeof(title_tl) - 1] = '\0';

  replaceAll(title_tl, " ", "-");

  char* forbidden_path_symbols[13] = { "!", ",", ".", ";", ":", "#", "^",
                                       "/", "*", "(", ")", "<", ">" };
  char* forbidden_main_symbols[1] = { ";" };

  for (int i = 0; i < (int)sizeof(forbidden_path_symbols) /
                        (int)sizeof(forbidden_path_symbols[0]);
       i++) {
    replaceAll(title_tl, forbidden_path_symbols[i], "");
  }

  for (int i = 0; i < (int)sizeof(forbidden_main_symbols) /
                        (int)sizeof(forbidden_main_symbols[0]);
       i++) {
    replaceAll(main_content, forbidden_main_symbols[i], "");
  }

  FILE* wttf;
  if (openFile(&wttf, config.dbpath, "a") != 0) {
    return 1;
  }

  char dateBuffer[11];
  currentTime(dateBuffer, sizeof(dateBuffer));

  fprintf(wttf, "%s;%s;%s;%s\n", title, main_content, title_tl, dateBuffer);
  fclose(wttf);

  return 0;
}

typedef struct
{
  char* title;
  char* main;
  char* path;
  char* date;
} Article;

Article articles[MAX_LINES];

void
parse_database()
{
  FILE* open_db = fopen(config.dbpath, "r");
  if (!open_db) {
    perror("Error opening DB");
    exit(1);
  }

  char dbfile[1024];

  while (fgets(dbfile, sizeof(dbfile), open_db)) {
    if (article_count >= MAX_LINES) {
      printf("Database has too many entries!\n");
      break;
    }

    char* tokens[4] = { NULL };
    char* token = strtok(dbfile, ";\n");

    for (int i = 0; token && i < 4; i++) {
      if (!token) {
        tokens[i] = strdup("");
      } else {
        tokens[i] = strdup(token);
        token = strtok(NULL, ";\n");
      }
    }

    articles[article_count].title = tokens[0];
    articles[article_count].main = tokens[1];
    articles[article_count].path = tokens[2];
    articles[article_count].date = tokens[3];

    printf("%s%d%s: %s | %s | %s | %s\n",
           COLOR_BOLD,
           article_count,
           COLOR_OFF,
           tokens[0] ? tokens[0] : "NULL",
           tokens[1] ? tokens[1] : "NULL",
           tokens[2] ? tokens[2] : "NULL",
           tokens[3] ? tokens[3] : "NULL");

    article_count++;
  }

  fclose(open_db);
}

int
write_index()
{
  char* list = malloc(MAX_SIZE);
  if (!list) {
    printf("Memory allocation failed.");
    return 1;
  }

  list[0] = '\0';

  for (int i = 0; i < article_count; i++) {
    char item[1024];
    snprintf(item,
             sizeof(item),
             "<li><i>%s</i><br><a href='./%s/%s.html'>%s</a></li>\n",
             articles[i].date,
             config.articlecompiledpath,
             articles[i].path,
             articles[i].main);

    if (strlen(list) + strlen(item) >= MAX_SIZE) {
      printf("List size exceeded.\n");
      free(list);
      return 1;
    }
    strcat(list, item);
  }

  FILE* read_template_file = fopen(config.indexpath, "r");
  if (read_template_file == NULL) {
    printf("File can't be opened.\n");
    free(list);
    return 1;
  }

  FILE* write_new_file = fopen("index.html", "w");

  char line[MAX_HTML_SIZE];
  while (fgets(line, sizeof(line), read_template_file) != NULL) {
    replaceAll(line, "${indexTitle}", list);

    fprintf(write_new_file, "%s", line);
  }

  fclose(write_new_file);
  fclose(read_template_file);

  return 0;
}

void write_articles() {
  if (access("articles", F_OK) != 0) {
    if (mkdir("articles", 0777) != 0) {
      perror("Failed creating directory.");
      return;
    }
  }

  for (int i = 0; i < article_count; i++) {
    char article_path[1024];
    snprintf(article_path,
             sizeof(article_path),
             "%s/%s.html",
             config.articlecompiledpath,
             articles[i].path);

    if (access(article_path, F_OK) == 0) {
      char updated_article_path[1024];
      snprintf(updated_article_path,
               sizeof(updated_article_path),
               "%s/%s-COPY.html",
               config.articlecompiledpath,
               articles[i].path);
      strcpy(article_path, updated_article_path);
    }

    FILE* read_template_file = fopen(config.articlepath, "r");

    FILE* write_article_template = fopen(article_path, "w");

    char line[MAX_HTML_SIZE];
    while (fgets(line, sizeof(line), read_template_file) != NULL) {
      replaceAll(line, "${articleTitle}", articles[i].title);
      replaceAll(line, "${main}", articles[i].main);
      fprintf(write_article_template, "%s", line);
    }

    fclose(read_template_file);

    fclose(write_article_template);
  }
}

int
generate_rss()
{
  if (access("rss", 0) != 0) {
    mkdir("rss", 0777);
  }

  char* list = malloc(MAX_SIZE);
  if (!list) {
    printf("Memory allocation failed.");
    return 1;
  }

  list[0] = '\0';

  for (int i = 0; i < article_count; i++) {
    char item[1024];
    snprintf(
      item,
      sizeof(item),
      "<item>\n<title>%s</title>\n<link>%s/%s/%s.html</link>\n<description>%s</"
      "description>\n</item>\n",
      articles[i].title,
      config.websitelink,
      config.articlecompiledpath,
      articles[i].path,
      articles[i].main);

    if (strlen(list) + strlen(item) >= MAX_SIZE) {
      printf("List size exceeded.\n");
      free(list);
      return 1;
    }
    strcat(list, item);
  }

  FILE* read_template_file = fopen(config.rsspath, "r");
  if (read_template_file == NULL) {
    printf("File can't be opened.\n");
    free(list);
    return 1;
  }

  char rssfullpath[1000];
  snprintf(
    rssfullpath, sizeof(rssfullpath), "%s/blog.rss", config.rsscompiledpath);
  FILE* write_new_file = fopen(rssfullpath, "w");

  char line[MAX_HTML_SIZE];
  while (fgets(line, sizeof(line), read_template_file) != NULL) {
    replaceAll(line, "${items}", list);

    fprintf(write_new_file, "%s", line);
  }

  fclose(write_new_file);
  fclose(read_template_file);

  return 0;
}

void
generate_config()
{
  if (access("verkblogg.conf", 0) == 0) {
    printf("Configuration file exists! Exiting...\n");
    exit(1);
  }

  FILE* generate_config_file;
  if (openFile(&generate_config_file, "verkblogg.conf", "a") != 0) {
    exit(1);
  }

  fprintf(generate_config_file,
          "indexpath = public/index.html\narticlepath = "
          "public/article.html\narticlecompiledpath = articles \nrsspath = "
          "public/template.rss\nrsscompiledpath = rss\ndbpath = "
          "verkblogg.db\nwebsitelink = https://www.example.org");

  fclose(generate_config_file);

  printf("The configuration file was successfully generated.\nDo not forget to "
         "fill it with the needed information!\n");
}

int
main(int argc, char* argv[])
{
  if (parse_config("verkblogg.conf", &config) != 0 &&
      strcmp(argv[1], "config") && strcmp(argv[1], "help") &&
      strcmp(argv[1], "version")) {
    printf("Configuraion file does not exist!\nTry 'verkblogg -h for more "
           "information\n");
    exit(1);
  }

  if (argc < 2) {
    printf("Usage: %s [OPTION]\nTry 'help' for more information.\n", argv[0]);
    return 1;
  }

  if (!strcmp(argv[1], "create")) {
    create_post();
    parse_database();
    write_index();
    write_articles();
    generate_rss();
  } else if (!strcmp(argv[1], "check")) {
    parse_database();
  } else if (!strcmp(argv[1], "config")) {
    generate_config();
  } else if (!strcmp(argv[1], "rss")) {
    parse_database();
    generate_rss();
  } else if (!strcmp(argv[1], "help")) {
    printf("Usage: verkblogg [OPTION]\n"
           "create\tfor creating articles\n"
           "check\tfor viewing DB\n"
           "config\tfor generating configuration file\n"
           "rss\tfor updating RSS file\n\n"
           "help\tdisplay help menu\n"
           "version\tdisplay current version\n");

  } else if (!strcmp(argv[1], "version")) {
    printf("verkblogg v1.1.2\n");
  } else {
    printf("verkblogg: %s: Invalid argument.\n", argv[1]);
  }

  return 0;
}
