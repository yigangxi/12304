#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

ssize_t readn(int fd,void *buf,size_t count){
	size_t nleft = count;
	ssize_t nread;
	char *bufp = (char *)buf;
	while(nleft > 0){
		if((nread = read(fd,bufp,nleft)) < 0){
			if(errno == EINTR)
				continue;
			return 1;
		}
		else if(nread == 0)
			break;
		bufp += nread;
		nleft -= nread;
	}
	return count;
}

ssize_t writen(int fd,const void *buf,size_t count){
	size_t nleft = count;
	ssize_t nwrite;
	char *bufp = (char *)buf;
	while(nleft > 0){
		if((nwrite = write(fd,bufp,nleft)) < 0){
			if(errno == EINTR)
				continue;
			return 1;
		}
		else if(nwrite == 0)
			continue;
		bufp += nwrite;
		nleft -= nwrite;
	}
	return count;
}

ssize_t recv_peek(int sockfd,void *buf,size_t len){
	while(1){
		int ret = recv(sockfd,buf,len,MSG_PEEK);
		if(ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

ssize_t readline(int sockfd,void *buf,size_t maxline){
	int ret,nread;
	char *bufp = (char *)buf;
	int nleft = maxline;
	while(1){
		ret = recv_peek(sockfd,bufp,nleft);
		if(ret < 0)
			return ret;
		else if(ret == 0)
			return ret;
		nread = ret;
		int i;
		for(i=0; i < nread; i++){
			if(bufp[i] == '\n'){
				ret = readn(sockfd,bufp,i+1);
				if(ret != i+1)
					exit(0);
				return ret;
			}
		}
		if(nread > nleft)
			exit(0);
		nleft -= nread;
		ret = readn(sockfd,bufp,nread);
		if(ret != nread)
			exit(0);
		bufp += nread;
	}
	return -1;
}

void do_server(int client_sockfd){
	char recvbuf[BUFSIZ];
	while(1){
		memset(recvbuf,0,sizeof(recvbuf));
		int ret = readline(client_sockfd,recvbuf,BUFSIZ);
		if(ret ==  -1){
			perror("readline error");
			return;
		}
		else if(ret == 0){
		    printf("client close\n");
			exit(0);
	    }
		fputs(recvbuf,stdout);
		writen(client_sockfd,recvbuf,strlen(recvbuf));
	}
}

int main(int argc, char *argv[]){
	//build taojiezi
	int server_sockfd;//server taojiezi
	if((server_sockfd = socket(PF_INET,SOCK_STREAM,0)) < 0){
		perror("socket error");
		return -1;
	}

	int opt = 1;
	if(setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0){
		perror("setsockopt error");
	}

	//bind ip port ipv4
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(8080);
	if(bind(server_sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
		perror("bind error");
		return -1;
	}

	//listen 5
	if(listen(server_sockfd,5) < 0){
		perror("listen error");
		return -1;
	}

	//connect 
	struct sockaddr_in client_addr;
	int client_sockfd;
	socklen_t length;
	length = sizeof(struct sockaddr_in);

	pid_t pid;
	while(1){
		if((client_sockfd = accept(server_sockfd,(struct sockaddr *)&client_addr,&length)) < 0){
			perror("accept error");
			return -1;
		}
		printf("connected success!\n");
		printf("ip = %s\nport = %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
		pid = fork();
		if(pid == -1){
			perror("fork error");
			return -1;
		}
		if(pid == 0){
			close(server_sockfd);
			do_server(client_sockfd);
		}
		else
			close(client_sockfd);
	}
	return 0;
}
