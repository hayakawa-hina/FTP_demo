#include "def.h"

int sd;
int command_exe(int , char *[]);

int pwd_proc(int , char *[]);
int quit_proc(int , char *[]);
int cd_proc(int , char *[]);
int dir_proc(int, char *[]);
int lpwd_proc(int, char *[]);
int lcd_proc(int, char *[]);
int ldir_proc(int, char *[]);
int get_proc(int, char *[]);
int put_proc(int, char *[]);
int help_proc(int , char* []);

struct command_table {
	char *cmd;
	int (*func)(int, char *[]);
}cmd_tbl[] = {
	{"quit", quit_proc},
	{"pwd", pwd_proc},
	{"cd", cd_proc},
	{"dir", dir_proc},
	{"lpwd", lpwd_proc},
	{"lcd", lcd_proc},
	{"ldir", ldir_proc},
	{"get", get_proc},
	{"put", put_proc},
	{"help", help_proc},
	{NULL, NULL}
};

int help_proc(int ac, char* av[])
{
	printf("\nCommand Manual\n\n");
	printf("quit\n");
	printf("\tExit myFTP\n");
	printf("pwd\n");
	printf("\tRemote: Print working directory\n");
	printf("cd Remote_DirectoryPath\n");
	printf("\tRemote: Change directory\n");
	printf("dir [Remote_Path]\n");
	printf("\tRemote: List segments\n");
	printf("lpwd\n");
	printf("\tClient: Print working directory\n");
	printf("lcd Client_DirectoryPath\n");
	printf("\tClient: Change directory\n");
	printf("ldir [Client_Path]\n");
	printf("\tClient: List segments\n");
	printf("get Remote_Path [Client_Path]\n");
	printf("\tCopy Remote_Path to Client_Path\n");
	printf("put Client_Path [Remote_Path]\n");
	printf("\tCopy Client_Path to Remote_Path\n");
	return 0;
}

int quit_proc(int ac, char* av[])
{
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_QUIT;
	request.data_length = 0;
	if (send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("send");
		exit(1);
	}
	if (recv(sd, (char *)&reply, sizeof(reply), 0) < 0) {
		perror("ERROR: recv");
		exit(1);
	}
	//printf("quit\n");
	return -1;
}

int pwd_proc(int ac, char* av[])
{
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_PWD;
	request.data_length = 0;
	if (send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("send");
		exit(1);
	}
	if (recv(sd, (char *)&reply_data, sizeof(reply_data), 0) < 0) {
		perror("ERROR; recv");
		exit(1);
	}
	//printf("pwd\n");
	printf("%s\n", reply_data.data);
	return 0;
}

int cd_proc(int ac, char *av[])
{
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_CWD;
	request.data_length = 0;
	if(av[1] != NULL)
		strcpy(request.data, strtok(av[1], " \n\0"));
	else
		strcpy(request.data, "");
	if (send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("send");
		exit(1);
	}
	if (recv(sd, (char *)&reply, sizeof(reply), 0) < 0) {
		perror("ERROR; recv");
		exit(1);
	}
	if(reply.type == TYPE_OK && reply.code == CODE_0x00){
		printf("cd\n");
	}else{
		printf("ERORR: cd(command)\n");
	}
	return 0;
}

int dir_proc(int ac, char *av[])
{
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_LIST;
	request.data_length = 0;//kake
	if(av[1] != NULL)
		strcpy(request.data, strtok(av[1], " \n\0"));
	else
		strcpy(request.data, "");
		
	if (send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("ERROR: send");
		exit(1);
	}
	if (recv(sd, (char *)&reply, sizeof(reply), 0) < 0) {
			perror("ERROR; recv");
			exit(1);
	}
	if(reply.type == TYPE_OK){
		do{
			if (recv(sd, (char *)&reply_data, sizeof(reply_data), MSG_WAITALL) < 0) {
				perror("ERROR; recv");
				exit(1);
			}
			//printf("reply_data.data_length = %d\n", reply_data.data_length);
			reply_data.data[reply_data.data_length] = '\0';
			printf("%s", reply_data.data);
		}while(reply_data.type == TYPE_DATA && reply_data.code == CODE_0x01);
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x00){
		printf("ERORR: dir: not exist file/directory\n");
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x01){
		printf("ERORR: dir: cannnot access file/directory\n");
	}else{
		printf("ERROR: dir: unknown error\n");
	}
	//printf("dir\n");
	return 0;
}

int lpwd_proc(int ac, char *av[])
{
	//printf("lpwd\n");
	char path[BUF_LEN];
	getcwd(path, BUF_LEN);
	printf("%s\n", path);
}

int lcd_proc(int ac, char *av[])
{
	//printf("lcd\n");
	if(chdir(av[1]) != 0)
		fprintf(stderr, "ERROR: lcd(command)\n");
}

