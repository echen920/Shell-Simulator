#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <mcheck.h>

#include "parser.h"
#include "shell.h"

/**
 * Program that simulates a simple shell.
 * The shell covers basic commands, including builtin commands (cd and exit only), 
 * standard I/O redirection and piping (|). 
 * 
 * You should not have to worry about more complex operators such as "&&", ";", etc.
 * Your program does not have to address environment variable substitution (e.g., $HOME), 
 * double quotes, single quotes, or back quotes.
 */

#define MAX_DIRNAME 100
#define MAX_COMMAND 1024
#define MAX_TOKEN 128

/* Functions to implement, see below after main */
int execute_cd(char** words);
int execute_nonbuiltin(simple_command *s);
int execute_simple_command(simple_command *cmd);
int execute_complex_command(command *cmd);


int main(int argc, char** argv) {
	
	char cwd[MAX_DIRNAME];           /* Current working directory */
	char command_line[MAX_COMMAND];  /* The command */
	char *tokens[MAX_TOKEN];         /* Command tokens (program name, parameters, pipe, etc.) */

	while (1) {

		/* Display prompt */		
		getcwd(cwd, MAX_DIRNAME-1);
		printf("%s> ", cwd);
		
		/* Read the command line */
		gets(command_line);
		
		/* Parse the command into tokens */
		parse_line(command_line, tokens);

		/* Empty command */
		if (!(*tokens))
			continue;
		
		/* Exit */
		if (strcmp(tokens[0], "exit") == 0)
			exit(0);
				
		/* Construct chain of commands, if multiple commands */
		command *cmd = construct_command(tokens);
		//print_command(cmd, 0);
    
		int exitcode = 0;
		if (cmd->scmd) {
			exitcode = execute_simple_command(cmd->scmd);
			if (exitcode == -1)
				break;
		}
		else {
			exitcode = execute_complex_command(cmd);
			if (exitcode == -1)
				break;
		}
		release_command(cmd);
	}
    
	return 0;
}


/**
 * Changes directory to a path specified in the words argument;
 * For example: words[0] = "cd"
 *              words[1] = "csc209/assignment2/"
 * Your command should handle both relative paths to the current 
 * working directory, and absolute paths relative to root,
 * e.g., relative path:  cd csc209/assignment2/
 *       absolute path:  cd /u/bogdan/csc209/assignment2/
 * You don't have to handle paths containing "~" or environment 
 * variables like $HOME, etc.. 
 */
int execute_cd(char** words) {
	
	/** 
	 * TODO: 
	 * The first word contains the "cd" string, the second one contains the path.
	 * Check possible errors:
	 * - The words pointer could be NULL, the first string or the second string
	 *   could be NULL, or the first string is not a cd command
	 * - If so, return an EXIT_FAILURE status to indicate something is wrong.
	 */

	// pointer words is null
	if (!words) {
		return EXIT_FAILURE;
	}

	// words[0] or words[1] is null
	if (!words[0] || !words[1]) {
		return EXIT_FAILURE;
	}

	// word[0] is not "cd"
	if (strcmp(words[0], "cd") != 0) {
		return EXIT_FAILURE;
	}

	/**
	 * TODO: 
	 * The safest way would be to first determine if the path is relative 
	 * or absolute (see is_relative function provided).
	 * - If it's not relative, then simply change the directory to the path 
	 * specified in the second word in the array.
	 * - If it's relative, then make sure to get the current working directory, 
	 * append the path in the second word to the current working directory and 
	 * change the directory to this path.
	 * Hints: see chdir and getcwd man pages.
	 * Return the success/error code obtained when changing the directory.
	 */
	 if (!is_relative(words[1])) {
		if (chdir(words[1]) == 0) {
			return 0;
		} else {
			perror(words[1]);
			return 1;
		}
	} else {
		char cwd[MAX_DIRNAME];
		getcwd(cwd, MAX_DIRNAME-1);
		strncat(cwd, "/", MAX_DIRNAME-1);
		strncat(cwd, words[1], MAX_DIRNAME-1);
		if (chdir(words[1]) == 0) {
			return 0;
		} else {
			perror(words[1]);
			return 1;
		}
	}
}


