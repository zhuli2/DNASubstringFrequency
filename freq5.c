#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/time.h>

//DNA char includes "A", "C", "G", and "T".
//If a substring length is 5, then max number of the substrings is 4^5=1024. 
#define SUBSTR_NUM 1024
#define SUBSTR_LEN 5

/*declare custom methods */

/*open directory entered and then store file names to an array
 */
void open_dir(char * dir_name, char * files[], int * file_count_ptr);

/*count sub_strings with specific length and then store the counts to
  an array.
 */
void count_substr(char * file_name, int substr_count[], int base, int degree);
void compute_idx(int * str_idx_ptr, char sub_str[], int base, int degree);
int power(int base, int degree);

/*reverse sub_string from its array index
 */
void reverse_substr(int idx, char sub_str[], int base, int degree);

/*send sub_string count to stdout or file
 */
void output_count(int substr_count[], char *file_name, int base, int degree); 


int main(int argc, char * argv[]) { 
	
	/* receive arguments from command line.
	 */
	extern char * optarg;
	int opt;	  

	char *dir_name = NULL ;	
	int n = -100;
	char *outputfile = NULL;

	//options following the command: 
	//freq5 -d <file dir> -n <number of processes> -o <output file name>
	while ((opt = getopt(argc,argv,"d:n:o:")) != -1)
	{
		switch(opt)
		{
			case 'd':
				dir_name = optarg;
				break;
			case 'n':
				n = atoi(optarg);
				break;
			case 'o':
				outputfile = optarg;
				break;
			case '?':				
				break;
			default:				
				exit(1);		
		}	
	}
	
	/* verfify optional parameter -d entered 
	 */
	if (dir_name == NULL) //dir_name is required to run the program. 
	{
		fprintf(stderr, 
				"Usage: freq5 -d <input file dir>" 
							" [-n <number of processes>]"
							" [-o <output file name>]\n");
		exit(1);	
	}	
	strcat(dir_name, "/");
	//puts(dir_name);	
	
	/* declare and initialize an array containing file names,     
     * then open input dir to get and store file names to the 
     * array.
     */ 
    char * files[500]; 
    int file_count = 0;    
    open_dir(dir_name, files, &file_count);    

    
    /* declare and initialize an array containing counts.
     * size of array is pre-defined (1024) and each
     * index is corresponding to an unique sub_string.     
     */    
    int substr_count[SUBSTR_NUM];
   	for (int i=0; i < SUBSTR_NUM; i++)
   	{
   		substr_count[i] = 0;
   	}    
 	
 	int base = 4; //corresponding to number of DNA characters.
    int degree = 5; //corresponding to length of sub_string.
    
    /* verfify optional parameter -n entered and assign value to num_child 
     */
    int num_child; 
    if (n != -100)
    {
    	if (n <= 0)
    	{   			
    		fprintf(stderr, "ERROR: invalid number of processes\n");
    		exit(1);
    	} else if (n > file_count)
    	{
    		num_child = file_count;    	
    	} else
    	{
    		num_child = n;   	
    	}       
    } else
    {
    	num_child = 1;
    }  
       
    int index = 0; //for parent process
    int start_index = 0; //for child process   	    
    int reminder = file_count % num_child;    	
 
    int pid;//for values returned by fork(): 0-->child, >0-->parent, -1-->error.
    //int fd[2]; //for pipe(): fd[0]-->read from pipe, fd[1]-->write to pipe.
        
    puts("START TO WORK...");
    
    /*record running time of the program below
     */
    struct timeval starttime, endtime;
    double timediff;
    if ((gettimeofday(&starttime, NULL)) == -1)
	{
		perror("failed to run gettimeofday");
		exit(1);
	} 
	
	/*create pipes for each child process
	*/
	int pipes[num_child][2];
	for (int i =0; i < num_child; i++)
	{
		if (pipe(pipes[i]) == -1)
    	{
    		perror("pipe failed");
    	}    	
	
	}  	
 	
	/* create multiple child processes each of which 
	 * has a pipe with the parent 
	 */   
    for (int i=0; i < num_child; i++)
    {
    	/* try to assign number of files to each child as evenly as possible.
    	 */
    	int span = (int) file_count/num_child;  
    	if (reminder != 0)
    	{
    		span += 1;
    		reminder -= 1;
   		} else
   		{
   			span = (int) file_count/num_child;    
   		} 
    
		//create pipe for each child.    
    	/*
    	if (pipe(fd) == -1)
    	{
    		perror("pipe failed");
    	}
    	*/   	
    	
    	//creat child process
    	pid = fork();      	    	

   		switch(pid)
    	{
    		case -1:
    				perror("fork failed");    				
    				exit(1);
    		case 0: /*child process.*/ 
    			
    			//printf("<%d> is working\n", getpid());		
 		
    			/*read index of 1st file from parent through pipe.
    			 */    			
    			if ((read(pipes[i][0], &start_index, sizeof(int))) ==  -1)
    			{
    			 	perror("failed to read from pipe");
    			}   		 		
    			close(pipes[i][0]);  
    			
    			/*count sub_strings from input files and store to array.
    			 */  
    			for (int i = start_index; i < start_index + span; i++)
    			{    				
    				count_substr(files[i], substr_count, base, degree); 
    			}  			
				
    			/*write counts to parent through pipe.
    			 */
				
				/** Not appropriate to write results to the parent from the child
				    here because it blocks other pipes.
					The better way is to write after the loop of creating the 
					child processes.  
				**/
    			for (int i=0; i < SUBSTR_NUM; i++)
    			{      								  			
    				int ret = write(pipes[i][1], &substr_count[i], 
    									   			sizeof(substr_count[i]));
    				if (ret == -1)
    				{
    					perror("falied to write to pipe");
    				}     				 				
    			}
    			close(pipes[i][1]);     		
    			exit(0);  		   	

    		default:/*parent process*/    	
    			
    			/* write index of 1st file to child through pipe.
    			 */   			
    			if ((write(pipes[i][1], &index, sizeof(int))) == -1)
    			{
    				perror("failed to write");
    			}
    			index += span;
    					
     			close(pipes[i][1]);
    			wait(NULL);//wait until this child is terminted. 
    			
    			/*read counts from child through pipe
    			 */	
				
				 /** Not appropriate to read results from child to parent here.
				  	 The better way is to rea after the loop of creating child processes. 
				  **/
    			int buffer;    			
    			for (int i = 0; i < SUBSTR_NUM; i++)
    			{    				
    				int ret = read(pipes[i][0], &buffer, sizeof(buffer));				    					
    				if (ret == -1)
    				{
    					perror("falied to read from pipe");
    				}    				
    				substr_count[i] = buffer;
    				//N.T., pipe accumulates data (data chunk) by default.  
    			}
    			close(pipes[i][0]); 
     	}
    } 
   
	/* print result to stdout or store to an external file.
	 */   
    output_count(substr_count, outputfile, base, degree);
    
    if ( (gettimeofday(&endtime, NULL) ) == -1 )
	{
		perror("gettimeofday");
		exit(1);
	}
	timediff = (endtime.tv_sec - starttime.tv_sec) +
				(endtime.tv_usec - starttime.tv_usec) / 1000000.0;		
	fprintf(stderr, "<%d> processes used <%.4f> seconds\n", num_child, timediff);
	
	puts("END OF WORK!");  
    
    exit(0);
}


