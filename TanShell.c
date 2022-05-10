/*
author: 20373864 谭立德 tanlide 
*/ 

// 库函数
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// 全局变量
int oldin, oldout; // 标准输入输出 
char *buff[105]; // 保存输入指令
char home[105]; // 保存家目录
int fdCnt; // 重定向记录
int count; // 管程数记录

// 函数定义
void preWork(); // Shell运行准备工作
void printPath(); // 输出路径，判断并进行路径压缩
void parse(char *buff); // 解析输入
void doWork(int argc, char *argv[]); // 根据解析的输入调用对应指令

// main函数
int main() {
	preWork();
	// puts(home);
	while (1) {
		printPath();
		buff[0] = (char *)malloc(105 * sizeof(char));
		fgets(buff[0], 105, stdin);
		buff[0][strlen(buff[0]) - 1] = 0;
		if (strcmp(buff[0], "quit") == 0) { // 退出
			break;
		}
		// 处理管道
		char *index;
		int cnt = 0;
		int myout = STDOUT_FILENO;

		count = 0;
		index = buff[count];
		// 统计管道数量 
		while ((index = strstr(buff[count], "|")) != NULL) {
			char buf[105];
			count++;
			buff[count] = (char *)malloc(105 * sizeof(char));
			strcpy(buff[count], (index + 1));
			*index = '\0';
		}

		if (count != 0) {
			int *fd;
			fd = (int *)malloc(2 * count * sizeof(int));
			cnt = 0;
			// 统一开启管道 
			while (cnt < count) {
				if (pipe(fd + 2 * cnt) < 0) {		
					fprintf(stderr, "Tan's Shell Error: Unsuccessful create the pipe\n");
					exit(1);
				}
				cnt++;
			}
			pid_t pid;
			// 管道输入，标准输出
			cnt = count; 
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "Tan's Shell Error: Unsuccessful fork\n");
				exit(1);
			}
			if (pid == 0) {
				dup2(fd[cnt * 2 - 2], STDIN_FILENO);
				int i;
				for (i = 0; i < count; i++) {
					if (i != cnt - 1) {
						close(fd[2 * i]);
					}
					if (i != cnt) {
						close(fd[2 * i + 1]);
					}
				}
				parse(buff[cnt]); 
				exit(0);
			}
			cnt--; 
			// 管道输入输出 
			while (cnt > 0) {
				pid = fork();
				if (pid < 0) {
					fprintf(stderr, "Tan's Shell Error: Unsuccessful fork\n");
					exit(1);
				}
				if (pid == 0) {
					dup2(fd[cnt * 2 - 2], STDIN_FILENO);
					int i;
					for (i = 0; i < count; i++) {
						if (i != cnt - 1) {
							close(fd[2 * i]);
						}
						if (i != cnt) {
							close(fd[2 * i + 1]);
						}
					}
					dup2(fd[cnt * 2 + 1], STDOUT_FILENO);
					parse(buff[cnt]);
					exit(0);
				}
				cnt--;
			}
			// 标准输入，管道输出 
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "Tan's Shell Error: Unsuccessful fork\n");
				exit(1);
			}
			if (pid == 0) {
				dup2(fd[1], STDOUT_FILENO);
				int i;
				for (i = 0; i < count; i++) {
					if (i != cnt - 1) {
						close(fd[2 * i]);
					}
					if (i != cnt) {
						close(fd[2 * i + 1]);
					}
				}
				parse(buff[0]);
				exit(0);
			}
			
			// 关闭所有管道 
			int i;
			for (i = 0; i < count; i++) {
				close(fd[2 * i]);
				close(fd[2 * i + 1]);
			}
			// 等待子进程结束
			for (i = 0; i <= count; i++) {
				wait(NULL);
			}
		} else {
			parse(buff[cnt]);
		}
	}
}

// 函数实现
void preWork() {
	// 指定初始标准输入输出和异常输出
	oldin = dup(STDIN_FILENO);
	oldout = dup(STDOUT_FILENO);
	// 身份标示信息 20373864 谭立德 
	fprintf(stdout, "\n\n##########  Tan's Shell  ##########\n\n\n");
	// 获得程序运行路径，视为家目录 
	getcwd(home, sizeof(home));
	fdCnt = 0;
}

void printPath() {
	int i;
	for (i = 5; i < fdCnt + 5; i++) {
		close(i);
	}
	fdCnt = 0;
	fflush(stdout); // 清空缓冲区 
	dup2(oldin, STDIN_FILENO);
	dup2(oldout, STDOUT_FILENO);
	char *path = (char *)malloc(105 * sizeof(char));
	getcwd(path, 105 * sizeof(char)); // 获取当前路径
	// 路径压缩 
	char *index = NULL;
	index = strstr(path, home); // 查询是否包含家目录，如果是则压缩路径
	if (index != NULL) {
		char *newPath = (char *)malloc(105 * sizeof(char));
		*newPath = '~';
		strcpy((newPath + 1), (path + strlen(home)));
		path = newPath;
	}
	fprintf(stdout, "Tan's Shell %s$ ", path);
	fflush(stdout); // 清空缓冲区 
}

