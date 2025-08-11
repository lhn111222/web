#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>

#define BUFSIZE 2048

//HTTP头
char success[BUFSIZE];
char content[] = "<HTML><BODY>JKJKJKJK</BODY></HTML>";
char dir[100][20] = {0};

int mydir(char *pathname)
{
	int count = 0;
	memset(dir,0x00,sizeof(dir));
	//home/worl/0422
	chdir(pathname);
	DIR *p = opendir(".");
	if(p == NULL)
	{
		printf("error!\n");
		return 0;
	}
	printf("open success!\n");
	char buf[100] = {0};
	struct dirent *pdir = readdir(p);
	while(pdir != NULL)
	{

		if(strcmp(pdir->d_name,".")==0 ||
					       	strcmp(pdir->d_name,"..")==0)
		{
			pdir = readdir(p);
			continue;
		}
		
		strcpy(dir[count],pdir->d_name);
		count++;
		pdir = readdir(p);
	}
	/*
	int i;
	for(i=0;i<count;i++)
	{
		printf("%s\n",dir[i]);
	}
	*/
	closedir(p);
	return count;
}


//简单的表达式计算函数
double evaluate_expression(const char *s) 
{
	double result = 0, num = 0;
	char op = '+';//一开始就是+第一个数 比如1+1 第一次解析1 直接就算0+1
	const char *ch = s;//记录首地址

	int isNegative = 0;

	while(*s)
	{
		while(*s == ' ')
		{
			//printf("跳过空格\n");
			s++;
		}

		//处理负号
		if(*s=='-' && (s==ch || *(s-1)=='(' || *(s-1)=='+' 
					|| *(s-1)=='-' || *(s-1)=='*' 
					|| *(s-1)=='/'))
		{
			isNegative = 1;
			s++;

			while(*s == ' ')
			{
				//printf("跳过空格\n");
				s++; 
			}
		}
		//手动解析数字
		if(*s >= '0' && *s <= '9')
		{
			num = 0;
			while (*s >= '0' && *s <= '9')
			{
				num = num * 10 + (*s - '0');
				s++;
			}
			// 处理小数部分
			if(*s == '.')
			{
				s++;
				double frac = 0.1;
				while (*s >= '0' && *s <= '9')
				{
					num += (*s - '0') * frac;
					frac *= 0.1;
					s++;
				}
			}
			//转成负数  123 -123
			if(isNegative)
			{
				num = -num;
			}
			//根据运算符计算
			switch(op)
			{
				case '+': result += num; break;
				case '-': result -= num; break;
				case '*': result *= num; break;
				case '/': result /= num; break;
			}
			continue;
		}

		// 记录运算符
		if(*s == '%' && *(s+2) == 'F')
		{
			op = '/';
			s+=3;
			continue;
		}
		else if(*s == '%' && *(s+2) == 'B')
		{
			op = '+';
			s+=3;
			continue;
		}
		else if(*s == '-' || *s == '*')
		{
			op = *s;
			s++;
			continue;
		}

		s++;//跳过其他字符
	}

	return result;
}

