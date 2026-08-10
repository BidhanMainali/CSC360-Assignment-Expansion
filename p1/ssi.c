#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <signal.h>


typedef struct bg_job {
	pid_t pid;
	char command[510];
	struct bg_job *next;
}bg_job;

bg_job *bg_head = NULL; //start with the head initized to null



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

	pid_t pid = fork();

	if (pid == 0){
		signal(SIGINT,SIG_IGN); //ignores Ctrl -C
		if (execvp(args[0],args) == -1){

			fprintf(stderr, "%s: No such file or directory\n", args[0]);
			exit(1);
		}


	} else if (pid > 0){
		int status; // Store child exit status
		waitpid(pid,&status,0);

	} else{

		perror("fork has failed");
		exit(1);

	}



}





void change_directory(char *args[]) {
      char path[512];
      if (args[1] == NULL) {  // If the input after cd is empty
          chdir(getenv("HOME"));

      } else if (strcmp(args[1], "~") == 0) {  // home command
          chdir(getenv("HOME"));

      } else if (args[1][0] == '~') {
          snprintf(path, sizeof(path), "%s%s", getenv("HOME"), args[1] + 1); //megre home with the directory
          if (chdir(path) != 0) {
              fprintf(stderr, "ssi: cd: %s: No such file or directory\n",args[1]);
          }

      } else {
          if (chdir(args[1]) != 0) {
              fprintf(stderr, "ssi: cd: %s: No such file or directory\n",args[1]);
          }
      }
  }




void bg_execute(char *args[0]){
	
	char command[510] = ""; //empty start

	int i = 1;

	while(args[i] != NULL){
	
		strcat(command,args[i]);
		if(args[i+1] != NULL){
			strcat(command, " ");// add space
		
		
		}

		i++;
	
	
	}

	pid_t pid = fork();


       
	  if (pid == 0) {
      signal(SIGINT, SIG_DFL); //kill the proccess
      if (execvp(args[1], args + 1) == -1) {
          fprintf(stderr, "%s: No such file or directory\n", args[1]);
          exit(1);
      }
  }else if (pid > 0) {
		bg_job *new_job = malloc(sizeof(bg_job)); //create new node
		new_job ->pid = pid;
		strcpy(new_job ->command, command); //fill in the string
		new_job ->next = bg_head; //add to the current head of list
		bg_head = new_job; //make that the new head
	
	}

}


void bg_list(){ //count the # of background jobs

	int count = 0;
	bg_job *current = bg_head;

	while(current != NULL){
		printf("%d: %s\n", current->pid, current->command);
		count++;
		current = current->next;
	
	
	}

	printf("Total Background jobs: %d\n", count);



}

  void check_bg_jobs() { // Goes through entire linked list, check for each proccess that finish.
	  		 // and remove that node
      bg_job *current = bg_head;
      bg_job *prev = NULL;
      int status;

      while (current != NULL) {
          if (waitpid(current->pid, &status, WNOHANG) > 0) {  //check if the proccess finished
              // finished — print and remove
              printf("%d: %s has terminated.\n", current->pid,current->command); //print to user

              if (prev == NULL) {
                  bg_head = current->next;
              } else {
                  prev->next = current->next; //if in the middle or end
              }

              bg_job *temp = current; //save current node, move forward and free first
              current = current->next;
              free(temp);

          } else {
              // still running — move forward
              prev = current;
              current = current->next;
          }
      }
  }




int main(){

	char prompt[1024];
	char cwd[512];
	char hostname[256];
	char *username;
	char *args[64];
	


	username = getlogin();
	
	gethostname(hostname, sizeof(hostname));
	signal(SIGINT,SIG_IGN);//ignores Ctrl -C
	while (1){
		
		check_bg_jobs();
		getcwd(cwd,sizeof(cwd));
		snprintf(prompt, sizeof(prompt), "%s@%s: %s > ", username, hostname, cwd);

		char *input = readline(prompt);

		if (input == NULL){  //if input is null
			break;
		}
		
	
		if (input[0] == '\0'){
			free(input);
			continue;
		}

		
		parse_input(input,args);

		if(strcmp(args[0], "cd") == 0){
			change_directory(args);
			
		} else if (strcmp(args[0], "bg") == 0){
			bg_execute(args);

			
		
		}else if (strcmp(args[0], "bglist") == 0){
			bg_list();
		
		
		} else{
			execute_command(args);
		
		
		
		}

		free(input);



		
	
	}



}