/*
 * help functions
 */

void open_dir(char * dir_name, char * files[], int * file_count)
{
	/* parameters-
	 *			dir_name: name of directory
	 *			files: array of file names
	 *			file_count: number of total files	  
	 * usage-
	 *			acces to the directory and then store and count 
	 * 			the files within.  
	 * return-
	 *			void  			 
	 */	
	 	
	char file[256];
	strncpy(file, dir_name, strlen(dir_name)+1);
	
	DIR *d;
	struct dirent *dir;
	
	d = opendir(dir_name);
	if (d == NULL)
	{
		perror("failed to open the dir");		
		exit(1);
	}	
	else
	{
		while ((dir = readdir(d)) != NULL)
		{
			//if (strstr(dir->d_name, ".dat") == NULL)
				//continue;
							
			strcat(file, dir->d_name);			
			
			files[*file_count] = malloc(256);					
			strcpy(files[*file_count], file);
			//puts(file);			
			
			*file_count += 1;			
			strncpy(file, dir_name, strlen(dir_name)+1);
		}
	} 
	closedir(d);	
}

void count_substr(char * file_name, int substr_count[], int base, int degree)
{
	/* parameters-
	 *			file_name: name of file that needs to read
	 *			substr_count: array of sub_string count
	 *			base: int used to compute index of each unique index
	 * 			degree: int used to compute index of each unique index
	 * usage-
	 *			open input file and read each line from which grabs each 
	 *			sub_string of lenght n and then stores count on it to the 
	 *			array per its index computed   
	 * return-
	 *			void  			 
	 */		
	 	
	int str_idx = -1;	
	char sub_str[SUBSTR_LEN + 1];	
	
	FILE * ipfile;
	char * original_line = NULL;
	size_t len = 0;	

	ipfile = fopen(file_name, "r");
	if (ipfile == NULL)
	{
		perror("falied to open the file");
		exit(1);
	}
	
	/*read line by line from input file
	 */	
	int read = 0;
	while ((read = getline(&original_line, &len, ipfile)) != -1)
	{
		//convert line of string to array of char.		
		int line_len = strlen(original_line);
		char line[line_len];
		strcpy(line, original_line);		
		
		//creat sub_string with specific length n
		for (int i=0; i < line_len - (SUBSTR_LEN - 1); i++)
		{						
			int beg = 0;
			sub_str[beg] = line[i];			
							
			for (int j = i+1; j < i + SUBSTR_LEN; j++)
			{
				beg += 1;			
				sub_str[beg] = line[j];
			}					
			
			//compute index of substring and store count to array			
			compute_idx(&str_idx, sub_str, base, degree);			
			if (str_idx >= 0)
			{				
				substr_count[str_idx] += 1;
			}
		}			
	}	
	free(original_line);
	fclose(ipfile);		
}


