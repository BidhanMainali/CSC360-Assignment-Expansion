#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>


// Takes in our shell input and parses them into args
int parse_input(char *input, char *args[]){
	int count = 0;
	char *token;

	token = strtok(input," ");

	while(token != NULL){

		args[count] = token;
		count++;
		token = strtok(NULL, " ");
		
		
	}


	args[count] = NULL;  //Needed for execvp to prevent memory crash
	return count;


}

//Runs the command for the user input
void execute_command(char *args[]){

	int pid = fork();



}



int main(){

	char prompt[1024];
	char cwd[512];
	char hostname[256];
	char *username;
	char *args[64];
	


	username = getlogin();
	
	gethostname(hostname, sizeof(hostname));

	while (1){

		getcwd(cwd,sizeof(cwd));
		snprintf(prompt, sizeof(prompt), "%s@%s: %s > ", username, hostname, cwd);

		char *input = readline(prompt);

		if (input == NULL){
			break;
		}
		
	
		if (input[0] == '\0'){
			free(input);
			continue;
		}



		
	
	}



}