/**
 * Executes a regular command, based on the tokens provided as 
 * an argument.
 * For example, "ls -l" is represented in the tokens array by 
 * 2 strings "ls" and "-l", followed by a NULL token.
 * The command "ls ?l | wc -l" will contain 5 tokens, 
 * followed by a NULL token. 
 */
int execute_command(char **tokens) {
	
	/**
	 * TODO: execute a regular command, based on the tokens provided.
	 * The first token is the command name, the rest are the arguments 
	 * for the command. 
	 * Hint: see execlp/execvp man pages.
	 * 
	 * - In case of error, make sure to use "perror" to indicate the name
	 *   of the command that failed.
	 *   You do NOT have to print an identical error message to what would happen in bash.
	 *   If you use perror, an output like: 
	 *      my_silly_command: No such file of directory 
	 *   would suffice.
	 * - Function returns the result of the execution.
	 */
	int result;
	if ((result=execvp(tokens[0], tokens)) < 0) {
		perror(tokens[0]);
	}
	return result;

}


/**
 * Executes a non-builtin command.
 */
int execute_nonbuiltin(simple_command *s)
{
	/**
	 * TODO: Check if the in, out, and err fields are set (not NULL),
	 * and, IN EACH CASE:
	 * - Open a new file descriptor (make sure you have the correct flags,
	 *   and permissions);
	 * - Set the file descriptor flags to 1 using fcntl - to avoid leaving 
	 *   the file descriptor open across an execve. Don't worry about this
	 *   too much, just use this: 
	 *         fcntl(myfd, F_SETFD, 1);
	 *   where myfd is the file descriptor you just opened.
	 * - redirect stdin/stdout/stderr to the corresponding file.
	 *   (hint: see dup2 man pages).
	 */
	int filedesc_in, filedesc_out, filedesc_err;
	int fd_in[2], fd_out[2], fd_err[2];
	int pid_in, pid_out, pid_err;
	if (s->in) {
		filedesc_in = open(s->in, O_RDONLY);
		fcntl(filedesc_in, F_SETFD, 1);
		dup2(filedesc_in, fileno(stdin));
		close(filedesc_in);
		pipe(fd_in);
		if ((pid_in = fork()) == 0) {
			dup2(fd_in[1], fileno(stdout));
			close(fd_in[0]);
			close(fd_in[1]);
		} else if (pid_in > 0) {
			dup2(fd_in[0], fileno(stdin));
			close(fd_in[1]);
			close(fd_in[0]);
		} else {
			perror("fork");
			exit(1);
		}
	}

	if (s->out) {
		filedesc_out = open(s->out, O_WRONLY | O_APPEND | O_CREAT, 0700);
		fcntl(filedesc_out, F_SETFD, 1);
		dup2(filedesc_out, fileno(stdout));
		close(filedesc_out);
		pipe(fd_out);
		if ((pid_out = fork()) == 0) {
			dup2(fd_out[0], fileno(stdin));
			close(fd_out[0]);
			close(fd_out[1]);
		} else if (pid_out > 0) {
			dup2(fd_out[1], fileno(stdout));
			close(fd_out[0]);
			close(fd_out[1]);
		} else {
			perror("fork");
			exit(1);
		}
	}

	if (s->err) {
		filedesc_err = open(s->err, O_WRONLY | O_APPEND | O_CREAT, 0700);
		fcntl(filedesc_err, F_SETFD, 1);
		dup2(filedesc_err, fileno(stderr));
		close(filedesc_err);
		pipe(fd_err);
		if ((pid_err = fork()) == 0) {
			dup2(fd_err[0], fileno(stdin));
			close(fd_err[0]);
			close(fd_err[1]);
		} else if (pid_err > 0) {
			dup2(fd_err[1], fileno(stderr));
			close(fd_err[0]);
			close(fd_err[1]);
		} else {
			perror("fork");
			exit(1);
		}
	}

	/**
	 * TODO: Finally, execute the command using the tokens 
	 * (see execute_command function provided)
	 */
	int result;
	result = execute_command(s->tokens);


	/**
	 * TODO: Close all filedescriptors needed and return status generated 
	 * by the command execution.
	 */
	close(filedesc_in);
	close(filedesc_out);
	close(filedesc_err);
	return result;
}


