#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <readline/history.h>
#include "./builtins/builtins.h"
#include "./quotations/quotations.h"
#include "sh.h"
#include "util/util.h"
#include "./util/prompt.h"

enum {
	INPUTSZ = 512,
	BUFSZ = 512,
	EXECSZ = 512,
	ARGSZ = 50
};

variable *root;

int main(void) {
	char buf[BUFSZ], newusrinput[INPUTSZ], prevcmd[INPUTSZ];
	char *args[ARGSZ];
	struct passwd *pw;
	char *usrinput, *pwname, *bufptr, *prompt;
	int wstatus, pid, i, pipecount, semicoloncount;
	root = malloc(sizeof(variable));
	root->next = NULL;
	
	pwname = getUsername(pw);

	while (true) {
		checkForCompletedBackgroundProcesses();

		usrinput = getUserInput(pwname);
		if (isUserInputNull(usrinput)) {
			freeVariables(root);
			break;
		}
		if (isUserInputBlank(usrinput)) {
			continue;
		}

		add_history(usrinput);
		addSpaces(usrinput, newusrinput);
		tokenizecmd(newusrinput, args, &pipecount, &semicoloncount);
		
		if (semicoloncount) {
			runsemicoloncmd(args, semicoloncount);
		} else {
			runcmd(args, pipecount);
		}
		free(usrinput);
	}
}

void addSpaces(char *old, char new[]) {
	int i, n, j;

	j = 0;
	for (i = 0, n = strlen(old); i <= n; i++) {
		if (old[i] == '&' || old[i] == '|' || old[i] == '<' || old[i] == ';' || old[i] == '=') {
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

void tokenizecmd(char cmd[], char *tokens[], int *pipes, int *semicolons) {
	int i;
	char *token;
	*pipes = 0;
	*semicolons = 0;

	token = strtok(cmd, " \n");\
	if (isFirstCharQuote(token)) {
		tokens[0] = packageQuotedArg(token);
	} else {
		if (isVariable(token)) {
			tokens[0] = getVariableValue(token, root);
		} else {
			tokens[0] = token;
		}
	}
	if (strcmp(tokens[0], ";") == 0) {
		*semicolons += 1;
	} else if (strcmp(tokens[0], "|") == 0) {
		*pipes += 1;
	}
	i = 1;
	while ((token = strtok(NULL, " \n")) != NULL) {
		if (isFirstCharQuote(token)) {
			tokens[i] = packageQuotedArg(token);
		} else {
			if (isVariable(token)) {
				tokens[i] = getVariableValue(token, root);
			} else {
				tokens[i] = token;
			}
		}
		if (strcmp(tokens[i], ";") == 0) {
			*semicolons += 1;
		} else if (strcmp(tokens[i], "|") == 0) {
			*pipes += 1;
		}
		i++;
	}
	tokens[i] = NULL;
}

void runcmd(char *args[], int pipecount) {
	struct passwd *pw;
	char *pwdir;
	int pid, wstatus, i, newcmdindex, cmdindex, cmdno;
	int inredir, outtrunc, outappend, amp, fd;
	char *cmd[ARGSZ], *newcmd[ARGSZ];
	int pfd[pipecount*2];

	// Initialize pipe (won't do anything if pipecount == 0)
	for (i = 0; i < pipecount; i++) {
		pipe(&pfd[i*2]);
	}

	cmdindex = 0;
	for (cmdno = 0; cmdno <= pipecount; cmdno++) {
		cmdindex = pipetokenize(args, cmdindex, cmd);
		inredir = 0;
		outtrunc = 0;
		outappend = 0;
		amp = 0;

		i = 0;
		newcmdindex = 0;
		while (cmd[i] != NULL) {
			// Input redirection
			if (strcmp(cmd[i], "<") == 0) {
				inredir = i + 1; // Get index of file that will replace stdin
				i++;
			// Output truncate
			} else if (strcmp(cmd[i], ">") == 0) {
				outtrunc = i + 1;
				i++;
			// Output append
			} else if (strcmp(cmd[i], ">>") == 0) {
				outappend = i + 1;
				i++;
			// Background
			} else if (strcmp(cmd[i], "&") == 0) {
				amp++;
			} else {
				newcmd[newcmdindex] = cmd[i];
				newcmdindex++;
			}
			i++;
		}
		newcmd[newcmdindex] = NULL; // Null-terminate newcmd array of arguments

		// Handle pipe, if necessary
		if (pipecount) {
			if (fork() == 0) {
				// First command
				if (cmdno == 0) {
					if (outappend || outtrunc) {
						pid = fork();
						if (pid == 0) {
							if (outappend) {
								fd = open(cmd[outappend], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
								dup2(fd, 1);
							} else if (outtrunc) {
								fd = open(cmd[outtrunc], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
								dup2(fd, 1);
							}
							if (execvp(newcmd[0], newcmd) == -1) {
								perror(newcmd[0]);
							}
							exit(0);
						} else {
							waitpid(pid, &wstatus, 0);
						}
					}
					dup2(pfd[1], 1);
				// Last command
				} else if (cmdno == pipecount) {
					dup2(pfd[(pipecount*2)-2], 0);
					if (outappend) {
						fd = open(cmd[outappend], O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
						dup2(fd, 1);
					} else if (outtrunc) {
						fd = open(cmd[outtrunc], O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
						dup2(fd, 1);
					}
				// Middle commands, if any
				} else {
					dup2(pfd[(cmdno-1)*2], 0);
					dup2(pfd[(cmdno*2)+1], 1);
				}

				// Close pipe fds and run command
				for (int i = 0; i < pipecount*2; i++) {
					close(pfd[i]);
				}
				if (execvp(newcmd[0], newcmd) == -1) {
					perror(newcmd[0]);
				}
				exit(0);
			}
		// Command does not contain any pipes
		} else {
			// cd built-in
			if (strcmp(newcmd[0], "cd") == 0) {
				if (newcmd[1] == NULL) {
					pw = getpwuid(getuid());
					pwdir = pw->pw_dir;
					chdir(pwdir);
				} else {
					chdir(newcmd[1]);
				}
			} else if (strcmp(newcmd[0], "echo") == 0) {
				pid = fork();
				if (pid == 0) {
					echo(newcmd);
					exit(0);
				} else if (!amp) {
					waitpid(pid, &wstatus, 0);
				} else if (amp) {
					printf("Process started in the background: %d\n", pid);
				}
			} else if (strcmp(newcmd[0], "set") == 0) {
				set(newcmd, root);
			} else if (strcmp(newcmd[0], "which") == 0) {
				pid = fork();
				if (pid == 0) {
					which(newcmd);
					exit(0);
				} else if (!amp) {
					waitpid(pid, &wstatus, 0);
				} else if (amp) {
					printf("Process started in the background: %d\n", pid);
				}
			} else {
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
		cmdindex++;
	}

	if (pipecount) {
		for (i = 0; i < pipecount*2; i++) {
			close(pfd[i]);
		}
		for (i = 0; i <= pipecount; i++) {
			wait(&wstatus);
		}
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
	int cmdindex, cmdno;
	char *args[ARGSZ];

	cmdindex = 0;
	for (cmdno = 0; cmdno <= semicoloncount; cmdno++) {
		cmdindex = pipetokenize(cmd, cmdindex, args);
		runcmd(args, 0);
		cmdindex++;
	}
}