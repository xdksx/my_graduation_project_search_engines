

#include "conf.h"
#include "search.h"

int main(int argc,char *argv[])
{
	//传入数据库名并打开数据库
	DB *dbp;
	char *dbname="index_builded.db";
	opendb(&dbp,dbname,0);
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
			      FILE *FP=fdopen(cli_fd,"w");
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
		    fprintf(FP,"HTTP/1.1 200 OK\r\nServer: Test http se      rver\r\nConnection: close\r\n\r\n");
                    send(cli_fd,p,lenz,0);
                    free(p);  
                    close(cli_fd);
		    fclose(FP);
				}
				else//否则检索
				{
					 FILE *ClientFP = fdopen(cli_fd, "w");
					 
				      
				   //   char *se="HTTP/1.1 200 OK\r\nConnection:close\r\n\r\n<html><head><title>hrer</title></head><body><p>just teat</p></body></html>";
                  // int pp=send(cli_fd,se,300,0);
                 //  cout<<"pp:"<<pp<<endl;
              // fprintf(ClientFP,"HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: close\r\n\r\n<html><head><title>%s</title></head><body><p>just teat</p></body></html>",result.c_str());        
				 string search_str(Req+1);
					 int pos;
					 pos=search_str.find('=',0);//得到=号的位置
					 search_str=search_str.substr(pos+1,search_str.size()-pos);//get 搜索的内容
					 cout<<"sear:"<<search_str<<endl;
					 string search_str_d;
					 dealurl(search_str,search_str_d);
					string tmp("浮生若");
					string result("resutt:");
					cout<<"search:"<<search_str_d<<endl;
					 if(search_all(dbp,search_str_d)<0)
					 {
						   fprintf(ClientFP,"HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: keep-alive\r\n\r\n<html><head><title>%s</title></head><body><p>nothing founed</p></body></html>",result.c_str());        
                             fclose(ClientFP);
                             continue;
					 }
					 for(size_t y=0;y!=pageid_posright.size();y++)
                           cout<<"ksksksk:"<<pageid_posright[y];
                           cout<<endl;
                     
                    fprintf(ClientFP,"HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: keep-alive\r\nContent-type:text/html\r\n\r\n<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><title>%s</title></head><body>",result.c_str());        
                           
                           //打开另一个数据库,搜索pageid对应的网址和标题等,返回网址和标题
                    if(dbp!=NULL)
                                dbp->close(dbp,0);
                           char *dbname2="pageidsearch.db";
			                opendb(&dbp,dbname2,1);
			                DBC *cursorp; 
			                dbp->cursor(dbp,NULL,&cursorp,0);
	                        read_result_db(cursorp);
	                    for(int i=0;i!=result_str.size();i++)
	                    {
	                         fprintf(ClientFP,"<p><a href=\"%s\">result:%d:  %s</a></p>",result_str[i][0].c_str(),i+1,result_str[i][1].c_str());
	                         fprintf(ClientFP,"<p>  %s</p>",result_str[i][2].c_str());
		                }
	                         fprintf(ClientFP,"</body></html>");
	                           // close(cli_fd);
	             fclose(ClientFP);
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