int ldir_proc(int ac, char *av[])
{
	//printf("ldir\n");
	int status;
	pid_t pid;
	char *ldir[] = {"ls", "-l", NULL, NULL};
	ldir[2] = av[1];
	if((pid = fork()) == 0){
		execvp(ldir[0], &ldir[0]);
	}
	wait(&status);
}

int get_proc(int ac, char *av[])
{
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_RETR;
	request.data_length = 0;
	char filename[BUF_LEN/2];
	int filename_flag = 0;
	FILE *fp;
	
	if(av[1] != NULL){
		strcpy(request.data, strtok(av[1], " \n\0"));
		if(av[2] != NULL){
			//printf("filename: %s\n", av[2]);
			struct stat st;
			stat(av[2], &st);
			if(av[2][strlen(av[2]) - 1] == '/' || strcmp(av[2], "/") == 0){
				//printf("%s: directory2\n", av[2]);
				filename_flag = 2;
			}else if(strcmp(av[2], ".") == 0){
				//printf("%s: directory3\n", av[2]);
				av[2] = NULL;
			}else if(strcmp(av[2], "..") == 0){
				//printf("%s: directory4\n", av[2]);
				filename_flag = 1;
			}else if((st.st_mode & S_IFMT) == S_IFDIR){
				//printf("%s: directory1\n", av[2]);
				filename_flag = 1;
			}else{
				//printf("%s: file\n", av[2]);
				strcpy(filename, av[2]);
			}
		}
		if(av[2] == NULL || filename_flag > 0){
			char *p, *prev;
			p = strtok(av[1], "/\n");
			while(p != NULL){
				prev = p;
				p = strtok(NULL, "/\n");
			}
			strcpy(filename, prev);
			if(filename_flag == 1){
				char tmp[BUF_LEN/2] = "";
				char s[] = "/";
				strcat(tmp, av[2]);
				strcat(tmp, s);
				strcat(tmp, filename);
				strcpy(filename, tmp);
			}else if(filename_flag == 2){
				strcat(av[2], filename);
				strcpy(filename, av[2]);
			}
			//printf("filename: %s\n", filename);
		}
	}else{
		printf("ERROR: get server_path filename\n");
		return 1;
	}
	if((fp = fopen(filename, "wb+")) == NULL) {
		perror("ERROR: ");
		return 1;
	}
	if(send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("ERROR: send");
		exit(1);
	}
	if(recv(sd, (char *)&reply, sizeof(reply), 0) < 0) {
		perror("ERROR; recv");
		exit(1);
	}
	//printf("type = %d, code = %d\n", reply.type, reply.code);
	if(reply.type == TYPE_OK){
		do{
			if (recv(sd, (char *)&reply_data, sizeof(reply_data), MSG_WAITALL) < 0) {
				perror("ERROR; recv");
				exit(1);
			}
			//reply_data.data[reply_data.data_length] = '\0';
			fwrite(reply_data.data , sizeof(char), reply_data.data_length, fp);
			/*
			int i = 0;
			for(i = 0; i < reply_data.data_length; i++){
				fputc(reply_data.data[i], fp);				
			}*/
			//printf("%s", reply_data.data);
			//fputs(reply_data.data, fp);	
			//printf("type = %d, code = %d\n", reply_data.type, reply_data.code);
		}while(reply_data.type == TYPE_DATA && reply_data.code == CODE_0x01);
		fclose(fp);
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x00){
		printf("ERORR: get: not exist file\n");
		return 1;
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x01){
		printf("ERORR: get: cannnot access file\n");
		return 1;
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x06){
		printf("ERORR: get: Is a directory\n");
		return 1;
	}else{
		printf("ERROR: get: unknown error\n");
		return 1;
	}
	//printf("get\n");
	return 0;
}

