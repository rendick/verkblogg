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
#include <dirent.h>
#include <sys/stat.h>

#include "./config.h"
#include "./utilities.h"

#define COLOR_BOLD "\e[1m"
#define COLOR_OFF "\e[m"

int createPost();
int parsePostsDb();
int updateIndexFile();
int updatePostsFiles();

int createPost() {
	char title[30], title_tl[30];
	char main_content[100];

	input("Title", title, 30);
	input("Main", main_content, 100);

	for (int i = 0; i < sizeof(title) - 1 && title[i] != '\0'; i++) {
		title_tl[i] = tolower(title[i]);
	}

	FILE* wttf;
	if (openFile(&wttf, dbpath, "a") != 0) {
		return 1;
	}

	fprintf(wttf, "%s;%s;%s\n", title, main_content, title_tl);
	fclose(wttf);

	parsePostsDb();

	return 0;
}

int parsePostsDb() {
	if (access(dbpath, 0) != 0) {
		printf("DB file is missing. Re-run command to create it.\n");
		exit(1);
	}

	FILE* pfpn;
	if (openFile(&pfpn, dbpath, "r") != 0) {
		return 1;
	}

	char buffer[256];
	char* lines[100] = {NULL};

	int x = 0;
	while (fgets(buffer, sizeof(buffer), pfpn)) {
		lines[x] = strdup(buffer);
		x++;
	}
	fclose(pfpn);

	char allPosts[5000] = "";

	for (int i = 0; i < x; i++) {
		replaceAll(lines[i], "\n", "");

		char* splitted_title = NULL;
		char* splitted_main = NULL;
		char* splitted_path = NULL;

		char* splitDb = strtok(lines[i], ";");
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
			snprintf(postInformation, sizeof(postInformation),
			         "<li><a href='./%s/%s.html'>%s</a></li>\n", compiledpath,
			         splitted_path, splitted_title);
			strncat(allPosts, postInformation,
			        sizeof(allPosts) - strlen(allPosts) - 1);
		}

		free(splitted_title);
		free(splitted_main);
		free(splitted_path);
		free(lines[i]);
	}

	return updateIndexFile(allPosts);
}

int updateIndexFile(char* info) {
	char indexContent[5000];
	char updatedIndexContent[5000] = "";

	FILE* openIndexFile;
	if (openFile(&openIndexFile, indexpath, "r") != 0) {
		return 1;
	}

	while (fgets(indexContent, 1000, openIndexFile)) {
		replaceAll(indexContent, "${indexTitle}", info);
		strncat(updatedIndexContent, indexContent,
		        sizeof(updatedIndexContent) - strlen(updatedIndexContent) - 1);
	}

	fclose(openIndexFile);

	FILE* writeUpdatedIndexFile;
	if (openFile(&writeUpdatedIndexFile, "index.html", "w") != 0) {
		return 1;
	}

	fprintf(writeUpdatedIndexFile, "%s", updatedIndexContent);

	fclose(writeUpdatedIndexFile);

	return updatePostsFiles();
}

int updatePostsFiles() {
	DIR *openPostDir = opendir(compiledpath);
	if (!(openPostDir)) {
		mkdir(compiledpath, 0777);
	}

	FILE* pfpn;
	if (openFile(&pfpn, dbpath, "r") != 0) {
		return 1;
	}

	char buffer[256];
	char* lines[100] = {NULL};

	int x = 0;
	while (fgets(buffer, sizeof(buffer), pfpn)) {
		lines[x] = strdup(buffer);
		x++;
	}
	fclose(pfpn);

	for (int i = 0; i < x; i++) {
		replaceAll(lines[i], "\n", "");

		char* splitted_title = NULL;
		char* splitted_main = NULL;
		char* splitted_path = NULL;

		char* splitDb = strtok(lines[i], ";");
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
			FILE* tpl;
			if (openFile(&tpl, postpath, "r") != 0) {
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
			replaceAll(templateContent, "${main}", splitted_main);

			char filePath[256];
			snprintf(filePath, sizeof(filePath), "%s/%s.html", compiledpath,splitted_path);

			FILE* postFile;
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

int checkPosts() {
	if (access(dbpath, 0) != 0) {
		printf("DB file is missing.\n");
		exit(1);
	}

	FILE* openPostsFile;
	if (openFile(&openPostsFile, dbpath, "r") != 0) {
		return 1;
	}

	char buffer[256];
	char* lines[100] = {NULL};

	int x = 0;
	while (fgets(buffer, sizeof(buffer), openPostsFile)) {
		lines[x] = strdup(buffer);
		x++;
	}

	fclose(openPostsFile);

	for (int i = 0; i < x; i++) {
		replaceAll(lines[i], "\n", "");

		char* splitted_title = NULL;
		char* splitted_main = NULL;
		char* splitted_path = NULL;

		int idx = 0;

		char* token = strtok(lines[i], ";");

		while (token != NULL) {
			if (idx == 0) {
				splitted_title = strdup(token);
			} else if (idx == 1) {
				splitted_main = strdup(token);
			} else if (idx == 2) {
				splitted_path = strdup(token);
			}

			token = strtok(NULL, ";");
			idx++;
		}

		printf("%d. %s%s%s - %s - %s/%s.html\n", i, COLOR_BOLD,
		       splitted_title, COLOR_OFF, splitted_main, compiledpath, splitted_path);
	}

	return 0;
}

int generateConfigFile() {
	// if (access("config.h", 0) == 0) {
	// 	printf("Configuraion file exists! Exiting...");
	// 	exit(1);
	// }

	FILE *configFile;
	if(openFile(&configFile, "config.h.test", "w") != 0) {
		return 1;
	}

	fprintf(configFile, "const char *projectname = \"\"; // Project name\nconst char *indexpath = \"public/index.html\"; // Default index.html template\nconst char *postpath = \"public/article.html\"; // Default article.html template\nconst char *compiledpath = \"\"; // Folder where articles will be compiled\nconst char *dbpath = \"\"; // Database\n");
	fclose(configFile);

	return 0;
}

int main(int argc, char* argv[]) {
//    if (access("verkblogg.h", 0) != 0 && strcmp(argv[1], "-c")) {
//        printf(
//            "Configuraion file does not exist!\nTry 'verkblogg -h for more information\n"
	// 		);
//        exit(1);
	// }

	if (argc < 2) {
		printf("Usage: %s -n|-p|-c|-h|-v\nTry '-h' for more information.\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "-n")) {
		createPost();
	} else if (!strcmp(argv[1], "-p")) {
		checkPosts();
	} else if (!strcmp(argv[1], "-c")) {
		// generateConfigFile();
		printf("In development.");
	} else if (!strcmp(argv[1], "-h")) {

		printf("Usage: verkblogg [OPTION]\n"
		       "  -n\tfor creating articles\n"
		       "  -p\tfor viewing DB\n"
		       "  -c\tfor generating configuration file\n\n"
		       "  -h\tdisplay help menu\n"
		       "  -v\tdisplay current version\n");
	} else if (!strcmp(argv[1], "-v")) {
		printf("verkblogg v1.1\n");
	} else {
		printf("Invalid argument.\n");
	}

	return 0;
}
