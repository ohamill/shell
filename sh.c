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
	EXECSZ = 512,
	ARGSZ = 50
};

void addSpaces(char [], char[]);
void tokenizecmd(char [], char*[]);
void runcmd(char *[]);
void piperedir(char *[], int);
int pipetokenize(char *[], int, char *[]);

int main(void) {
	char buf[BUFSZ], usrinput[INPUTSZ], newusrinput[INPUTSZ];
	char *args[ARGSZ];
	struct passwd *pw;
	char *pwname, *bufptr;
	int wstatus, pid, i, pipesymbol;

	// Get user's home directory and username
	pw = getpwuid(getuid());
	pwname = pw->pw_name;

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

		addSpaces(usrinput, newusrinput);
		tokenizecmd(newusrinput, args);

		// Check for pipes
		i = 0;
		pipesymbol = 0;
		while (args[i] != NULL) {
			if (*args[i] == '|') {
				pipesymbol++;
			}
			i++;
		}
		if (pipesymbol) {
			piperedir(args, pipesymbol);
		} else {
			runcmd(args);
		}
	}
}

void addSpaces(char old[], char new[]) {
	int j;

	j = 0;
	for (int i = 0, n = strlen(old); i <= n; i++) {
		if (old[i] == '&' || old[i] == '|' || old[i] == '<') {
			new[j] = ' ';
			new[j+1] = old[i];
			new[j+2] = ' ';
			j += 3;
		} else if (old[i] == '>') {
			if (old[i+1] == '>') {
				new[j] = ' ';
				new[j+1] = old[i];
				new[j+2] = old[i+1];
				new[j+3] = ' ';
				j += 4;
				i++;
			} else {
				new[j] = ' ';
				new[j+1] = old[i];
				new[j+2] = ' ';
				j += 3;
			}
		} else {
			new[j] = old[i];
			j++;
		}
	}
	new[j] = '\0';
}

void tokenizecmd(char cmd[], char *tokens[]) {
	int i;
	char *token;

	tokens[0] = strtok(cmd, " \n");
	i = 1;
	while ((token = strtok(NULL, " \n")) != NULL) {
		tokens[i] = token;
		i++;
	}
	tokens[i] = NULL;
}

void runcmd(char *cmd[]) {
	struct passwd *pw;
	char *pwdir;
	char execPath[EXECSZ];
	int pid, wstatus, i, newcmdindex;
	int inredir, outtrunc, outappend, amp, fd;
	char *newcmd[ARGSZ];

	// cd built-in
	if (strcmp(cmd[0], "cd") == 0) {
		if (cmd[1] == NULL) {
			pw = getpwuid(getuid());
			pwdir = pw->pw_dir;
			chdir(pwdir);
		} else {
			chdir(cmd[1]);
		}
	// Run command if not built-in
	} else {
		inredir = 0;
		outtrunc = 0;
		outappend = 0;
		amp = 0;

		i = 0;
		newcmdindex = 0;
		while (cmd[i] != NULL) {
			// Input redirection
			if (strcmp(cmd[i],"<") == 0) {
				inredir = i + 1;
				i++;
			// Output truncate
			} else if (strcmp(cmd[i],">") == 0) {
				outtrunc = i + 1;
				i++;
			// Output append
			} else if (strcmp(cmd[i],">>") == 0) {
				outappend = i + 1;
				i++;
			// Background
			} else if (strcmp(cmd[i],"&") == 0) {
				amp++;
			} else {
				newcmd[newcmdindex] = cmd[i];
				newcmdindex++;
			}
			i++;
		}
		newcmd[newcmdindex] = NULL;

		pid = fork();
		if (pid == 0) {
			if (inredir) {
				fd = open(cmd[inredir], O_RDONLY);
				dup2(fd, 0);
			}
			if (outtrunc) {
				fd = open(cmd[outtrunc], O_WRONLY|O_TRUNC);
				dup2(fd, 1);
			}
			if (outappend) {
				fd = open(cmd[outappend], O_WRONLY|O_APPEND);
				dup2(fd, 1);
			}
			sprintf(execPath, "/bin/%s", newcmd[0]);
			execv(execPath, newcmd);
			exit(0);
		} else if (!amp) {
			waitpid(pid, &wstatus, 0);
		} else if (amp) {
			printf("Process started in the background: %d\n", pid);
		}
	}
}

void piperedir(char *cmd[], int pipecount) {
	int pfd[pipecount*2];
	int pid, cmdindex, cmdno, wstatus;
	char *args[ARGSZ];
	char execPath[EXECSZ];

	// Create pipes
	for (int i = 0; i <= pipecount; i+=2) {
		pipe(&pfd[i]);
	}

	// Run commands
	cmdindex = 0;
	for (cmdno = 0; cmdno <= pipecount; cmdno++) {
		cmdindex = pipetokenize(cmd, cmdindex, args);
		if (fork() == 0) {
			// First command
			if (cmdno == 0) {
				dup2(pfd[1], 1);
			// Last command
			} else if (cmdno == (pipecount)) {
				dup2(pfd[(pipecount*2)-2], 0);
			// Middle commands, if any
			} else {
				dup2(pfd[(cmdno-1)*2], 0);
				dup2(pfd[(cmdno*2)+1], 1);
			}

			// Close pipe fds and run command
			for (int i = 0; i <=pipecount*2; i++) {
				close(pfd[i]);
			}
			sprintf(execPath, "/bin/%s", args[0]);
			execv(execPath, args);
			exit(0);
		}
		cmdindex++;
	}

	for (int i = 0; i <= pipecount*2; i++) {
		close(pfd[i]);
	}
	for (int i = 0; i <= pipecount; i++) {
		wait(&wstatus);
	}
}

int pipetokenize(char *cmd[], int cmdindex, char *new[]) {
	int newindex = 0;

	while (cmd[cmdindex] != NULL && strcmp(cmd[cmdindex], "|") != 0) {
		new[newindex] = cmd[cmdindex];
		newindex++;
		cmdindex++;
	}
	new[newindex] = NULL;
	return cmdindex;
}
