all : freq5

freq5: freq5.c
	gcc -Wall -std=c99 -g freq5.c -o freq5 
	
clean:
	rm freq5