void *thread(void *p)
{
	char date[BUFSIZE] = {0};
	int fd = (int)p;
	char buf[4096];
	
	read(fd,buf,sizeof(buf));
	//printf("%s\n",buf);
	//解析请求行 GET /example.jpg HTTP/1.1\r\n 空格隔开的三个字符串
	char method[10],path[100],version[20];
	sscanf(buf,"%s %s %s",method,path,version);//遇到空格结束\n也一样:

	if(strcmp(path,"/favicon.ico") == 0)
	{
		close(fd);
		return NULL;
	}
	printf("请求: %s %s %s\n", method, path, version);

	//处理静态文件请求
	char file_path[100];
	strcpy(file_path,"./www");

	//默认页面,第一次进入页面申请的网页就是/，如下：
	//GET / HTTP/1.1
	//Host: localhost
	if(strcmp(path, "/") == 0)
	{
		strcat(file_path,"/index.html");
	}
	else//后面申请的都是文件名：GET /example.jpg 
	{
		strcat(file_path,path);
	}

	//确定Content-Type
	char content_type[50] = "text/html";
	if(strstr(file_path, ".jpg") || strstr(file_path, ".jpeg"))
	{
		strcpy(content_type, "image/jpeg");
	}
	else if(strstr(file_path, ".png"))
	{
		strcpy(content_type, "image/png");
	}
	else if(strstr(file_path, ".mp3"))
	{
		strcpy(content_type, "audio/mpeg");
	}

	//设置HTTP头
	sprintf(success,"HTTP/1.1 200 OK\r\n"
			"Content-type:%s\r\n"
			"\r\n",content_type);
	//发送报头  之后再发送网站申请的文件内容
	write(fd,success,strlen(success));

	int postflag=0,calflag=0,phoflag=0,lsflag=0,exflag=0;
	int score=0;
	double result;
	int rtn;
	char *body_start;
	if(strcmp(method,"POST") == 0)
	{
		if(strcmp(path,"/calculator.html") == 0) calflag = 1;
		printf("post!\n");
		postflag = 1;
		//提取表达式
		body_start = strstr(buf, "\r\n\r\n");
		if((body_start != NULL) && (calflag == 1))
		{
			body_start += 4;
			if(calflag == 1)
				result = evaluate_expression(body_start);
			printf("POST解析串：%s\n",body_start);
		}

		if(strcmp(path,"/photo.html") == 0) phoflag = 1;
		else if((strcmp(path,"/myls.html") == 0) || (strcmp(path,"/nmap.html") == 0)) 
		{
			char ndate[1024*5];
			printf("myls\n");
			char mbuf[50] = {0};
			char *c,*a; 
			if(strcmp(path,"/myls.html") == 0)
			{
				strcpy(mbuf,body_start);
				c = strstr(mbuf,"=");
				c++;
				while(a = strstr(mbuf,"+"))
				{
					*a = ' ';
				}
				printf("c=:%s\n",c);
			}
			//rtn = mydir("./");
			lsflag = 1;
			char fline[200];
			FILE *fp;
			if(strcmp(path,"/myls.html") == 0)
			{
				printf("打开文件\n");
				fp = fopen("./www/myls.html","r");
			}
			else
			{
				printf("打开状态\n");
				fp = fopen("./www/nmap.html","r");
				strcpy(mbuf,"nmap -sn -v 192.168.37.100-119");
				c = mbuf;
			}
			while(fgets(fline,sizeof(fline),fp))
			{
				//printf("读取\n");
				strcat(ndate,fline);
				if(strstr(fline,"comment"))
				{
					FILE *fp1 = popen(c,"r");
					char fline1[200];
					while(fgets(fline1,sizeof(fline1),fp1))
					{
						//printf("读取2\n");
						strcat(ndate,"<th>");
						strcat(ndate,fline1);
						strcat(ndate,"</th>\n");
						memset(fline1,0x00,sizeof(fline1));
					}
					pclose(fp1);
				}
				memset(fline,0x00,sizeof(fline));
			}
			write(fd,ndate,strlen(ndate));
			fclose(fp);
			return NULL;
		}
		else if(strcmp(path,"/file.html") == 0)
		{

			printf("上传处理\n");
			// 上传文件处理
			char *filest,filedate[2048];

			body_start = strstr(buf,"\r\n\r\n");
			if(body_start == NULL)
			{
				printf("没找到body行\n");
				return NULL;
			}
			body_start += 4; // 跳过2个换行符
			printf("body:%s\n",body_start);

			printf("---\n");
			filest = strstr(body_start,"\r\n\r\n");
			filest +=4;
			
			char boundary[100] = {0};
			char *boundary_start = strstr(buf,"Content-Type: multipart/form-data; boundary=");
			boundary_start += strlen("Content-Type: multipart/form-data; boundary=");

			char *boundary_end = strchr(boundary_start,'\r');
			*boundary_end = '\0';
			strcpy(boundary,"--");
			strcat(boundary,boundary_start);
			strcat(boundary,"--");
			*boundary_end = '\r';
			//printf("boundary：%s\n",boundary);

			char *p = strstr(filest,boundary);
			*p = '\0';
			strcpy(filedate,filest);
			printf("filedate = %s\n",filedate);	
			*p = '-';

			char *filenamest,*filenameend,filename[20] = {0};
			filenamest = strstr(buf,"filename=");
			filenamest +=10;
			//printf("filest：%s\n",filenamest);
			filenameend = strstr(filenamest,"\r");
			filenameend--;
			//printf("fileend：%s\n",filenameend);
			*filenameend = '\0';
			strcpy(filename,"./uploads/");
			strcat(filename,filenamest);
			*filenameend = '\r';
			printf("filename：%s\n",filename);

			//printf("%s\n",buf);
			char *fileend = strstr(buf,boundary);
			printf("END:%s\n",fileend);


			
			int fww = open(filename,O_WRONLY|O_CREAT,0644);

			write(fww,filedate,strlen(filedate));
			close(fww);
			
			
		}
		else if(strcmp(path,"/exam.html") == 0)
		{
			exflag = 1;
			printf("分数计算：\n");

			body_start = strstr(buf, "\r\n\r\n");
			body_start += 4;
			printf("考生答案：%s\n",body_start);

			int i=0;
			char right[6] = "ABACD";
			char *p;//time_limit=30&q1=A&q2=A&q3=A&q4=A&q5=A
			p = body_start;
			while(p = strstr(p,"&"))
			{
				p+=4;
				printf("*p = %c\n",*p);
				if(*p == right[i++])
                                        score+=20;
			}
			printf("score:%d\n",score);

		}
	}
	if(postflag == 1)
	{
		//post .html请求
		printf("处理post网页\n");
		FILE* fp = fopen(file_path,"r");
		char line[1024] = {0};
		char jpg[10] = "1.jpg";
		char aim[5] = "1";
		printf("%s\n",file_path);
		while(fgets(date,sizeof(date)-1,fp) != NULL)
		{
			if(calflag == 1 && (strstr(date,"计算结果") != NULL))
			{
				printf("结果输出:%lf\n",result);
				memset(line,0x00,sizeof(line));
				sprintf(line,"<div id=\"result\">计算结果:%f</div>\n",result);
				//printf("666\n");
				write(fd,line,strlen(line));
			}
			else if(phoflag == 1)
			{
				if(strstr(date,"input") && strstr(body_start,"next"))
				{
					printf("next\n");
					if(strstr(body_start,"=1"))
					{
						//printf("1->2\n");
						memset(aim,0x00,sizeof(aim));
						strcpy(aim,"2");
						memset(jpg,0x00,sizeof(jpg));
						strcpy(jpg,"2.jpg");
						//printf("jpg = %s\n",jpg);
					}
					else if(strstr(body_start,"=2"))
					{
						memset(aim,0x00,sizeof(aim));
						strcpy(aim,"3");
						memset(jpg,0x00,sizeof(jpg));
						strcpy(jpg,"3.jpg");
					}
					memset(line,0x00,sizeof(line));
					sprintf(line,"<input type=\"hidden\" name=\"current\" value=%s>\n",aim);
					write(fd,line,strlen(line));
				}
				else if(strstr(date,"input") && strstr(body_start,"prev"))
				{
					printf("prev\n");
					if(strstr(body_start,"=3"))
					{
						memset(aim,0x00,sizeof(aim));
						strcpy(aim,"2");
						memset(jpg,0x00,sizeof(jpg));
						strcpy(jpg,"2.jpg");
					}
					else if(strstr(body_start,"=2"))
					{
						memset(aim,0x00,sizeof(aim));
						strcpy(aim,"1");
						memset(jpg,0x00,sizeof(jpg));
						strcpy(jpg,"1.jpg");
					}
					else if(strstr(body_start,"=1"))
					{
						memset(aim,0x00,sizeof(aim));
						strcpy(aim,"3");
						memset(jpg,0x00,sizeof(jpg));
						strcpy(jpg,"3.jpg");
					}
					memset(line,0x00,sizeof(line));
					sprintf(line,"<input type=\"hidden\" name=\"current\" value=%s>\n",aim);
					write(fd,line,strlen(line));
				}
				else if(strstr(date,"img"))
				{	
					printf("输出图片:%s\n",jpg);
					/*
					   sprintf(line,"\n",result);
					   write(fd,date,strlen(date));*/
					memset(line,0x00,sizeof(line));
					sprintf(line,"<img src=%s alt=\"当前图片\" style=\"max-width: 80%%; margin: 20px;\">\n",jpg);
					write(fd,line,strlen(line));
				}
				else
				{
					write(fd,date,strlen(date));
				}
			}
			else if(exflag == 1)
			{
				if(strstr(date,"成绩显示"))
				{
					
					memset(line,0x00,sizeof(line));
					sprintf(line,"<h3>%d</h3>\n",score);
					write(fd,line,strlen(line));
				}
				else
					write(fd,date,strlen(date));
			}
			else
				write(fd,date,strlen(date));
		}
		fclose(fp);
	}
	else//fgets 不能读取二进制文件 所以这里用read处理get请求
	{
		//发送文件内容
		int fr,bytes_read;
		fr = open(file_path,O_RDONLY);
		char file_buf[BUFSIZE];
		while((bytes_read = read(fr,file_buf,BUFSIZE)) > 0)
		{
			write(fd,file_buf,bytes_read);//读多少写多少
		}
		close(fr);
	}
	//}
close(fd);
printf("线程结束\n");
}

int main()
{
	int sfd,newfd;

	signal(SIGPIPE,SIG_IGN);
	sfd = socket(AF_INET,SOCK_STREAM,0);
	int val;
	setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

	struct sockaddr_in info;

	info.sin_family = AF_INET;
	info.sin_port = htons(80);
	info.sin_addr.s_addr = 0;

	bind(sfd,(void*)&info,sizeof(info));

	listen(sfd,10);

	pthread_t tid;

	while(1)
	{
		newfd = accept(sfd,NULL,NULL);
		pthread_create(&tid,NULL,thread,(void*)newfd);

	}
	return 0;
}
