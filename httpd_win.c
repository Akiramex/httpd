#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ctype.h>
#include <process.h>
#pragma comment(lib, "WS2_32.lib")

#define SERVER_STRING "Server: httpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2

void error_die(const char *message)
{
	perror(message);
	exit(1);
}

// 请求成功响应 200 OK
void headers(int client, const char *filename)
{
	char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

// 400 BAD REQUEST
void badRequest(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

// 未找到文件响应 404 Not Found
void notFound(int client)
{
	char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

// 500 Internal Server Error
void cannotExecute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

// 处理非GET和POST请求的情况 501
void unimplemented(int client)
{
	char buf[1024];
	// 响应
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

// 读取文件并发送
void cat(int client, FILE *resource)
{
	char buf[1024]

	fgets(buf, sizeof(buf), resource);
	while (!feof(resource))
	{
		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}

// 输出请求成功并发送文件
void serveFile(int client, const char *filename)
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	buf[0] = 'A', buf[1] = '\0';
	// 读取、记录余下的全部请求头
	while ((numchars > 0) && strcmp("\n", buf))
		numchars = get_line(client, buf, sizeof(buf));

	resource = fopen(filename, "r");
	if(resource == NULL)
		not_found(client);
	else
	{
		header(client, filename);
		cat(client, resource);
	}
	fclose(resource);
}

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while((i < size -1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				// MSG_PEEK标志：读取sock的数据进缓存区，但是不减少sock的数据
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';

	return i;
}

// cgi请求服务
void executeCgi(int client, const char *path,const char *method, const char *query_string)
{
	char buf[1024];
	int cgi_output[2];
	int cgi_input[2];
	pid_t pid;
	int status;
	int i;
	char c;
	int numchars = 1;
	int content_length = -1;

	buf[0] = "A"; buf[1] ='\0';
	if (strcasecmp(method, "GET") == 0)
		while ((numchars > 0) && strcmp("\n", buf))
			numchars = get_line(client, buf, sizeof(buf));
	else if(strcasecmp(method, "POST") == 0)
	{
		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n"), buf)
		{
			buf[15] = '\0';
			if(strcasecmp(buf, "content_Length") == 0)
				content_length = atoi(&(buf[16]));
			numchars = get_line(client, buf, sizeof(buf));
		}
		if(content_length == -1)
		{
			badRequest(client);
			return;
		}
	}
	else
	{
	}

	// 创建两个管道
	if(pipe(cgi_output) < 0)
	{
		cannotExecute(client);
		return;
	}
	if(pipe(cgi_input) < 0)
	{
		cannotExecute(client);
		return;
	}

	// 新建新进程
	if((pid == fork() < 0)) {
		cannotExecute(client);
		return;
	}
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	// 子进程：CGI脚本
	if(pid == 0)
	{
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		// 复制管道并重定向到标准流中
		dup2(cgi_output[1], STDOUT);
		dup2(cgi_input[0], STDIN);
		closesocket(cgi_output[0]);
		closesocket(cgi_input[1]);
		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);
		if(strcasecmp(method, "GET") == 0)
		{
			sprintf(query_env, "QIERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else
		{
			printf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		execl(path, NULL);
		exit(0);
	}
	// 父进程
	else
	{
		closesocket(cgi_output[1]);
        closesocket(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
        {
        	for (i = 0; i < content_length; i++)
        	{
        		recv(client, &c, 1, 0);
        		write(cgi_input[1], &c, 1);
        	}
        }
        while (read(cgi_output[0], &c, 1) > 0)
        	send(client, &c, 1, 0);

        closesocket(cgi_output[0]);
        closesocket(cgi_input[1]);
        waitpid(pid, &status, 0);
	}
}

unsigned WINAPI acceptRequest(void *arg)
{
	HANDLE clnt = (HANDLE)arg;
	char buf[1024];
	size_t numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0;

	char *queryString = NULL;

	numchars = get_line(clnt, buf, sizeof(buf));

	i =0; j = 0;
	while(!isspace(buf[i]) && (i < sizeof(method) -1 ))
	{
		method[i] = buf[i];
		i++;
	}
	j = i;
	method[i] ='\0';

	if(strcasecmp(method, "GET") && strcasecmp(method, "POST"))
	{
		unimplemented(clnt);
		return 0;
	}

	if(strcasecmp(method, "POST") == 0)
		cgi = 1;

	// 获取url
	i = 0;
	while (isspace(buf[j]) && (j < numchars))
		j++;
	while (!isspace(buf[j]) && (i < sizeof(url) - 1) && (j <numchars))
	{
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	// 对GET请求处理
	if(strcasecmp(method, "GET") == 0)
	{
		queryString = url;
		// 将字符指针移至 '?' 或 '\0'
		while ((*queryString != '?') && (*queryString != '\0'))
			queryString++;
		if(*queryString == '?')
		{
			cgi = 1;
			// 截断
			*queryString = '\0';
			queryString++;
		}
	}

	spintf(path, "htdocs%s", url);
	if(path[strlen(path) - 1] == '/')
		strcat(path, "index.html");

	// 获取文件失败
	if (stat(path, &st) == -1)
	{
		// 读取、记录余下的全部请求头
		while((numchars > 0) && strcmp("\n", buf))
			numchars = get_line(clnt, buf, sizeof(buf));
		notFound(clnt);
	}
	else
	{
		// 路径是文件夹
		if((st.st_mode & S_IFMT) == S_IFDIR)
			strcat(path, "/index.html");
		// 文件对user，group，other具有可执行权限
		if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
			cgi = 1;
		if(!cgi)
			serveFile(clnt, path);
		else
			executeCgi(client, path, method, queryString);
	}
	closesocket(client);
	return 0;
}


SOCKET StartUp(unsigned short *port)
{
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
		error_die("WSAStartup");

	int servSock
	int on = 1;
	struct sockaddr_in servAdr;

	servSock = socket(PF_INET, SOCK_STREAM, 0);
	if(servSock == -1)
		error_die("socket");
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(*port);

	// 开启地址复用
	if((setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
	{
		error_die("setsockopt failed");
	}

	if(bind(servSock, (struct sockaddr*)&servAdr, sizeof(servAdr)) < 0)
	{
		error_die("bind");
	}

	// 如果不指定端口，则动态分配端口
	if(*port == 0)
	{
		socklen_t servAdrSz = sizeof(servAdr);
		if(getsockname(servSock, (struct sockaddr *)&servAdr, sizeof(servAdr)) == -1)
		{
			error_die("getsockname");
		}
		*port = ntohs(servAdr.sin_port);
	}
	if(listen(servSock, 5) < 0)
		error_die("listen");
	WSACleanup();
	return servSock;
}

int main(void)
{
	unsigned short port = 4000;
	SOCKET servSock = -1;
	SOCKET clntSock = -1;
	SOCKADDR_IN clntAdr;
	socklen_t clntAdrSz = sizeof(clntAdr);
	HANDLE newThread;

	ServSock = StartUp(&prot);
	printf("http running on port %d\n", port);

	while(1)
	{
		clntSock = accept(servSock, (struct sockaddr*)&clntAdr, clntAdrSz);
		if(clntSock == -1)
			error_die("accept");

		// 创建新的线程启动服务
		newThread = (HANDLE)_beginthreadex(0, NULL, acceptRequest,(void*)&clntSock, 0, NULL);
		if(newThread == 0)
			perror("_beginthreadex");
	}

	closesocket(servSock);

	return 0;
}