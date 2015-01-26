#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

extern char **getline(void);
char *concat(char* a, char* b);
char *which(char* cmd);
int array_length(char** array);
void shell_pipe2(char** command, int save[]);
void redirect_output(char** LHS, char* filename);
void print_array(char ** array, char* caller);
void standard_exec(char** command, int save[], int original[]);
int get_cmd_end(char** RHS_start);
void redirect_input(char** LHS, char* filename);
void parseargs(char** args);
int begin_first_cmd(char** args);
int prefix_strcmp(char* arg, char* match);
void strict_exec(char** args);

/*continually gets new lines from stdin,
avoids breaking if user enters nothing*/
int main(void) {
	char** args;
	while(1) {
		printf("SEXY_SHELL# ");
		args = getline();
		if (args[0]=='\0')continue;
		parseargs(args);
	}
}

/*main algorithm for parsing arguments and infinite pipes*/
void parseargs(char** args){
	char** LHS; 
	char** RHS;
	char* filename;
	char* char_save;
	int i, end, execute_first_flag, c;
	int save[2];
	int original[2];
	FILE* stream;
	i = 0;
	execute_first_flag = begin_first_cmd(args);
	/*execute the first argument in the case of pipe or single cmd*/
	if(execute_first_flag){ 
       end = get_cmd_end(args);
       char_save = args[end];
       args[end] = '\0';
       standard_exec(args, save, original);
       args[end] = char_save;
       i = end;
       if(char_save == '\0'){
          stream = fdopen (save[0], "r");
          while ((c = fgetc (stream)) != EOF)
             putchar (c);
          dup2(original[0], 0);
   	      dup2(original[1], 1);
	      close(stream);
       }
    }
    /*parse additional commands and their arguments*/
    /*this is done by parsing a line into LHS <event char> RHS*/
    /*in the case of pipe, you would have LHS <pipe> RHS*/
    /*in the case of redirect, you would have LHS <redir> file*/
    while(args[i] != NULL){
		switch(*args[i]){
			case '|': 
               /*execute start to the pipe. save it into stdout of w/e.*/
               end = get_cmd_end(args+i+1) + i+1;
               /*if this print is commented out the program won't work*/
               printf("");
               char_save = args[end];
               args[end] = '\0';
               shell_pipe2(args+i+1, save);
               args[end] = char_save;
               i = end;
			break;
			case '>':
			/*if we find a redirect output, grab the left hand side
			and the filename and pass them into a function to process
			the redirect*/
			   args[i] = '\0';
			   LHS = args;
			   filename = args[i+1];
			   redirect_output(LHS,filename);
			break;
			case '<':
			/*if we find a redirect input, grab the left hand side
			and the filename and pass them into a function to process
			the redirect*/
			   args[i] = '\0';
			   LHS = args;
			   filename = args[i+1];
			   redirect_input(LHS,filename);
			break;
			default: 
               /*manually search for cd, exit, pwd. do an exec without a fork*/
			   if(prefix_strcmp(args[0], "cd")){
			   	  chdir(args[1]);
			   }else if(prefix_strcmp(args[0], "pwd")){			  
			   	  strict_exec(args);
			   }else if(prefix_strcmp(args[0], "exit")){			   	 
			   	  exit(0);
			   }
			   ++i;
			   break;
		}
	}
	/*print from stream when needed*/
	if(execute_first_flag){
	   /*printf("Printing final output: \n");*/
	   stream = fdopen (save[0], "r");
       while ((c = fgetc (stream)) != EOF) {
    	   if(c == '\0'){
    	      printf("NULL FOUND\n");
    	   }
           putchar (c);
       }
       /*printf("The end.\n");*/
    }
    /*clean up stdin and stdout*/
    dup2(original[0], 0);
	dup2(original[1], 1);
}

/*execs a command without a fork*/
void strict_exec(char** args){
	char* cmd;
	cmd = which(args[0]);
    execv(cmd, args);
}

/*
Function takes in 2 character arrays called arg and match.
It checks to see if the prefix of arg is equal to match.
If there is a match, it will return 1.
*/
int prefix_strcmp(char* arg, char* match){
   int i;
   i = 0;
   while(arg[i]!='\0' && match[i]!='\0'){
      if(arg[i]!=match[i]){
      	  return 0;
      }
      ++i;
   }
   /*check to see if we have reached the end of match. If yes, then we know that
     the prefixs match*/
   if(match[i]=='\0'){
   	  return 1;
   }
}

/*

*/
int begin_first_cmd(char** args){
   int end;
   end = get_cmd_end(args);
   if(prefix_strcmp(args[0],"cd")||prefix_strcmp(args[0],"pwd")||prefix_strcmp(args[0],"exit")){
   	  return 0;
   }
   if(args[end] == NULL){
   	  return 1;
   }else if(strcmp(args[end],"|") == 0){
   	  return 1;
   }
   return 0;
}