void compute_idx(int * str_idx, char sub_str[], int base, int degree)
{
	/* parameters-
	 *			str_idx: index of an unique sub_string in the array
	 *			sub_str: sub_string needed to compute index
	 *			base: int used to compute index of each unique index
	 * 			degree: int used to compute index of each unique index
	 * usage-
	 *			compute index of an unique sub_string based on its 
	 *          characters ({'A', 'C', 'G', 'T'})   
	 * return-
	 *			void  			 
	 */
	 
	 /**Assuming base = 4 (number of char for a substring) and degree = 5 (substring length),
		then max index of substring = 4^5 - 1 = 1024 -1 = 1023.
		The substring can be divided into 4 categories: {"A...", "C...","G...", and "T..."}
		where each category has the same size 4^4=256.
		The indices for each categore is: 
		"A...":   0,...,  4^4-1= 255,
		"B...": 256,...,255+256= 511,
		"C...": 512,...,511+256= 767,
		"D...": 767,...,767+256=1023.
		
		For a substring "AAAAA", its index is computed as:
		"A....": index <= 1023 - 3*(4^4) = 255 because there are 3 categories after "A..." and 4 more char. 
		"AA...": index <=  255 - 3*(4^3) =  63 because there are 3 categories after "A..." and 3 more char.
		"AAA..": index <=   63 - 3*(4^2) =  15 because there are 3 categories after "A..." and 3 more char.
		"AAAA.": index <=   15 - 3*(4^1) =   3 because there are 3 categories after "A..." and 1 more char.
		"AAAAA": index <=    3 - 3*(4^0) =   0 because there are 3 categories after "A..." and 0 more char.
	 **/

	*str_idx = power(base, degree);		
	
	//N.T., len of sub_str includes null terminator at the end.	
	for (int i = 0; i < SUBSTR_LEN; i++)
	{
		//printf("char-> %c\n", sub_str[i]);		
		if (sub_str[i] == 'A' || sub_str[i] == 'a')
		{
			*str_idx -= 3*power(base, degree-1);					
			degree -=1;
			continue;			
		} 
		else if (sub_str[i] == 'C' || sub_str[i] == 'c')
		{
			*str_idx -= 2*power(base, degree-1);
			degree -=1;
			continue;
		} 
		else if (sub_str[i] == 'G' || sub_str[i] == 'g')
		{
			*str_idx -= 1*power(base, degree-1);
			degree -=1;
			continue;	
		} 
		else if (sub_str[i] == 'T' || sub_str[i] == 't') 
		{		
			*str_idx -= 0*power(base, degree-1);
			degree -=1;
			continue;								
		} 
		else
		{
			//fprintf(stderr, "Invalid sub_string <%s>!\n", sub_str);		
			*str_idx = -1;
			break;		
		}					
	}		
	*str_idx -= 1;
	//printf("index = %d\n", *str_idx);		
}