/**
 * Executes a simple command (no pipes).
 */
int execute_simple_command(simple_command *cmd) {
	pid_t pid;
	int status;
	int result = 0;

	/**
	 * TODO: 
	 * Check if the command is builtin.
	 * 1. If it is, then handle BUILTIN_CD (see execute_cd function provided) 
	 *    and BUILTIN_EXIT (simply return an appropriate exit status).
	 * 2. If it isn't, then you must execute the non-builtin command. 
	 * - Fork a process to execute the nonbuiltin command 
	 *   (see execute_nonbuiltin function provided).
	 * - The parent should wait for the child.
	 *   (see wait man pages).
	 */
	if (cmd->builtin != 0) {
		if (cmd->builtin == 1) {
			result = execute_cd(cmd->tokens);
		} else if (cmd->builtin == 2) {
			exit(0);
		}
	} else {
		pid = fork();

		// parent
		if (pid > 0) {
			wait(&status);
		}
		// child
		else if (pid == 0) {
			result = execute_nonbuiltin(cmd);
			exit(result);
		} else {
			perror("fork");
			exit(1);
		}
	}
	return result;
}


/**
 * Executes a complex command - it can have pipes (and optionally, other operators).
 */
int execute_complex_command(command *c) {
	
	/**
	 * TODO:
	 * Check if this is a simple command, using the scmd field.
	 * Remember that this will be called recursively, so when you encounter
	 * a simple command you should act accordingly.
	 * Execute nonbuiltin commands only. If it's exit or cd, you should not 
	 * execute these in a piped context, so simply ignore builtin commands. 
	 */

	if (!c->scmd) {
	/** 
	 * Optional: if you wish to handle more than just the 
	 * pipe operator '|' (the '&&', ';' etc. operators), then 
	 * you can add more options here. 
	 */

		if (!strcmp(c->oper, "|")) {
		/**
		 * TODO: Create a pipe "pfd" that generates a pair of file descriptors, 
		 * to be used for communication between the parent and the child.
		 * Make sure to check any errors in creating the pipe.
		 */

			int pfd[2], pid, pid2, status, status2;
			pipe(pfd);
		/**
		 * TODO: Fork a new process.
		 * In the child:
		 *  - close one end of the pipe pfd and close the stdout file descriptor.
		 *  - connect the stdout to the other end of the pipe (the one you didn't close).
		 *  - execute complex command cmd1 recursively. 
		 * In the parent: 
		 *  - fork a new process to execute cmd2 recursively.
		 *  - In child 2:
		 *     - close one end of the pipe pfd (the other one than the first child), 
		 *       and close the standard input file descriptor.
		 *     - connect the stdin to the other end of the pipe (the one you didn't close).
		 *     - execute complex command cmd2 recursively. 
		 *  - In the parent:
		 *     - close both ends of the pipe. 
		 *     - wait for both children to finish.
		 */
			if ((pid = fork()) == 0) {
				close(fileno(stdout));
				close(pfd[0]);
				dup2(pfd[1], fileno(stdout));
				execute_complex_command(c->cmd1);
				exit(0);
			} else if (pid > 0) {
				if ((pid2 = fork()) == 0) {
					close(pfd[1]);
					close(fileno(stdin));
					dup2(pfd[0], fileno(stdin));
					execute_complex_command(c->cmd2);
					exit(0);
				} else if (pid2 > 0) {
					close(pfd[0]);
					close(pfd[1]);
					if ((pid2 = wait(&status)) == -1) {
						perror("wait");
					}
					if ((pid2 = wait(&status2)) == -1) {
						perror("wait");
					}
				} else {
					perror("fork");
					exit(1);
				}
			} else {
				perror("fork");
				exit(1);
			}
		}

	} else {
		if(!is_builtin(*(c->scmd->tokens))) {
			execute_nonbuiltin(c->scmd);
		}
	}
	return 0;
}