void parse(char *buff) {
	int argc; // 保存指令参数数量
	char *argv[105]; // 保存指令参数
	char command[105]; // 保存指令类型
	strcpy(command, buff);
	int i, j, len;
	len = strlen(command);
	argc = 0;
	argv[argc] = (char *)malloc(105 * sizeof(char));
	int flag = 0;
	// 逐个读取参数，并处理输入输出重定向
	for (i = 0, j = 0; i < len; i++) {
		if (command[i] == ' ') {
			if (j != 0) {
				argv[argc][j] = '\0';
				if (flag == 1) { // 重定向输入
					int fd = open(argv[argc], O_RDONLY);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDIN_FILENO);
					fdCnt++;
				} else if (flag == 2) { // 重定向输出
					int fd = open(argv[argc], O_WRONLY | O_CREAT | O_TRUNC, 0x777);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot create the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDOUT_FILENO);
					fdCnt++;
				} else if (flag == 3) { // 重定向输出（追加）
					int fd = open(argv[argc], O_WRONLY | O_APPEND);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDOUT_FILENO);
				}
				flag = 0;
			}
			if (strcmp(argv[argc], "<") == 0) { // 输入重定向
				flag = 1;
			} else if (strcmp(argv[argc], ">") == 0) { // 输出重定向
				flag = 2;
			} else if (strcmp(argv[argc], ">>") == 0) { // 输出重定向（追加）
				flag = 3;
			}
			if (flag == 0 && j != 0) {
				argc++;
				argv[argc] = (char *)malloc(105 * sizeof(char));
			}
			j = 0;
			while(command[i] == ' ' && i < len) {
				i++;
			}
			if (i == len) break;
		}
		argv[argc][j++] = command[i];
	}
	if (j != 0) {
		argv[argc][j] = '\0';
		if (flag == 0) {
			argc++;
		} else if (flag == 1) { // 重定向输入
			int fd = open(argv[argc], O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
			}
			dup2(fd, STDIN_FILENO);
			fdCnt++;
		} else if (flag == 2) { // 重定向输出
			mode_t power = 0777;
			int fd = creat(argv[argc], power);
			if (fd < 0) {
				fprintf(stderr, "Tan's Shell Error: Cannot create the file: '%s'\n", argv[argc]);
			}
			dup2(fd, STDOUT_FILENO);
			fdCnt++;
		} else if (flag == 3) { // 重定向输出（追加）
			int fd = open(argv[argc], O_WRONLY | O_APPEND);
			if (fd < 0) {
				fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
			}
			dup2(fd, STDOUT_FILENO);
		}
	}
	argv[argc] = NULL;
	doWork(argc, argv);
}

void doWork(int argc, char *argv[]) {
	int i;
	// 空输入
	if (argc == 0) {
		return;
	}
	// 处理指令
	if (strcmp(argv[0], "type") == 0) { // 'type'指令，识别一个指令是内部指令还是外部指令 
		char *ins[105] = {"type", "cd", "pwd", "chmod", NULL}; 
		int i;
		i = 0;
		while (ins[i] != NULL) {
			if (strcmp(ins[i], argv[1]) == 0) {
				fprintf(stdout, "%s is a Tan's Shell builtin\n", argv[1]);
				return;
			}
			i++;
		}
		fprintf(stdout, "%s is not a Tan's Shell builtin\n", argv[1]);
	} else if (strcmp(argv[0], "cd") == 0) { // 'cd'指令
		// 还原路径压缩： 替换 ~ 为家目录路径
		char *index; 
		if ((index = strstr(argv[1], "~")) != NULL) {
			char buf[105];
			strcpy(buf, index + 1);
			*index = '\0';
			strcat(argv[1], home);
			strcat(argv[1], buf);
		}
		if (chdir(argv[1]) != 0) { // 使用'chdir' 
			fprintf(stderr, "Tan's Shell Error: Cannot change directory to: %s\n", argv[1]);
		} else {
			fprintf(stdout, "Tan's Shell: Successfully change the directory!\n");	
		}
	} else if (strcmp(argv[0], "pwd") == 0) { // 'pwd'指令
		char *path = (char *)malloc(105 * sizeof(char));
		if (getcwd(path, 105 * sizeof(char)) == NULL) { // 使用'getcwd'
			fprintf(stderr, "Tan's Shell Error: Failed to obtain the path\n");
		} else {
			fprintf(stdout, "Tan's Shell: Successfully obtain the work directory: %s\n", path);	
		}
	} else if (strcmp(argv[0], "chmod") == 0) { // 'chmod'指令 
		mode_t mod;
		int i;
		for (i = 0, mod = 0; i < strlen(argv[2]); i++) {
			mod += (i - '0') + mod * 8;
		}
		if (chmod(argv[1], mod) < 0) { // 使用'chmod' 
			fprintf(stderr, "Tan's Shell Error: Fail to change the mode of %s\n", argv[1]);
		} else {
			fprintf(stdout, "Tan's Shell: Successfully change the file mode\n");	
		}
	} else { // 其他命令（默认为外部命令）
		pid_t pid = fork();
		if(pid < 0) {
			fprintf(stderr, "Tan's Shell Error: Unsuccessful fork\n");
			exit(1);
		}
		if(pid == 0) {
			if (execvp(argv[0], argv) < 0) {
				fprintf(stderr, "Tan's Shell Error: Unsupported Instruction!!!\n");
				exit(1);
			}
		}
		if (pid > 0) {
			wait(NULL);
		}
	}
}