/*finds special characters and returns the index*/
int get_cmd_end(char** cmd_start){
   int index;
   char* word;
   index = 0;
   while(cmd_start[index]!=NULL && *cmd_start[index]!='|' && *cmd_start[index]!='<' && *cmd_start[index]!='>' ){
      ++index;
   }
   return index;
}

void standard_exec(char** command, int save[], int original[]){ 
	if(command[0] != '\0'){ /* I can't return if null for some reason*/
		pid_t pid;
		char* cmd;
		FILE* stream;
		int c;
		cmd = which(command[0]);
		pipe(save);
		pid = fork();
		if(pid == 0){
			close(save[0]); /* close pipe read, we are not using it */
			original[1] = dup(1);
			close(1); /* close std_out so we can dup it */
	        dup2(save[1], 1); /*std_out (the output of which) -> pipe write */
			execv(cmd, command); /*This is the difference*/
		}else{
			close(save[1]); /* close pipe write, we are not using it */
			original[0] = dup(0);
			close(0);
		    dup2(save[0], 0);
			wait(&pid);
		}
    }
}


/*This function reads from the stdin to get the arguments*/
void shell_pipe2(char** command, int save[]){
	char* cmd;
	pid_t pid;
	FILE* stream;
	char c;
	int fd[2];
	cmd = which(command[0]);
	pipe(fd);
	pid = fork();
	if(pid == 0){
   	/*READ FROM PIPE!!*/
		close(fd[0]);
		/*c = dup(0);*/
        stream = fdopen (save[0], "r");
        close(1);
        dup2(fd[1],1);
        execv(cmd, command); /*stream*/
	}else{
		close(fd[1]);
		close(0);
		dup2(fd[0],0);
		wait(&pid);
		save[0] = fd[0];
		save[1] = fd[1];
		close(stream);
	}
}

/*
Given a result and a command, where the result is additional arguments for the command,
it will execute the command with the additional args (result) and it will return
the output of that call.
*/
void redirect_output(char** LHS, char* filename){
	char* cmd;
	pid_t pid;
	FILE* fp;
	fp = fopen(filename, "w+");
	/*print_array(LHS, "rd_out_LHS");*/
	cmd = which(LHS[0]);
	pid = fork();
	if (pid == 0){
		fflush(stdout);
		dup2(fileno(fp), 1);
		execv(cmd, LHS);
	}else{

		wait(&pid);
		fclose(fp);
	}
}

/*
redirects the input to be from a file instead of stdin
*/
void redirect_input(char** LHS, char* filename){
	char* cmd;
	pid_t pid;
	FILE* fp;
	fp = fopen(filename, "rw+");
	cmd = which(LHS[0]);
	pid = fork();
	if (pid == 0){
		fflush(stdin);
		dup2(fileno(fp), 0);
		execv(cmd, LHS);
	}else{
		wait(&pid);
		fclose(fp);
	}
}

/*concatinates a string*/
char* concat(char* a, char* b){
	char *c = (char *) malloc(1 + strlen(a)+ strlen(b));
	strcpy(c, a);
	strcat(c, b);
	return c;
}

/*prints an array*/
void print_array(char ** array, char* caller){
	int i;
	i=-1;
	printf("------ARRAY_PRINTER<%s>\n", caller);
	while(array[++i] != NULL)printf("ARRAY[%d]: %s\n", i, array[i]);
	printf("------END_ARRAY_PRINTER<%s>\n", caller);

}
/*is this assuming that the first index is not null?*/
int array_length(char** array){
	int count = 0; 
	while(array[++count] != NULL);
	return count;
}

/*uses builtin which to determine the full path of a given command*/
char* which(char* cmd){
   	int fd[2];
   	int nbytes, i, ln;
   	pid_t childpid;
	char readbuffer[80]; /*why do we have a limit of 80? can we redesign to prevent it from overflowing?*/
   	char *c;
   	for(i = 0; i<80; i++){
   		readbuffer[i] = '\0'; 
   	}
   	pipe(fd);
   	childpid = fork();
   	if(childpid == -1){
   		perror("fork");
   		exit(1);
   	}
    /*Whatever is written to fd[1] will be read from fd[0].*/
   	if(childpid == 0){
		close(fd[0]); /* close pipe read, we are not using it */
		close(1); /* close std_out so we can dup it */
		dup2(fd[1], 1); /*std_out (the output of which) -> pipe write */
        execl("/usr/bin/which", "/usr/bin/which", cmd ,NULL); /* now the output is in our pipe*/
   	}else{
		close(fd[1]); /* close pipe write, we are not using it */
   		wait(&childpid);
        /* put the contents of fd[0] into readbuffer*/
   		/* printf("Size of buffer is %d\n",sizeof(readbuffer)); */
   		nbytes = read(fd[0], readbuffer, sizeof(readbuffer)); 
   	}
    c = strrchr(readbuffer, '\n'); /* strip \n which is added*/
   	if (c != NULL){
   		*(c) = '\0'; 
   	} 
   	return concat("",readbuffer); 
}