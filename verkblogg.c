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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./utilities.h"
#include "./config.h"

#define COLOR_BOLD  "\e[1m"
#define COLOR_OFF   "\e[m"

int createPost();
int parsePostsDb();
int updateIndexFile();
int updatePostsFiles();

int createPost() {
    char title[30], title_tl[30];
    char main_content[100];

    input("Title", title, 30);
    input("Main", main_content, 100);

	for (int i = 0; i < sizeof(title) - 1 && title[i] != '\0'; i++){
		title_tl[i] = tolower(title[i]);
	}

    FILE *wttf;
    if (openFile(&wttf, "posts", "a") != 0) {
        return 1;
    }

    fprintf(wttf, "%s;%s;%s\n", title, main_content, title_tl);
    fclose(wttf);

	parsePostsDb();

    return 0;
}

int parsePostsDb() {
    FILE *pfpn;
    if (openFile(&pfpn, "posts", "r") != 0) {
        return 1;
    }

    char buffer[256]; 
    char *lines[100] = {NULL};

    int x = 0;
    while (fgets(buffer, sizeof(buffer), pfpn)) {
        lines[x] = strdup(buffer);
        x++;
    }
    fclose(pfpn);

    char allPosts[5000] = ""; 

    for (int i = 0; i < x; i++) {
        replaceAll(lines[i], "\n", ""); 

        char *splitted_title = NULL;
        char *splitted_main = NULL;
        char *splitted_path = NULL;

        char *splitDb = strtok(lines[i], ";");
        int idx = 0;

        while (splitDb != NULL) {
            if (idx == 0) {
                splitted_title = strdup(splitDb);
            } else if (idx == 1) {
                splitted_main = strdup(splitDb);
            } else if (idx == 2) {
                splitted_path = strdup(splitDb);
            }
            splitDb = strtok(NULL, ";");
            idx++;
        }

        if (splitted_title && splitted_path) {
            char postInformation[256];
            snprintf(postInformation, sizeof(postInformation), "<li><a href='./post/%s.html'>%s</a></li>\n", splitted_path, splitted_title);
            strncat(allPosts, postInformation, sizeof(allPosts) - strlen(allPosts) - 1);
        }

        free(splitted_title);
        free(splitted_main);
        free(splitted_path);
        free(lines[i]);
    }

    return updateIndexFile(allPosts);
}

int updateIndexFile(char *info) {
	char indexContent[5000];
	char updatedIndexContent[5000] = "";
	FILE *openIndexFile;
	if (openFile(&openIndexFile, indexpath, "r") != 0) {
		return 1;
	}

	while(fgets(indexContent, 1000, openIndexFile)) {
		replaceAll(indexContent, "${posts}", info);
		strncat(updatedIndexContent, indexContent, sizeof(updatedIndexContent) - strlen(updatedIndexContent) - 1);
	}
	
	fclose(openIndexFile);

	FILE *writeUpdatedIndexFile;
	if (openFile(&writeUpdatedIndexFile, "index.html", "w") != 0) {
		return 1;
	}

	fprintf(writeUpdatedIndexFile, "%s", updatedIndexContent);

	fclose(writeUpdatedIndexFile);

    return updatePostsFiles();
}

int updatePostsFiles() {
    FILE *pfpn;
    if (openFile(&pfpn, "posts", "r") != 0) {
        return 1;
    }

    char buffer[256]; 
    char *lines[100] = {NULL};

    int x = 0;
    while (fgets(buffer, sizeof(buffer), pfpn)) {
        lines[x] = strdup(buffer);
        x++;
    }
    fclose(pfpn);

    for (int i = 0; i < x; i++) {
        replaceAll(lines[i], "\n", ""); 

        char *splitted_title = NULL;
        char *splitted_main = NULL;
        char *splitted_path = NULL;

        char *splitDb = strtok(lines[i], ";");
        int idx = 0;

        while (splitDb != NULL) {
            if (idx == 0) {
                splitted_title = strdup(splitDb);
            } else if (idx == 1) {
                splitted_main = strdup(splitDb);
            } else if (idx == 2) {
                splitted_path = strdup(splitDb);
            }
            splitDb = strtok(NULL, ";");
            idx++;
        }

        if (splitted_title && splitted_path && splitted_main) {
            FILE *tpl;
            if (openFile(&tpl, postpath, "r") != 0) {
                return 1;
            }

            char templateContent[5000] = "";
            char line[256];
            while (fgets(line, sizeof(line), tpl)) {
                strncat(templateContent, line, sizeof(templateContent) - strlen(templateContent) - 1);
            }
            fclose(tpl);

            replaceAll(templateContent, "${title}", splitted_title);
            replaceAll(templateContent, "${myTitle}", splitted_title);
            replaceAll(templateContent, "${post}", splitted_main);

            char filePath[256];
            snprintf(filePath, sizeof(filePath), "post/%s.html", splitted_path);

            FILE *postFile;
            if (openFile(&postFile, filePath, "w") != 0) {
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

int checkPosts(){
	FILE *openPostsFile;
	if (openFile(&openPostsFile, "posts", "r") != 0) {
		return 1;
	}

    char buffer[256]; 
    char *lines[100] = {NULL};

    int x = 0;
    while (fgets(buffer, sizeof(buffer), openPostsFile)) {
        lines[x] = strdup(buffer);
        x++;
    }

    fclose(openPostsFile);

	for (int i = 0; i < x; i++){
		replaceAll(lines[i], "\n", "");

		char *splitted_title = NULL;
		char *splitted_main  = NULL;
		char *splitted_path  = NULL;

		int idx = 0;

		char *token = strtok(lines[i], ";");

		while (token != NULL) {
			if (idx == 0) {
				splitted_title = strdup(token);
			} else if (idx == 1) {
				splitted_main = strdup(token);
			}else if (idx == 2){
				splitted_path= strdup(token);
			}

			token = strtok(NULL, ";");
			idx++;
		}

		printf("%d. %s%s%s - %s - posts/%s.html\n", i, COLOR_BOLD,splitted_title, COLOR_OFF, splitted_main, splitted_path);

	}

	return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s -n|-p\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], "-n")) {
        createPost();
    } else if (!strcmp(argv[1], "-p")) {
		checkPosts();
    } else {
        printf("Invalid argument\n");
    }

    return 0;
}
