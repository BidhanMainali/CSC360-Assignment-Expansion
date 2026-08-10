Reading through this assignmnet, the first thing I immedietly recognized is to parse our shell input.

Before that, I need to get important user information such as the users current working directory.

I did this by using getcwd(cwd,sizeof(cwd).  we store the working directory into a variable with a
fixed size.

One of the requriments was to get the hostname and login of the user. This was done via getlogin() and
gethostname() built in by the C library.


I combined login, hostname and the user shell prompt into 1 input vairable

the snprintf builds a string from the login, hostname, and cwd to create "shell" representation

example:  

getlogin:   bidhanmainali
hostname: linux202
cwd:  /home/bidhanmainali/CSC360/p1

prompt:  bidhanmainali@linux202: /home/bidhanmainali/CSC360/p1 > 

Next, the readline(prompt) will execute the stored prompt, wait for the user type something and press enter. On Enter, the the input that the user typed will be stored into a varaible for tokenizing.


This leads to our first function: parse_input(input,args)

This takes in our user-entred input and the args to store our tokens.

We will tokenize each input and tokenize it to our args for later use. We loop until there is nomore to parse and before returning count, we make the last argument NULL so when we run execvp later on, it will not crash.


if the user had entred "cd", change_directory() function will fire


change_directory(args)

Take in our args that we tokenized into arugments. We create a path variable to store and check for
cases.

If the first argument after cd is null (aka nothing), we just change their directory to its home
via cd(getenv("HOME")). 


If the input is "~", we also send them to the home directory the same way above.


if the input has a '~' and some directory, we merge it with the home directory and execute.

Finally, we have edge cases if the directory does not exit.




Next, the most imporant part of the assignment is  if the user inputed "bg". We fire
bg_execute(args)



bg_execute(args)


we take in our arguments. The first thing I did was for each  string, we put them into arguemnts
for ease of access. This was done in the while loop.




we create a pid vrabile and fork() to create a child proccess.

signal() will let you do Cntrl- C to kill proccess if needed.


execvp(args[1], args + 1) — runs the actual command. Uses args[1] not
  args[0] because args[0] is "bg" which isn't the command. args + 1 shifts
  the whole array forward by one, so the program sees the right arguments
  	- If the command doesn't exist, print error and exit


Parent (pid > 0):
  This is the shell. It saves the job info:

  1. malloc(sizeof(bg_job)) — create a new node in memory
  2. new_job->pid = pid — save the child's PID so you can check on it later
  3. strcpy(new_job->command, command) — save the command string
     for printing in bglist and terminated
  messages
  4. new_job->next = bg_head — point this node to the current first node
  5. bg_head = new_job — make this node the new first node

  So the parent just records the job and immediately returns to the prompt,
  while the child runs in the background.

  


next if the user types "bglist", fire bg_list()


This function will loop through the created linked list and outputs the total # of background jobs
that are currently running




But, if the proccess is finished, it would just sit there in memory and create leaks. To counter this
I created check_bg_jobs()



This function will loop through the list and find any proccess that has finished and remove them from
the linked list.
This can be done utiizing the WNOHANG, it returns 0 if the proccess running and if it != 0, proccess is finished.

the next couple of lines were simple linked list logic. If the procces is finished, I unlink them.





Lastly, we free(input) to prevent memory leaks.

For this assignment, i initally had all the logic above in 1 main which was very bad practice. 
This took me a while to rearange everything and put them into functions to make my code clean! 
