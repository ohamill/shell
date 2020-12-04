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
#include <errno.h>

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
void runsemicoloncmd(char *[], int);
int pipetokenize(char *[], int, char *[]);

int main(void) {
	char buf[BUFSZ], usrinput[INPUTSZ], newusrinput[INPUTSZ], prevcmd[INPUTSZ];
	char *args[ARGSZ];
	struct passwd *pw;
	char *pwname, *bufptr;
	int wstatus, pid, i, pipesymbol, semicolonsymbol;

	// Get user's username
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
		// Check for blank user input
		if (usrinput[0] == '\n') {
			continue;
		}
		// Check for " symbol
		if (usrinput[0] == '"') {
			strcpy(usrinput, prevcmd);
		}

		addSpaces(usrinput, newusrinput);
		tokenizecmd(newusrinput, args);

		// Check for pipes and semicolons
		i = 0;
		pipesymbol = 0;
		semicolonsymbol = 0;
		while (args[i] != NULL) {
			if (*args[i] == '|') {
				pipesymbol++;
			} else if (*args[i] == ';') {
				semicolonsymbol++;
			}
			i++;
		}
		if (pipesymbol) {
			piperedir(args, pipesymbol);
		} else if (semicolonsymbol) {
			runsemicoloncmd(args, semicolonsymbol);
		} else {
			runcmd(args);
		}
		strcpy(prevcmd, usrinput);
	}
}

void addSpaces(char old[], char new[]) {
	int i, n, j;

	j = 0;
	for (i = 0, n = strlen(old); i <= n; i++) {
		if (old[i] == '&' || old[i] == '|' || old[i] == '<' || old[i] == ';') {
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
				fd = open(cmd[outtrunc], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
				dup2(fd, 1);
			}
			if (outappend) {
				fd = open(cmd[outappend], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
				dup2(fd, 1);
			}
			if (execvp(newcmd[0], newcmd) == -1) {
				perror(newcmd[0]);
			}
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
	int pid, cmdindex, cmdno, wstatus, fd, i;
	char *args[ARGSZ];

	// Create pipes
	for (i = 0; i <= pipecount; i+=2) {
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
				i = 0;
				while (args[i] != NULL) {
					if (strcmp(args[i], "<") == 0) {
						args[i] = NULL;
						fd = open(args[i+1], O_RDONLY);
						dup2(fd, 0);
						break;
					}
					i++;
				}
			// Last command
			} else if (cmdno == (pipecount)) {
				dup2(pfd[(pipecount*2)-2], 0);
				i = 0;
				while (args[i] != NULL) {
					if (strcmp(args[i], ">") == 0) {
						args[i] = NULL;
						fd = open(args[i+1], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
						dup2(fd, 1);
						break;
					} else if (strcmp(args[i], ">>") == 0) {
						args[i] = NULL;
						fd = open(args[i+1], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
						dup2(fd, 1);
						break;
					}
					i++;
				}
			// Middle commands, if any
			} else {
				dup2(pfd[(cmdno-1)*2], 0);
				dup2(pfd[(cmdno*2)+1], 1);
			}

			// Close pipe fds and run command
			for (int i = 0; i <=pipecount*2; i++) {
				close(pfd[i]);
			}
			if (execvp(args[0], args) == -1) {
				perror(args[0]);
			}
			exit(0);
		}
		cmdindex++;
	}

	for (i = 0; i <= pipecount*2; i++) {
		close(pfd[i]);
	}
	for (i = 0; i <= pipecount; i++) {
		wait(&wstatus);
	}
}

int pipetokenize(char *cmd[], int cmdindex, char *new[]) {
	int newindex = 0;

	while (cmd[cmdindex] != NULL && strcmp(cmd[cmdindex], "|") != 0 && strcmp(cmd[cmdindex], ";") != 0) {
		new[newindex] = cmd[cmdindex];
		newindex++;
		cmdindex++;
	}
	new[newindex] = NULL;
	return cmdindex;
}

void runsemicoloncmd(char *cmd[], int semicoloncount) {
	int cmdindex, cmdno, wstatus;
	char *args[ARGSZ];
	char execPath[EXECSZ];

	cmdindex = 0;
	for (cmdno = 0; cmdno <= semicoloncount; cmdno++) {
		cmdindex = pipetokenize(cmd, cmdindex, args);
		runcmd(args);
		cmdindex++;
	}
}