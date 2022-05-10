/*
author: 20373864 ̷���� tanlide 
*/ 

// �⺯��
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// ȫ�ֱ���
int oldin, oldout; // ��׼������� 
char *buff[105]; // ��������ָ��
char home[105]; // �����Ŀ¼
int fdCnt; // �ض����¼
int count; // �ܳ�����¼

// ��������
void preWork(); // Shell����׼������
void printPath(); // ���·�����жϲ�����·��ѹ��
void parse(char *buff); // ��������
void doWork(int argc, char *argv[]); // ���ݽ�����������ö�Ӧָ��

// main����
int main() {
	preWork();
	// puts(home);
	while (1) {
		printPath();
		buff[0] = (char *)malloc(105 * sizeof(char));
		fgets(buff[0], 105, stdin);
		buff[0][strlen(buff[0]) - 1] = 0;
		if (strcmp(buff[0], "quit") == 0) { // �˳�
			break;
		}
		// ����ܵ�
		char *index;
		int cnt = 0;
		int myout = STDOUT_FILENO;

		count = 0;
		index = buff[count];
		// ͳ�ƹܵ����� 
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
			// ͳһ�����ܵ� 
			while (cnt < count) {
				if (pipe(fd + 2 * cnt) < 0) {		
					fprintf(stderr, "Tan's Shell Error: Unsuccessful create the pipe\n");
					exit(1);
				}
				cnt++;
			}
			pid_t pid;
			// �ܵ����룬��׼���
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
			// �ܵ�������� 
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
			// ��׼���룬�ܵ���� 
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
			
			// �ر����йܵ� 
			int i;
			for (i = 0; i < count; i++) {
				close(fd[2 * i]);
				close(fd[2 * i + 1]);
			}
			// �ȴ��ӽ��̽���
			for (i = 0; i <= count; i++) {
				wait(NULL);
			}
		} else {
			parse(buff[cnt]);
		}
	}
}

// ����ʵ��
void preWork() {
	// ָ����ʼ��׼����������쳣���
	oldin = dup(STDIN_FILENO);
	oldout = dup(STDOUT_FILENO);
	// ��ݱ�ʾ��Ϣ 20373864 ̷���� 
	fprintf(stdout, "\n\n##########  Tan's Shell  ##########\n\n\n");
	// ��ó�������·������Ϊ��Ŀ¼ 
	getcwd(home, sizeof(home));
	fdCnt = 0;
}

void printPath() {
	int i;
	for (i = 5; i < fdCnt + 5; i++) {
		close(i);
	}
	fdCnt = 0;
	fflush(stdout); // ��ջ����� 
	dup2(oldin, STDIN_FILENO);
	dup2(oldout, STDOUT_FILENO);
	char *path = (char *)malloc(105 * sizeof(char));
	getcwd(path, 105 * sizeof(char)); // ��ȡ��ǰ·��
	// ·��ѹ�� 
	char *index = NULL;
	index = strstr(path, home); // ��ѯ�Ƿ������Ŀ¼���������ѹ��·��
	if (index != NULL) {
		char *newPath = (char *)malloc(105 * sizeof(char));
		*newPath = '~';
		strcpy((newPath + 1), (path + strlen(home)));
		path = newPath;
	}
	fprintf(stdout, "Tan's Shell %s$ ", path);
	fflush(stdout); // ��ջ����� 
}

void parse(char *buff) {
	int argc; // ����ָ���������
	char *argv[105]; // ����ָ�����
	char command[105]; // ����ָ������
	strcpy(command, buff);
	int i, j, len;
	len = strlen(command);
	argc = 0;
	argv[argc] = (char *)malloc(105 * sizeof(char));
	int flag = 0;
	// �����ȡ��������������������ض���
	for (i = 0, j = 0; i < len; i++) {
		if (command[i] == ' ') {
			if (j != 0) {
				argv[argc][j] = '\0';
				if (flag == 1) { // �ض�������
					int fd = open(argv[argc], O_RDONLY);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDIN_FILENO);
					fdCnt++;
				} else if (flag == 2) { // �ض������
					int fd = open(argv[argc], O_WRONLY | O_CREAT | O_TRUNC, 0x777);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot create the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDOUT_FILENO);
					fdCnt++;
				} else if (flag == 3) { // �ض��������׷�ӣ�
					int fd = open(argv[argc], O_WRONLY | O_APPEND);
					if (fd < 0) {
						fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
					}
					dup2(fd, STDOUT_FILENO);
				}
				flag = 0;
			}
			if (strcmp(argv[argc], "<") == 0) { // �����ض���
				flag = 1;
			} else if (strcmp(argv[argc], ">") == 0) { // ����ض���
				flag = 2;
			} else if (strcmp(argv[argc], ">>") == 0) { // ����ض���׷�ӣ�
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
		} else if (flag == 1) { // �ض�������
			int fd = open(argv[argc], O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "Tan's Shell Error: Cannot open the file: '%s'\n", argv[argc]);
			}
			dup2(fd, STDIN_FILENO);
			fdCnt++;
		} else if (flag == 2) { // �ض������
			mode_t power = 0777;
			int fd = creat(argv[argc], power);
			if (fd < 0) {
				fprintf(stderr, "Tan's Shell Error: Cannot create the file: '%s'\n", argv[argc]);
			}
			dup2(fd, STDOUT_FILENO);
			fdCnt++;
		} else if (flag == 3) { // �ض��������׷�ӣ�
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
	// ������
	if (argc == 0) {
		return;
	}
	// ����ָ��
	if (strcmp(argv[0], "type") == 0) { // 'type'ָ�ʶ��һ��ָ�����ڲ�ָ����ⲿָ�� 
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
	} else if (strcmp(argv[0], "cd") == 0) { // 'cd'ָ��
		// ��ԭ·��ѹ���� �滻 ~ Ϊ��Ŀ¼·��
		char *index; 
		if ((index = strstr(argv[1], "~")) != NULL) {
			char buf[105];
			strcpy(buf, index + 1);
			*index = '\0';
			strcat(argv[1], home);
			strcat(argv[1], buf);
		}
		if (chdir(argv[1]) != 0) { // ʹ��'chdir' 
			fprintf(stderr, "Tan's Shell Error: Cannot change directory to: %s\n", argv[1]);
		} else {
			fprintf(stdout, "Tan's Shell: Successfully change the directory!\n");	
		}
	} else if (strcmp(argv[0], "pwd") == 0) { // 'pwd'ָ��
		char *path = (char *)malloc(105 * sizeof(char));
		if (getcwd(path, 105 * sizeof(char)) == NULL) { // ʹ��'getcwd'
			fprintf(stderr, "Tan's Shell Error: Failed to obtain the path\n");
		} else {
			fprintf(stdout, "Tan's Shell: Successfully obtain the work directory: %s\n", path);	
		}
	} else if (strcmp(argv[0], "chmod") == 0) { // 'chmod'ָ�� 
		mode_t mod;
		int i;
		for (i = 0, mod = 0; i < strlen(argv[2]); i++) {
			mod += (i - '0') + mod * 8;
		}
		if (chmod(argv[1], mod) < 0) { // ʹ��'chmod' 
			fprintf(stderr, "Tan's Shell Error: Fail to change the mode of %s\n", argv[1]);
		} else {
			fprintf(stdout, "Tan's Shell: Successfully change the file mode\n");	
		}
	} else { // �������Ĭ��Ϊ�ⲿ���
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
