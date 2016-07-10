#include "def.h"

sigset_t sigset;
#define SIGINIT do{sigemptyset(&sigset);sigaddset(&sigset, SIGCHLD);}while(0)
#define SIGBLK do{sigprocmask(SIG_BLOCK, &sigset, NULL);}while(0)
#define SIGUNBLK do{sigprocmask(SIG_UNBLOCK, &sigset, NULL);}while(0)

int sd2;

void close_tcp(int sig){
	printf("\ncatch SIGINT\n");
	close(sd2);
	exit(1);
}

void remove_zombie(int sig){
	int status;
	printf("\ncatch SIGCHLD\n");
	wait(&status);
	//printf("close this client\n");
}

int main(int argc, char *argv[])
{
	int sd;
	char current[BUF_LEN], path[BUF_LEN];
	in_port_t myport = 50021;
	struct sockaddr_in myskt;
	struct sockaddr_in skt;
	int sktlen = sizeof(skt);
	struct myftph_data request;
	
	
	if(argv[1] != NULL){
		strncpy(current, argv[1], BUF_LEN - 1);
		current[strlen(current)] = '\0';
		if(chdir(current) != 0)
			fprintf(stderr, "ERROR: current_dir\n");
	}
	getcwd(path, BUF_LEN);
	printf("FTP_SERVER(daemon): %s\n", path);

	if(daemon(0, 0) < 0){
		perror("ERROR: daemon");
	}
	
	if(chdir(path) != 0)
		fprintf(stderr, "ERROR: current_dir\n");
		
	memset(&myskt, 0, sizeof(myskt)); 
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(myport);
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("ERROR: socket");
			exit(1);
	}

	const int one = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	
	
	if (bind(sd, (struct sockaddr *)&myskt, sizeof(myskt)) < 0) {
		perror("ERROR: bind");
		exit(1);
	}
	
	if (listen(sd, 5) < 0) {
		perror("ERROR: listen");
		exit(1);
	}
	
	signal(SIGINT, close_tcp);
	
	for(;;){
		signal(SIGCHLD, remove_zombie);
		
		if ((sd2 = accept(sd, (struct sockaddr *)&skt, &sktlen)) < 0) {
			perror("ERROR: accept");
			exit(1);
		}

		pid_t pid;
		if((pid = fork()) == 0){
			close(sd);
			/*
			if(daemon(0, 0) < 0){
				perror("ERROR: daemon");
			}
			
			if(chdir(path) != 0)
				fprintf(stderr, "ERROR: current_dir\n");
			*/
			fprintf(stderr, "Client IP-address: %s\n", inet_ntoa(skt.sin_addr));
			for(;;){
				struct myftph reply;
				struct myftph_data reply_data;
				if (recv(sd2, (char *)&request, sizeof(request), 0) < 0) {
					perror("ERROR; recv");
					exit(1);
				}
				switch(request.type){
				case TYPE_QUIT:
					printf("command: quit\n");
					reply.type = TYPE_OK;
					reply.code = CODE_0x00;
					reply.data_length = 0;
					if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
						perror("ERROR: send");
						exit(1);
					}					
					//close(sd2);
					exit(1);
					break;
				case TYPE_PWD:
					printf("command: pwd\n");
					char pwd_path[BUF_LEN] = "";
					getcwd(pwd_path, BUF_LEN);
					reply_data.type = TYPE_OK;
					reply.code = CODE_0x00;
					reply.data_length = strlen(pwd_path);
					
					strcpy(reply_data.data, pwd_path);
					printf("path: %s\n", pwd_path);
					if (send(sd2, (char *)&reply_data, sizeof(reply_data), 0) < 0) {
						perror("ERROR: send");
						exit(1);
					}
					printf("command_END: pwd\n");
					break;
				case TYPE_CWD:
					printf("command: cwd\n");
					if(chdir(request.data) != 0){
						fprintf(stderr, "ERROR: current_dir\n");
						reply.type = TYPE_CMD_ERR;
						reply.code = CODE_0x01;
					}else{
						reply.type = TYPE_OK;
						reply.code = CODE_0x00;
					}
					reply.data_length = 0;
					if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
						perror("ERROR: send");
						exit(1);
					}
					printf("command_END: cwd\n");
					break;
				case TYPE_LIST:
					printf("command: list\n");
					char ls[BUF_LEN] = "ls -l ";
					FILE *fp;
					//printf("request.data = %s\n", request.data);
					if(access(request.data, F_OK) == -1 && strcmp(request.data, "") != 0){
						//printf("not found\n");
						reply.type = TYPE_FILE_ERR;
						reply.code = CODE_0x00;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
					}else if(access(request.data, R_OK) == -1 && strcmp(request.data, "") != 0){
						//printf("cannot access\n");
						reply.type = TYPE_FILE_ERR;
						reply.code = CODE_0x01;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
					}else{
						//printf("start list\n");
						strcat(ls, request.data);
						ls[strlen(ls)] = '\0';
						//printf("%s\n", ls);
						if((fp = popen(ls, "r")) == NULL){
							perror("ERROR: popen");
							reply.type = TYPE_UKWN_ERR;
							reply.code = CODE_0x05;
							reply.data_length = 0;
							if (send(sd2, (char *)&reply, 
									 sizeof(reply), 0) < 0) {
								perror("ERROR: send");
								exit(1);
							}
							break;
						}
						reply.type = TYPE_OK;
						reply.code = CODE_0x01;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}						
						char *fin_flag;
						int first_flag = 0;
						char readline[DATASIZE] = "";
						for(;;){
							//memset(readline , (int)'0', DATASIZE);
							fin_flag = fgets(readline, DATASIZE, fp);
							if(first_flag++ != 0){
								//printf("%s\n", reply_data.data);
								reply_data.type = TYPE_DATA;
								printf("reply_data.data_length = %d\n", reply_data.data_length);
								if(fin_flag == NULL)
									reply_data.code = CODE_0x00;							
								else
									reply_data.code = CODE_0x01;
								//printf("type = %d, code = %d\n", reply_data.type, reply_data.code);	
								if (send(sd2, (char *)&reply_data, 
										 sizeof(reply_data), 0) < 0) {
									perror("ERROR: send");
									exit(1);
								}
								if(reply_data.code == CODE_0x00)
									break;
							}
							reply_data.data_length = strlen(readline);
							strcpy(reply_data.data, readline);
							printf("reply_data.data = %s\n", reply_data.data);
							reply_data.data[DATASIZE] = '\0';
							reply_data.data[reply_data.data_length] = '0';
						}
						pclose(fp);
					}
					printf("command_END: list\n");
					break;
				case TYPE_RETR:
					printf("command: retr\n");
					FILE *fp2;					
					struct stat st;
					stat(request.data, &st);					
					if(access(request.data, F_OK) == -1){
						//printf("not found\n");
						reply.type = TYPE_FILE_ERR;
						reply.code = CODE_0x00;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
					}else if(access(request.data, R_OK) == -1){
						//printf("cannot access\n");
						reply.type = TYPE_FILE_ERR;
						reply.code = CODE_0x01;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
					}else if((st.st_mode & S_IFMT) == S_IFDIR){
						//printf("directory\n");
						reply.type = TYPE_FILE_ERR;
						reply.code = CODE_0x06;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
					}else{
						//printf("request.data = %s\n", request.data);
						if((fp2 = fopen(request.data, "rb")) == NULL) {
							reply.type = TYPE_UKWN_ERR;
							reply.code = CODE_0x05;
							reply.data_length = 0;
							if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
								perror("ERROR: send");
								exit(1);
							}
							break;
						}
						reply.type = TYPE_OK;
						reply.code = CODE_0x01;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							fclose(fp2);
							exit(1);
						}
						for(;;){
							reply_data.data_length = fread(reply_data.data, sizeof(char), DATASIZE, fp2);
							reply_data.type = TYPE_DATA;
							printf("reply_data.data_length = %d\n", reply_data.data_length);
							if(DATASIZE > reply_data.data_length)
								reply_data.code = CODE_0x00;							
							else
								reply_data.code = CODE_0x01;
							if (send(sd2, (char *)&reply_data, sizeof(reply_data), 0) < 0) {
								perror("ERROR: send");
								exit(1);
							}
							if(reply_data.code == CODE_0x00)
								break;
						}
						fclose(fp2);
					}
					printf("command_END: retr\n");
					break;
				case TYPE_STOR:
					printf("command: stor\n");
					FILE *fp3;
					struct stat st2;
					stat(request.data, &st2);
					printf("%s\n", request.data);
					//printf("request.data = %s\n", request.data);
					if(access(request.data, F_OK) != -1){
						//printf("found\n");
						if(access(request.data, R_OK) == -1){
							//printf("cannot access\n");
							reply.type = TYPE_FILE_ERR;
							reply.code = CODE_0x01;
							reply.data_length = 0;
							if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
								perror("ERROR: send");
								exit(1);
							}
							break;
						}else if((st2.st_mode & S_IFMT) == S_IFDIR){
							//printf("directory\n");
							reply.type = TYPE_FILE_ERR;
							reply.code = CODE_0x06;
							reply.data_length = 0;
							if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
								perror("ERROR: send");
								exit(1);
							}
							printf("ok6\n");
							break;
						}
					}

					if((fp3 = fopen(request.data, "wb+")) == NULL) {
						perror("fopen");
						reply.type = TYPE_UKWN_ERR;
						reply.code = CODE_0x05;
						reply.data_length = 0;
						if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
							perror("ERROR: send");
							exit(1);
						}
						break;
					}
					printf("okok\n");
					reply.type = TYPE_OK;
					reply.code = CODE_0x02;
					reply.data_length = 0;
					printf("%u\n", reply.type);
					printf("%u\n", reply.code);
					if (send(sd2, (char *)&reply, sizeof(reply), 0) < 0) {
						perror("ERROR: send");
						fclose(fp3);
						exit(1);
					}
					if(reply.type == TYPE_OK){
						do{
							if (recv(sd2, (char *)&reply_data, sizeof(reply_data), 0) < 0) {
									perror("ERROR: recv");
								exit(1);
							}
							fwrite(reply_data.data, sizeof(char), reply_data.data_length, fp3);
							//fputs(reply_data.data, fp3);
						}while(reply_data.type == TYPE_DATA && reply_data.code == CODE_0x01);
						fclose(fp3);
					}else{
						printf("ERROR: unkown\n");
					}
					printf("command_END: stor\n");
					break;
				}
			}
		}			   
	}
	return 0;
}
