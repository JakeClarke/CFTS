#ifndef CONFIG_H
#define CONFIG_H

#define ACC_READ 1
#define ACC_WRITE 2
#define ACC_EXEC 4
#define ACC_SHUTDOWN 8
#define MIN_FORKS 1

typedef struct
{
	char * usrName, * passwd;
	char access;
} user;

int numberOfChilren = 5;
char * currUsrName;
char currAccess = 0;
user * users = NULL;
int numUsers = 0;
int port = 4321;
char * wkDir = "./";
char wdChanged = 0;

void readConfig(char * file) {
	FILE *fp;
	fp = fopen(file, "r");

	if (fp == NULL) {
		printf("Failed to open file: %s", file);
		exit(EXIT_FAILURE);
	}

	char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int lineNo = 0;

	while((read = getline(&line, &len, fp)) != -1)
	{
		lineNo++;
		if(strncmp(line, "port: ", 6) == 0 && strlen(line) > 6) {
			port = atoi(&line[6]);
		}
		else if(strncmp(line, "wd: ", 4) == 0) {
			int len = strlen(&line[4]) + 1;
			if(wdChanged)
				free(wkDir);
			wkDir = malloc(len * sizeof(char));
			memcpy(wkDir, &line[4], len * sizeof(char));
			if(wkDir[len - 2] == '\n')
				wkDir[len - 2] = '\0';

			wdChanged = 1;
		}
		else if(strncmp(line, "u:", 2) == 0 && strlen(line) > 2) {
			users = (user*)realloc(users, ++numUsers * sizeof(user));

			// copy the username.
			char * p = strtok(&line[2], ":");
			int len = strlen(p) + 1; // dont forget the null term.
			users[numUsers -1].usrName = malloc(len * sizeof(char));
			memcpy(users[numUsers -1].usrName, p, len * sizeof(char));

			// copy the password.
			p = strtok(NULL, ":");
			len = strlen(p) + 1; // dont forget the null term.
			users[numUsers -1].passwd = malloc(len * sizeof(char));
			memcpy(users[numUsers -1].passwd, p, len * sizeof(char));

			// load in the access level for the user
			p = strtok(NULL, ":");
			users[numUsers -1].access = atoi(p);

			syslog(LOG_DEBUG, "Added user: %s, password: %s, access: %i", users[numUsers -1].usrName, users[numUsers -1].passwd, users[numUsers -1].access);
		}
		else if(strncmp(line, "forks: ", 7) == 0 && strlen(line) > 7) {
			numberOfChilren = atoi(&line[6]);
			if(numberOfChilren <= MIN_FORKS) {
				printf("Warning config fork value too low setting to min value: %i\n", MIN_FORKS);
				numberOfChilren = MIN_FORKS;
			}
		}
		else {
			printf("Line %i no recognised in config file: %s", lineNo, file);
		}
	}

	fclose(fp);
	if(numUsers == 0) {
		syslog(LOG_WARNING, "No users in config, server will allow any password and user name!");
	}
}

char authUser(char * user, char * pass) {
	if(numUsers == 0) {
		int usrLen = strlen(user);
		currUsrName = malloc(usrLen + 1);
		memcpy(currUsrName, user, usrLen * sizeof(char));
		currUsrName[usrLen] = '\0';
		currAccess = 7;
		return 1;
	}

	for(int i = 0; i < numUsers; i++) {
		if(strcmp(user, users[i].usrName) == 0 && strcmp(pass, users[i].passwd) == 0)
		{
			currUsrName = users[i].usrName;
			currAccess = users[i].access;
			return 1;
		}
	}

	return 0;
}
#endif
