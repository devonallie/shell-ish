/*
* File: Assignment2.c
* Author: Devon Allie B00742753
* Date: May 31th 2020
* Version: 7.0
*
* Purpose:	This program is a simplified UNIX shell. Features such as calling
			system files as well as "piping" the output of one file into the 
			input of another are implemented. Multiple commands can be input in
			a single line when seperated by a semicolon. Whitespace is required
			between a command and its arguments but is not required between
			commands or semicolons. The shell will exit when "exit" is entered.
*
*
*
*
* Notes:	Background processes are nearly complete but I ran out of time. 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CHAR 80
#define MAX_ARG 10

void tokenizeArguments(char commandToken[MAX_CHAR]);

void tokenizeCommands(char pipelineToken[MAX_CHAR]);

void tokenizePipeline(char userInput[MAX_CHAR]);

void execute(char* argv[MAX_ARG]);

char backgroundIndicator;

int pipeDirection, pipeSide;

/*
* File descriptor array is declared globally so that piping functions properly.
*/

int fd[2];

int main(void)
{	
	char userInput[MAX_CHAR];
	
	int i;

	while(1)
	{	
		pid_t pida = waitpid(-1, NULL, WNOHANG);
		
		if(pida > 0)
		{
			printf("PID is %d\n", pida);
		}
		
		printf("$ ");
		
		fflush(stdout);		
		
		fgets(userInput, MAX_CHAR, stdin);
			
		tokenizePipeline(userInput);
		
	}
	
	return 0;
}

/*
* This function tokenizes each part of the user input, seperated by the "|" or
* pipe command. Each token is saved as a string to an array. Based on if the 
* string is before or after the pipe, a pipeSide flag is set. Each string is
* sent to the manyCommands function. 
*/

void tokenizePipeline(char userInput[MAX_CHAR])
{
	char* pipelineToken;
	
	char pipelineTokenArray[MAX_ARG][MAX_CHAR];
	
	int i = 0;

	pipe(fd);	//pipe is opened in the first function
	
	pipeSide = 0;
			
	pipelineToken = strtok(userInput, "|");
	
	while(pipelineToken != NULL)
	{
		strcpy(pipelineTokenArray[i++], pipelineToken);
	
		pipelineToken = strtok(NULL, "|");
	}

	
	for(int j = 0; j < i; j++)
	{		
		if((i == 2) && (j == 0)) //if "i" reached two then pipe is present
		{
			pipeSide = 1; //this token is on the left side of the pipe
		}
		if((i == 2) && (j == 1))
		{
			pipeSide = 2; //this token is on the right side of the pipe
		}

		tokenizeCommands(pipelineTokenArray[j]);
	}
	
}

/*
* This function takes each set of commands and tokenizes them based on being
* seperated by the ";".Each token is stored as a string in an array of strings.
* The IF ELSE structure in the forloop checks if the output of the argument is
* either sent through a pipe, or the output of the pipe serves as the input of
* the function. Each string will contain a distinct command + argument combo 
* to be sent to the tokenize function.
*/

void tokenizeCommands(char pipelineToken[MAX_CHAR])
{
	char* commandToken;
	
	char commandTokenArray[MAX_ARG][MAX_CHAR];
	
	char backgroundIndicatorArray[MAX_CHAR];
	
	int i = 0, a = 0;

/*
* This while structure determines is each set of commands is run in either
* the foreground or the background.
*/
	
	char backgroundString[MAX_CHAR];
	
	strcpy(backgroundString, pipelineToken);
	
	while(backgroundString[i] != '\0')
	{
		if(backgroundString[i] == '&')
		{
			backgroundIndicatorArray[a++] = '&';
		}
		if(backgroundString[i] == ';')
		{
			backgroundIndicatorArray[a++] = ';';
		}
		
		i++;
	}

	
	int j = 0;
	
	commandToken = strtok(pipelineToken, ";&"); //both run in forground
	
	pipeDirection = 0;
	
	while(commandToken != NULL)
	{	
		strcpy(commandTokenArray[j++], commandToken);

		commandToken = strtok(NULL, ";&");
	}
		
/*
* This if else structure adds an extra semicolon to complete the array
* since the user does not add one if their final command runs in the
* foreground.
*/
				
	if(a == 0 && j == 1)
	{
		backgroundIndicatorArray[a++] = ';';
	}
	else if(a < j)
	{
		backgroundIndicatorArray[a++] = ';';
	}
	
	for(int k = 0; k < j; k++)
	{
		if((pipeSide == 1) && (k == j - 1))
		{
			pipeDirection = 1; //into pipe
		}
		else if((pipeSide == 2) && (k == 0))
		{
			pipeDirection = 2; //out from pipe
		}
		else
		{
			pipeDirection = 0;
		}
		
		backgroundIndicator = backgroundIndicatorArray[k];
		
		tokenizeArguments(commandTokenArray[k]);
	}

}

/*
* This function takes each command + argument combo and tokenizes them based
* on white space. The command and subsequent arguments are saved in a pointer
* array which is then sent to be executed. If any of the tokens contains the 
* string "exit", the whole programme is exited.
*/

void tokenizeArguments(char commandToken[MAX_CHAR])
{
	char* argv[MAX_ARG];
	
	int i = 0;
	
	argv[i] = strtok(commandToken, " \n");
	
	while(argv[i] != NULL)
	{	
		argv[++i] = strtok(NULL, " \n");
	}
	
	for(int j = 0; j < i; j++)
	{
		if((strcmp(argv[j], "exit\n") == 0) || (strcmp(argv[j], "exit") == 0))
			{
				exit(1);
			}
	}	

	execute(argv);
}

/*
* This function forks a child to process the command while the parent waits
* for the child to finish. Based on the side of the pipe the command is coming
* from (determined in the previous two functions), either the output of the 
* "execvp" function goes into the pipe, or the input of the function is taken
* from the pipe.
*/

void execute(char* argv[MAX_ARG])
{
	pid_t pid = fork();
	
	if(pid == 0)
	{
		if(pipeDirection == 1) //if the output should go into the pipe
		{		
			close(fd[0]);
		
			dup2(fd[1], STDOUT_FILENO);
			
			execvp(argv[0], argv);
		}
		else if(pipeDirection == 2) //if the input should be taken from pipe
		{			
			close(fd[1]);
						
			dup2(fd[0], STDIN_FILENO);
			
			execvp(argv[0], argv);
		}
		else
		{
			close(fd[1]);
			
			close(fd[0]);
			
			execvp(argv[0], argv);
		}
	}
	else
	{
		if(pipeDirection == 2) //parent closes pipe after reading from it
		{
			close(fd[1]);
			
			close(fd[0]);
		}
		
		if(backgroundIndicator == ';')
		{
			waitpid(-1, NULL, 0); //waiting on children	
		}
	}
}













