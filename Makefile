mysh: mysh.l mysh.y shell.c commands.c tailq.c utils.c shared.c
	bison -d mysh.y
	flex -o mysh.yy.c mysh.l
	gcc -Wall -Wextra shell.c mysh.tab.c mysh.yy.c utils.c shared.c -lfl -lreadline -o mysh