int power(int base, int degree)
{
	/* parameters-	 
	 *			base: int used to compute index of each unique index
	 * 			degree: int used to compute index of each unique index
	 * usage-
	 *			compute the value of base^(degree)   
	 * return-
	 *			int  			 
	 */	
	 	 		
	int result = 1;
	int i = 1;
	while (i <= degree)
	{
		result *= base;
		i += 1;
	}	
	return result;
}

void reverse_substr(int index, char substr[], int base, int degree)
{
	/* parameters-
	 *			index: index of an unique sub_string in the array
	 *			substr: an unique sub_string
	 *			base: int used to compute index of each unique index
	 * 			degree: int used to compute index of each unique index
	 * usage-
	 *			convert back to characteristic sub_string from its 
	*           corresponding index
	 *			in the array.   
	 * return-
	 *			void  			 
	 */	
	 
	 /**the index is computed for every char in the substring as shown above using the base.
		So, reverse the process to find corresponding char based on reminder: 
		0:"A", 1:"C", 2:"G", 3:"T".
		
		For example, if index = 1023, its corresponding substring is computed as:
		reminder = (1023/4^0)/4 = 3 => "....T"
		reminder = (1023/4^1)/4 = 3 => "...TT"
		reminder = (1023/4^2)/4 = 3 => "..TTT"
		reminder = (1023/4^3)/4 = 3 => ".TTTT"
		reminder = (1023/4^4)/4 = 3 => "TTTTT"	  
	  **/
	 
	int reminder;	
	for (int i = 0; i < degree; i++)
	{
		//the conversion starts from least significant position of the index
		reminder = ((int)index/power(base, i)) % base;		
		switch(reminder)
		{
			case 0:
					substr[degree-1-i] = 'a';
					continue;					
			case 1:
					substr[degree-1-i] = 'c';
					continue;					
			case 2:
					substr[degree-1-i] = 'g';
					continue;					
			case 3:
					substr[degree-1-i] = 't';
					continue;	
		}		
	}
	substr[degree] = '\0';	
}


void output_count(int substr_count[], char * file_name, int base, int degree)
{
	/* parameters-
	 *			substr_count: array of count on each unique sub_string
	 *			file_name: file used to contain the count
	 *			base: int used to compute index of each unique index
	 * 			degree: int used to compute index of each unique index
	 * usage-
	 *			ouput the count to an external file; otherwise, print to stdout   
	 * return-
	 *			void  			 
	 */				
	
	FILE * opfile;	
    if (file_name != NULL)
    {
    	opfile = fopen(file_name, "w+");
    	if (opfile == NULL)
    	{
    		perror("failed to reach the file");
    		exit(1);
    	}   	
   	 } 
   	    	 	 	
    char substr[degree + 1];       
   	for (int i=0; i < SUBSTR_NUM; i++)
   	{  
   		reverse_substr(i, substr, base, degree);
   		if (file_name == NULL)
   		{  	
   			if (substr_count[i] > 0)
   			{	
   				printf("%s, %d\n", substr, substr_count[i]);
   			}
   			continue;
   		} 
   		else
   		{    			
   			fprintf(opfile, "%s,%d\n", substr, substr_count[i]);   			
   		}   			 
   	 }
   	 
   	 if (file_name != NULL)
   	 {
		fclose(opfile);
	}   	
}