int put_proc(int ac, char *av[])
{	
	struct myftph_data request;
	struct myftph reply;
	struct myftph_data reply_data;

	request.type = TYPE_STOR;
	request.data_length = 0;
	char filename[BUF_LEN/2];
	char filename2[BUF_LEN/2];
	int filename_flag = 0;
	FILE *fp;
	
	if(av[1] != NULL){
		strcpy(filename2, strtok(av[1], " \n\0"));
		if(av[2] != NULL){
			//printf("filename: %s\n", av[2]);
			if(av[2][strlen(av[2]) - 1] == '/' || strcmp(av[2], "/") == 0){
				//printf("%s: directory2\n", av[2]);
				filename_flag = 2;
			}else if(strcmp(av[2], ".") == 0){
				//printf("%s: directory3\n", av[2]);
				av[2] = NULL;
			}else if(strcmp(av[2], "..") == 0){
				//printf("%s: directory4\n", av[2]);
				filename_flag = 1;
			}else{
				//printf("%s: file\n", av[2]);
				strcpy(filename, av[2]);
			}
		}
		if(av[2] == NULL || filename_flag > 0){
			char *p, *prev;
			p = strtok(av[1], "/\n");
			while(p != NULL){
				prev = p;
				p = strtok(NULL, "/\n");
			}
			strcpy(filename, prev);
			if(filename_flag == 1){
				char tmp[BUF_LEN/2] = "";
				char s[] = "/";
				strcat(tmp, av[2]);
				strcat(tmp, s);
				strcat(tmp, filename);
				strcpy(filename, tmp);
			}else if(filename_flag == 2){
				strcat(av[2], filename);
				strcpy(filename, av[2]);
			}
			//printf("request.data: %s\n", filename);
		}
		strcpy(request.data, strtok(filename, " \n\0"));
	}else{
		printf("ERROR: put client_path filename\n");
		return 1;
	}
	
	struct stat st;
	stat(filename2, &st);					
	if(access(filename2, F_OK) == -1){
		printf("ERORR: put: not exist file\n");
		return 1;
	}else if(access(filename2, R_OK) == -1){
		printf("ERORR: put: cannnot access file\n");
		return 1;
	}else if((st.st_mode & S_IFMT) == S_IFDIR){
		printf("ERORR: put: Is a directory\n");
		return 1;
	}
	
	if((fp = fopen(filename2, "rb")) == NULL) {
		perror("ERROR: ");
		return 1;
	}
	if(send(sd, (char *)&request, sizeof(request), 0) < 0) {
		perror("ERROR: send");
		exit(1);
	}
	if(recv(sd, (char *)&reply, sizeof(reply), 0) < 0) {
		perror("ERROR: recv");
		exit(1);
	}
	if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x01){
		printf("ERROR: cannot access\n");
		return 1;
	}else if(reply.type == TYPE_FILE_ERR && reply.code == CODE_0x06){
		printf("ERROR: Is a directory\n");
		return 1;
	}else if(reply.type == TYPE_OK && reply.code == CODE_0x02){
		for(;;){
			reply_data.data_length = fread(reply_data.data, sizeof(char), DATASIZE, fp);
			reply_data.type = TYPE_DATA;
			//printf("reply_data.data_length = %d\n", reply_data.data_length);
			if(DATASIZE > reply_data.data_length)
				reply_data.code = CODE_0x00;							
			else
				reply_data.code = CODE_0x01;
			if (send(sd, (char *)&reply_data, sizeof(reply_data), 0) < 0) {
				perror("ERROR: send");
				exit(1);
			}
			if(reply_data.code == CODE_0x00)
				break;
		}
		fclose(fp);
	}else{
		printf("ERROR: unknown error\n");
		return 1;
	}
	//printf("put\n");
	return 0;
}

int main(int argc, char *argv[])
{
	int err;
	int c; // connect
	char host[BUF_LEN];
	char *serv;
	
	int ac;
	char *av[16], buf[BUF_LEN];
	
	struct addrinfo hints, *res;
	
	serv = "50021";
	
	if(argc != 2){
		fprintf(stderr, "ERORR: ./myftpc server-IP-address");
		exit(1);
	}
	
	strncpy(host, argv[1], BUF_LEN - 1);
	host[strlen(host)] = '\0';
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	
	if ((err = getaddrinfo(host, serv, &hints, &res)) < 0) {
           fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		   exit(1);
	}
	
	if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("ERROR: socket");
		exit(1);
	}
	
	if ((c = connect(sd, res->ai_addr, res->ai_addrlen)) < 0) {
		perror("ERROR: connect");
		exit(1);
	}else if(c == 0){
		printf("Established connection: client\n");
	}
	
	
	for(;;){
		printf("myFTP%% ");
		
		if(fgets(buf, sizeof(buf), stdin) == NULL)
			return 0;
		if(strlen(buf) == 1)
			continue;
		ac = get_commandline(buf, av);
		if(ac == -1)
			continue;
			
		//printf("argc = %d\n", ac);
		/*int i;
		for(i = 0; av[i] != NULL; i++)
			printf("argv[%d] = (%s)\n", i, av[i]);
		*/
		if(command_exe(ac, av) == -1){
			close(sd);
			break;
		}
	}
	return 0;
}

int command_exe(int ac, char *av[])
{
	struct command_table *p;
	
	for (p = cmd_tbl; p->cmd; p++){
		if(strcmp(av[0], p->cmd) == 0){
			return (*p->func)(ac, av);
		}
	}
	if(p->cmd == NULL){
		fprintf(stderr, "unknown command\n");
	}		
}
