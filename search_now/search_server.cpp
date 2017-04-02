

#include "conf.h"
#include "search.h"

int main(int argc,char *argv[])
{
	//传入数据库名并打开数据库
	DB *dbp;
	char *dbname="search12.db";
	opendb(&dbp,dbname);
  //   string tmp("浮生若梦人世事");
				//	 search_all(dbp,tmp);

//服务器部分:
     char *portin="port",*ethnamein="ethname",*backin="back";  
     get_arg("port");
     get_arg("ethname");
     get_arg("back");//取得listend监听个数
     int listennum=atoi(back);
     get_addr(ethname);
     struct sockaddr_in seraddr; 
     seraddr.sin_family=AF_INET;
     seraddr.sin_port=htons(atoi(port));//字符串转为int,在转为网络字节顺序的
     seraddr.sin_addr.s_addr=inet_addr(ip);
     
     int ser_sockfd;
	 if((ser_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	 {
		 perror("socket");
		 exit(EXIT_FAILURE);
	 }
	 
      //一个端口释放后会等待两分钟之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用。
	   int reuse = 1;
	   if (setsockopt(ser_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) 
	   {     
			      perror("setsockopt");
		          exit(EXIT_FAILURE);
		}
		
		socklen_t addlen=sizeof(struct sockaddr_in);
		 if(bind(ser_sockfd,(struct sockaddr *)&seraddr,addlen)<0)
	    {
		 perror("bind");
		 exit(EXIT_FAILURE);
	    }
	     if(listen(ser_sockfd,listennum)<0)
	    {
		 perror("listen");
		 exit(EXIT_FAILURE);
	     }
	     
	     int cli_fd;//接受的客户端fd
	  
	     while(1)
	     {
			 if((cli_fd=accept(ser_sockfd,(struct sockaddr*)&seraddr,&addlen))<0)
			 {
				 perror("accept");
				 exit(EXIT_FAILURE);
			 }
			 //输出接受的ip等
			 char buffer[1025];
	       bzero(buffer, 1025);
           sprintf(buffer, "connect come from: %s:%d\n",inet_ntoa(seraddr.sin_addr), ntohs(seraddr.sin_port));	

           int pid;
           if((pid=fork()==0))//在子进程中,处理发送接受
           {
			    int len;
		        char Req[129]="";
	            if((len=recv(cli_fd,buffer,1025,0))>0)//取得请求
	           {
		         cout<<buffer<<endl;
		         sscanf(buffer,"GET %s HTTP",Req);
                 cout<<"req:"<<Req<<endl;      
                     
                }
                
                if(strstr(Req,"=")==NULL)//如果不是搜索请求,提取的Req一般为/index.html或者其他文件
                {
					char *ind="/";
					char *inde="/index.html";
					if(strncmp(ind,Req,2)==0)
					{
						strcpy(Req,inde);
					}
					int fd = open(Req+1, O_RDONLY);//以只读的方式打开文件  
                    int lenz = lseek(fd, 0, SEEK_END);//获取文件的大小  
                    char *p = (char *) malloc(lenz + 1);
                    bzero(p, lenz + 1);
                    lseek(fd, 0, SEEK_SET);//从文件的开头开始读取
                    int retz = read(fd, p, lenz);//读取文件到p区域中 
                    close(fd); 
                    send(cli_fd,p,lenz,0);
                    free(p);  
				}
				else//否则检索
				{
					 string search_str(Req+1);
					 int pos;
					 pos=search_str.find('=',0);//得到=号的位置
					 search_str=search_str.substr(pos+1,search_str.size()-pos);//get 搜索的内容
					 cout<<"sear:"<<search_str<<endl;
					 string tmp("浮生若梦人世事");
					 search_all(dbp,tmp);
					 for(size_t y=0;y!=pageid_posright.size();y++)
                           cout<<"ksksksk:"<<pageid_posright[y];
                           cout<<endl;
				  }
                      
	     } 
	  else
	 {
		 close(cli_fd);
		 continue;
	 }       
	//由输入的文本分隔

	//TODO:
	

	     }




	return 0;
  }

