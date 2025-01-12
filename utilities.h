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

#pragma once

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int
replaceAll(char* source, char* textToReplace, char* replacementText)
{
  char* substring_source = strstr(source, textToReplace);

  if (substring_source == NULL) {
    return 0;
  }

  memmove(substring_source + strlen(replacementText),
          substring_source + strlen(textToReplace),
          strlen(substring_source) - strlen(textToReplace) + 1);

  memcpy(substring_source, replacementText, strlen(replacementText));

  return 0;
}

int
split(char* source, char* symbol)
{
  char* token = strtok(source, symbol);

  while (token != NULL) {
    printf("%s \n", token);
    token = strtok(NULL, "\n");
  }

  return 0;
}

int
input(char* title, char* var, int amount)
{
  printf("%s: ", title);
  if (fgets(var, amount, stdin) != NULL) {
    var[strcspn(var, "\n")] = '\0';
    printf("%s\n", var);
  }
  return 0;
}

int
openFile(FILE** file, char* path, char* mode)
{
  *file = fopen(path, mode);
  if (file == NULL) {
    return 1;
  }
  return 0;
}

int
currentTime(char* buffer, size_t size)
{
  time_t t = time(NULL);
  struct tm tm;
  localtime_r(&t, &tm);
  snprintf(
    buffer, size, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

  return 0;
}
