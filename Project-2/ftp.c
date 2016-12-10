
#include "ftp.h"



static int connectSocket(const char* ip, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int connectFTP( const char* ip, int port, ftpSockets* ftp){
	char rd[1024];

	ftp->data = 0;

	if ((ftp->control = connectSocket(ip, port)) < 0) {
		printf("Socket cannot be connected.\n");
		return 1;
	}

	if(readFromFTP(ftp->control, rd, sizeof(rd))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	return 0;

}

int loginFTP(const char* user, const char* password, ftpSockets* ftp){
	char userTest[1024];
	char passTest[1024];

	sprintf(userTest, "user %s\n", user);
	sprintf(passTest, "pass %s\n", password);


	if(sendToFTP(ftp->control, userTest, sizeof(userTest))) {
		printf("Sending to FTP failed.\n");
		return 1;
	}

	if(readFromFTP(ftp->control, userTest, sizeof(userTest))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	if(sendToFTP(ftp->control, passTest, sizeof(passTest))){
		printf("Sending to FTP failed.\n");
		return 1;
	}


	if(readFromFTP(ftp->control, passTest, sizeof(passTest))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	return 0;
}

int changeDirFTP(const char* path, ftpSockets* ftp){
	char currPath[1024];

	sprintf(currPath, "XWD %s\n", path);

	if(sendToFTP(ftp->control, currPath, sizeof(currPath))){
		printf("Sending to FTP failed.\n");
		return 1;
	}


	if(readFromFTP(ftp->control, currPath, sizeof(currPath))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	return 0;
}



int passiveModeFTP(ftpSockets* ftp){
	char passive[1024];
	char passiveIp[1024];
	sprintf(passive, "PASV\n");

	if(sendToFTP(ftp->control, passive, sizeof(passive))){
		printf("Sending to FTP failed.\n");
		return 1;
	}


	if(readFromFTP(ftp->control, passive, sizeof(passive))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	int ip1,ip2,ip3,ip4;
	int port1, port2;
	if((sscanf(passive,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1,&ip2,&ip3,&ip4,&port1,&port2)) < 0){
		printf("Response is wrong.");
		return 1;
	}

	sscanf(passiveIp,"%d.%d.%d.%d",&ip1,&ip2,&ip3,&ip4);
	int port = port1*256 + port2;

	if((ftp->data = connectSocket(passiveIp,port)) < 0){
		printf("Passive mode cannot be entered.");
		return 1;
	}

	return 0;
 
}

int copyFileFTP(const char* filename, ftpSockets* ftp){
	char retr[1024];

	sprintf(retr, "RETR %s\n", filename);

	if(sendToFTP(ftp->control, retr, sizeof(retr))){
		printf("Sending to FTP failed.\n");
		return 1;
	}


	if(readFromFTP(ftp->control, retr, sizeof(retr))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	return 0;

}

int downloadFileFTP(const char* filename, ftpSockets* ftp){
	FILE* file;
	int bytes;

	if (!(file = fopen(filename, "w"))) {
		printf("File cannot be opened.\n");
		return 1;
	}

	char buf[1024];

	while((bytes = read(ftp->data, buf, sizeof(buf))) ){
		if (bytes < 0) {
			printf("ftp->data socket sent nothing.\n");
			return 1;
		}

		if((bytes = fwrite(buf,bytes, 1 , file)) < 0 ){
			printf("Write failed.\n");
			return 1;
		}
	}

	fclose(file);
	close(ftp->data);

	return 0;
}

int disconnectFromFTP(ftpSockets* ftp){
	char disc[1024];

	sprintf(disc, "QUIT\n");

	if(sendToFTP(ftp->control, disc, strlen(disc))){
		printf("Sending to FTP failed.\n");
		return 1;
	}

	if(readFromFTP(ftp->control, disc, sizeof(disc))){
		printf("Read from FTP failed.\n");
		return 1;
	}

	return 0;
}

int sendToFTP(int ftpControl, char* str, size_t size){
	int bytes; 

	if((bytes= write(ftpControl,str,size))<= 0){
		//mensagem de erro
	}

	printf("Bytes written: %d ; Info written: %s", bytes, str);

	return 0;
}


int readFromFTP(int ftpControl, char* str, size_t size){
	FILE* fp = fdopen(ftpControl, "r");


	do {
		memset(str, 0, size);
		fgets(str, size, fp);
		printf("%s", str);
	} while (!('1' <= str[0] && str[0] <= '5') || str[3] != ' ');

	return 0;
}

int main(int argc, char** argv){
	
	if(argc != 2){
		perror("Incorrect number of arguments");
		exit(0);
		
	}
	
	
	url_t* url;
	if(parsePath(argv[1] , url)){
		perror("Failed on parsing path");
		exit(0);
	}
	
	if(getIpByHost(url)){
		perror("Failed on obtaining ID");
		exit(0);
	}
	
	int port = 21;
	ftpSockets* ftp;
	
	if(connectFTP(url->ip, port,ftp)){
		perror("Failed on COnnecting to FTP");
		exit(0);
	}
	
	if(loginFTP(url->username,url->password, ftp)){
		perror("Failed on COnnecting to FTP");
		exit(0);
	}
	
	if(changeDirFTP(url->path,ftp)){
		perror("Failed on COnnecting to FTP");
		exit(0);
	}
	
	if(passiveModeFTP(ftp)){
		perror("Failed to enter passive mode.");
		exit(0);
	}
	
	
	if(copyFileFTP(url->filename,ftp)){
		perror("Failed to copy file.");
		exit(0);
	}
	
	if(downloadFileFTP(url->filename,ftp)){
		perror("Failed to download file");
		exit(0);
	}
	
	if(disconnectFromFTP(ftp)){
		perror("Failed to disconnect from FTP");
		exit(0);
	}
	
	return 0;
}
