#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

ssize_t readn(int fd, void *buf, size_t count){
	size_t nleft = count;
	ssize_t nread;
	char *bufp = (char *)buf;
	while(nleft > 0){
		if((nread = read(fd,bufp,nleft)) < 0){
			if(errno == EINTR)
				continue;
			return 1;
		}
		else if(nread  == 0){
			break;
		}
		bufp += nread;
		nleft -=  nread;
	}
	return count;
}

ssize_t writen(int fd, const void *buf, size_t count){
	size_t nleft =  count;
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

ssize_t rev_peek(int sockfd, void *buf, size_t len){
	while(1){
		int ret = recv(sockfd,buf,len,MSG_PEEK);
		if(ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

ssize_t readline(int sockfd, void *buf,size_t maxline){
	int ret,nread,nleft = maxline;
	char *bufp = (char *)buf;
	while(1){
		ret =rev_peek(sockfd,bufp,nleft);
		if(ret < 0)
			return ret;
		else if(ret == 0)
			return ret;
		nread =ret;
		int i;
		for(i = 0; i < nread; i++){
			if(bufp[i] == '\n'){
				ret = readn(sockfd,buf,i+1);
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

int main(int argc, char *argv[]){
	//the first,创建套接字
	int client_sockfd;
	if((client_sockfd = socket(PF_INET,SOCK_STREAM,0)) < 0){
		perror("socket error");
		return -1;
	}

	//the second,绑定套接字
	struct sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr("192.168.10.128");
	client_addr.sin_port = htons(8080);
	if(connect(client_sockfd,(struct sockaddr *)&client_addr,sizeof(struct sockaddr)) < 0){
		perror("connect error");
		return -1;
	}
	printf("connected to server\n");

	//the third,数据交互
	char recvbuf[BUFSIZ] = {0};
	char sendbuf[BUFSIZ] = {0};
	while(fgets(sendbuf,sizeof(sendbuf),stdin) != NULL){
		writen(client_sockfd,sendbuf,strlen(sendbuf));

		int ret = readline(client_sockfd,recvbuf,sizeof(recvbuf));
		if(ret ==  -1){
			perror("readline error");
			return 1;
		}
		else if(ret == 0){
			printf("client close\n");
			break;
		}

		fputs(recvbuf,stdout);
		memset(recvbuf,0,sizeof(recvbuf));
		memset(sendbuf,0,sizeof(sendbuf));
	}

	//the forth,关闭套接字
	close(client_sockfd);
	
	return 0;
}
