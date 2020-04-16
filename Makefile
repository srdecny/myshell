mysh: mysh.l mysh.y shell.c commands.c datastructs.c utils.c 
	bison -d mysh.y
	flex -o mysh.yy.c mysh.l
	gcc -Wall -Wextra shell.c mysh.tab.c mysh.yy.c utils.c -lfl -lreadline \
		-o mysh

clean:
	-rm -f mysh.tab.* mysh.yy.c mysh
