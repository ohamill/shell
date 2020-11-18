#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <fcntl.h>

enum {
	INPUTSZ = 512,
	BUFSZ = 512,
	EXECSZ = 512
};

char execPath[EXECSZ];
char *args[50];
int pid, wstatus, i, fd;

void tokenizecmd(char []);
void runcmd(char *[]);
void outredir(char *[], char *, bool);
void inredir(char *[], char *);
void piperedir(char *[]);
void ampcmd(char *[]);

int main(void) {
	char usrinput[INPUTSZ], buf[BUFSZ];
	char *bufptr, *pwname, *pwdir;
	struct passwd *pw;

	// Get user's home directory and username
	pw = getpwuid(getuid());
	pwname = pw->pw_name;
	pwdir = pw->pw_dir;

	while (true) {
		// Check for completed background processes
		if ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
			printf("Process completed: %d\n", pid);
		}
		// Print prompt
		getcwd(buf, BUFSZ);
		bufptr = strstr(buf, pwname);
		printf("%s: ", bufptr);
		// Read user input
		if (fgets(usrinput, INPUTSZ, stdin) == NULL) {
			printf("\n");
			break;
		}

		tokenizecmd(usrinput);

		if (i == 0) {
			continue;
		}
		args[i] = NULL;
		runcmd(args);
	}
}

void tokenizecmd(char cmd[]) {
	char *token;

	i = 0;

	if (cmd != NULL) {
		args[0] = strtok(cmd, " \t\n");
		i = 1;
	}

	while ((token = strtok(NULL, " \t\n")) != NULL) {
		// Replace stdin
		if (strcmp(token, "<") == 0) {
			inredir(args, token);
		// Truncate stdout
		} else if (strcmp(token, ">") == 0) {
			outredir(args, token, true);
		// Append stdout
		} else if (strcmp(token, ">>") == 0) {
			outredir(args, token, false);
		// Pipe
		} else if (strcmp(token, "|") == 0) {
			piperedir(args);
		// Run in background
		} else if (strcmp(token, "&") == 0) {
			ampcmd(args);
		} else {
			args[i] = token;
			i++;
		}
	}
}

void runcmd(char *args[]) {
	struct passwd *pw;
	char *pwdir;

	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) {
			pw = getpwuid(getuid());
			pwdir = pw->pw_dir;
			chdir(pwdir);
		} else {
			chdir(args[1]);
		}
	} else {
		pid = fork();
		if (pid == 0) {
			sprintf(execPath, "/bin/%s", args[0]);
			execv(execPath, args);
			exit(0);
		} else {
			waitpid(pid, &wstatus, 0);
		}
	}
}

void outredir(char *args[], char *token, bool trunc) {

	args[i] = NULL;
	token = strtok(NULL, " \t\n");

	pid = fork();
	if (pid == 0) {
		if (trunc) {
			fd = open(token, O_WRONLY | O_TRUNC);
		} else {
			fd = open(token, O_WRONLY | O_APPEND);
		}
		dup2(fd, 1);
		sprintf(execPath, "/bin/%s", args[0]);
		execv(execPath, args);
		exit(0);
	} else {
		waitpid(pid, &wstatus, 0);
		i = 0;
	}
}

void inredir(char *args[], char *token) {

	args[i] = NULL;
	token = strtok(NULL, " \t\n");

	pid = fork();
	if (pid == 0) {
		fd = open(token, O_RDONLY);
		dup2(fd, 0);
		sprintf(execPath, "/bin/%s", args[0]);
		execv(execPath, args);
		exit(0);
	} else {
		waitpid(pid, &wstatus, 0);
		i = 0;
	}
}

void piperedir(char *args[]) {
	char *token;
	int pfd[2];

	args[i] = NULL;

	pipe(pfd);

	// Write to pipe
	if (fork() == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
		sprintf(execPath, "/bin/%s", args[0]);
		execv(execPath, args);
		exit(0);
	}

	// Get next command after pipe
	i = 0;
	while ((token = strtok(NULL, " \t\n")) != NULL) {
		args[i] = token;
		i++;
	}
	args[i] = NULL;

	// Read from pipe
	if (fork() == 0) {
		close(pfd[1]);
		dup2(pfd[0], 0);
		sprintf(execPath, "/bin/%s", args[0]);
		execv(execPath, args);
		exit(0);
	}

	close(pfd[0]);
	close(pfd[1]);
	wait(&wstatus);
	wait(&wstatus);
	i = 0;
}

void ampcmd(char *args[]) {

	args[i] = NULL;

	pid = fork();
	if (pid == 0) {
		// to-do
		sprintf(execPath, "/bin/%s", args[0]);
		execv(execPath, args);
		exit(0);
	} else {
		printf("Process in-progress: %d\n", pid);
	}

	i = 0;
}
