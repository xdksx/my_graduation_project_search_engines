

#include "conf.h"
#include "search.h"

int main(int argc,char *argv[])
{
	//传入数据库名并打开数据库
	DB *dbp,*dbp1,*dbp2;
	char *dbname="index_builded.db";
	char *dbname2="index_builded_oneword.db";
	opendb(&dbp,dbname,0);
	opendb(&dbp1,dbname2,0);
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
		    fprintf(FP,"HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: close\r\n\r\n");
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
					string result("search resutt:");
					cout<<"search:"<<search_str_d<<endl;
					int search_all_result=search_all(dbp,dbp1,search_str_d);
					char *return_to_user="HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: keep-alive\r\n\r\n<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><title>search result:</title></head><body><form>输入你要搜索的内容:<input type=\"text\" name=\"search\"><input type=\"submit\" value=\"点我搜\"></form><p>%s</p></body></html>";
				    char *return_1="数据库中没找到任何相关记录\n";
			//		char *return_2="在数据库中没找到该序列中的某个词元:\n";
			//	    char *return_3="找不到页面包含全部词元...\n";
			//		char *return_4="找到了对的页面,开始看这些词元是否相邻.....\n";
					if(search_all_result==-1)
					{
					    fprintf(ClientFP,return_to_user,return_1);  
					//	case -2:fprintf(ClientFP,return_to_user,return_2);  
						//         break;
					//	case -3:fprintf(ClientFP,return_to_user,return_3);  
						      //  break;
						//case -4:fprintf(ClientFP,return_to_user,return_4);  
						     //   break;
						   //fprintf(ClientFP,"",result.c_str());        
                             fclose(ClientFP);
                              clear_global_ds();
                             continue;
				    }
					/* for(size_t y=0;y!=pageid_posright.size();y++)
                           cout<<"ksksksk:"<<pageid_posright[y];
                           cout<<endl;
                     */
                                       fprintf(ClientFP,"HTTP/1.1 200 OK\r\nServer: Test http server\r\nConnection: keep-alive\r\nContent-type:text/html\r\n\r\n<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><title>%s</title></head><body><div style=\"background-color:white;height:70px;width:1000px;float:left;position:fixed;top:0;Z-index:1\"><form style=\"position:fixed;top:30\">输入你要搜索的内容:<input type=\"text\" name=\"search\"><input  type=\"submit\" value=\"搜索\"></form></div>",result.c_str());        
                           
                           //打开另一个数据库,搜索pageid对应的网址和标题等,返回网址和标题
                   // if(dbp!=NULL)
                       //         dbp->close(dbp,0);
                           char *dbname2="pageidsearch.db";
			                opendb(&dbp2,dbname2,1);
			                DBC *cursorp; 
			                dbp2->cursor(dbp2,NULL,&cursorp,0);
			               
			              
			              //  tf_sort_pageid(pageid_posright); 
			                vector<int> page_idabout(pageid_set_justabout.begin(),pageid_set_justabout.end());
			                cout<<"pageid_set_justabout:  "<<pageid_set_justabout.size()<<endl;
	                     /*    for(int i=0;i<page_idabout.size();i++)
	                              pos_about.push_back(5);*/
                              if(search_all_result==0)  //相邻或者单个字词
			                {
								cout<<"相邻或者单个字词"<<endl;
								 tf_sort_pageid(pageid_posright); 
								  tf_sort_pageid(pageid_set_notnear); 
								  cout<<"setnotnear:"<<pageid_set_notnear.size()<<endl; 
								   cout<<"setnotnear:"<<page_idabout.size()<<endl;
								  tf_sort_pageid(page_idabout); 
	                          read_result_db(cursorp,pageid_posright,posright);
	                          read_result_db(cursorp,pageid_set_notnear,pos_allhava);
	                          cout<<"over00:"<<endl;
	                          read_result_db(cursorp,page_idabout,pos_about);
	                          cout<<"over:"<<endl;
						    }
						    else if(search_all_result==7)
						    {
								cout<<"相邻或者单个字词"<<endl;
								 tf_sort_pageid(pageid_posright); 								 
	                          read_result_db(cursorp,pageid_posright,posright);
	                          cout<<"over:"<<endl;
						  }
	                        else if(search_all_result==-3) //有但是不相邻
	                        {
								cout<<"有但是不相邻"<<endl;
								  tf_sort_pageid(pageid_set_notnear); 
								  tf_sort_pageid(page_idabout); 
	                           read_result_db(cursorp,pageid_set_notnear,pos_allhava);
	                          read_result_db(cursorp,page_idabout,pos_about);
						      cout<<"over:"<<endl;
						    }
						    else if(search_all_result==-2)//不全且不相邻
	                        {
								cout<<"不全且不相邻"<<endl;
							  tf_sort_pageid(page_idabout); 
	                          read_result_db(cursorp,page_idabout,pos_about);
						      cout<<"over:"<<endl;
						    }
	                    for(int i=0;i!=result_str.size();i++)
	                    {
	                         fprintf(ClientFP,"<p style=\"position:relative;top:60px\"><a href=\"%s\">%d:  %s</a></p>",result_str[i][0].c_str(),i+1,result_str[i][1].c_str());
	                         fprintf(ClientFP,"<p style=\"position:relative;top:40px\"><small> %s　</small> </p>",result_str[i][2].c_str());
		                }
		                 fprintf(ClientFP,"<p style=\"position:relative;top:40px\">    %s</p>","以上为全部结果");
		              //   fprintf(ClientFP,"<p style=\"position:relative;top:40px\">  </p>");
	                         fprintf(ClientFP,"</body></html>");
	                           // close(cli_fd);
	                           clear_global_ds();
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

