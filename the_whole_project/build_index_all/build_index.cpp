#include "build_index.h"
        ostream& operator<<(ostream& out,const postings_list& s)//重载输出操作符
	{
		out<<s.page_id<<"\t";
		for(size_t i=0;i!=s.pos.size();i++)
			out<<s.pos[i]<<"\t";
	        out<<s.count_of_page<<"\t";
		return out;
	} 
map<string,int> pagecount_exist_map;
multimap<string,postings_list> post_index;//内存中的倒排索引
/*-------------------------此部分为处理网页的函数,得到只有中文和英文（不包括标点及标签的）的文本,deal_tag可以修改过滤的标签,deal为主要的状态机,主要算法-----------------------*/
/*
 * 函数说明:
 * deal(string filename,char *result)传入文件名,返回结果字符串
 */

int deal_tag(string tag)//html剔除选项,可以扩展如只提取<p>标签
{
	if((tag.compare("script")==0))return 0;
	if((tag.compare("style")==0))return 0;
	else return 1;
}
int deal(string filename,char *result)//传入文件名和结果字符串,把网页文件提取的文本提取到字符串中
{ 
      ifstream in;
   //   cout<<"c_str():"<<filename.c_str()<<endl;
      in.open(filename.c_str(),fstream::in);//或者传入的参数是const char *,就不用使用c_str()转换了
	//先读取整个文件,保存在动态分配的内存中
      //ofstream out("yy");    //for test
      string resultq("");
      char *buffer=(char *)malloc(600000);//为网页分配的内存空间
      char *buffer_index=buffer;        //buffer用于放置网页的内容
      char *fr=buffer;
      int i=0;
      int  result_length=0;
       cout<<"--------------------------原始网页------------------------"<<endl;
      while(in.get(*buffer))
      {
		  //cout<<"d大调"<<endl;
        // cout<<*buffer;
	     buffer++;
         i++;
       }
      string tag("");
      int SIG_FILL=1;
      int SIG_SPACE=0;
	  Tag_State state=TAG_RIGHT_S;
	  CH_symbol state_CH_symbol=S0;
	  int sig_CH_symbol=0,sig_CH_symbol2=0;
	  char I0,I1;//中文字符过滤用
	  int signal=0;//过滤中文字符及其他字符时用
	  string old_tag("");
      while(i>0)
      {
	 // Tag_State state=TAG_RIGHT_S;
          switch(state)
	  {
             case TAG_RIGHT_S://<xx>/</xx>/<xx >
               {
	          if(*buffer_index=='<')
		      {
			   state=TAG_LEFT;//change state;
               tag="";
			   SIG_SPACE=0;
		      }
		      else if(*buffer_index=='>'&&SIG_FILL==1);//这里出现的是<xx>  >
			//  out<<*buffer_index;
		      else if(*buffer_index!='<'&&*buffer_index!='>'&&SIG_FILL==1&&*buffer_index!='/'&&((*buffer_index>='a'&&*buffer_index<='z')||(*buffer_index>='A'&&*buffer_index<='Z')||(*buffer_index==' ')))
			  {
				*result=*buffer_index;
					result++;
			//	out<<*buffer_index;    //for test
				result_length++;
			}
			else if(*buffer_index<0&&SIG_FILL==1)//在这里面放置一个剔除中文字符的有限状态机
			{
				switch(state_CH_symbol)
				{
					case S0:
					{
						if(*buffer_index==-29)//E3的补码
						{
							  state_CH_symbol=S1;
							  I0=*buffer_index;
						  }
						else if(*buffer_index==-17)//EF
						{
							  state_CH_symbol=S3;
							  I0=*buffer_index;
						 }
						else if(*buffer_index>-29&&*buffer_index<-17&&signal==0)
						{
							signal=1;
							*result=*buffer_index;
					        result++;
						//    out<<*buffer_index;
						    result_length++;
						}
						else if((*buffer_index<-29||*buffer_index>-17)&&*buffer_index>=-32)//过滤掉其他如下箭头等
						   signal=0;
						   else if(*buffer_index<-32&&*buffer_index>=-64)signal=0;//比如碰到点C2B7
						 else if(signal==1)
						 {
							 *result=*buffer_index;
					        result++;
					//		 out<<*buffer_index;
							 result_length++;
						 }
					 }
					  break;
					 case S1:
					  {
						  if(*buffer_index==-122)//86
						  {
							  state_CH_symbol=S2;
							  I1=*buffer_index;
						  }
						  else if(*buffer_index!=-122&&sig_CH_symbol==0)//<86为中文标点符号等>86,为中文
						  {
							  sig_CH_symbol++;
							  I1=*buffer_index;
						  }
						  else if(I1<-122&&sig_CH_symbol==1)//出现一次了
						   {
							   sig_CH_symbol=0;
							   state_CH_symbol=S0;
						   }
						   else if(I1>-122&&sig_CH_symbol==1)
						   {
							   sig_CH_symbol=0;
							   state_CH_symbol=S0;
							   *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=*buffer_index;
					           result++;
							 //  out<<I0<<I1<<*buffer_index;
							   result_length=result_length+3;
						   }
						}
					  break;
					  case S2:
					  {
						  if(*buffer_index<=-111)
					            state_CH_symbol=S0;
					      else if(*buffer_index>-111)
					      {
							  state_CH_symbol=S0;
							    *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=*buffer_index;
					           result++;
							 // out<<I0<<I1<<*buffer_index;
							  result_length=result_length+3;
						  }
					  }
					         break;
					  case S3:
					      {
							  if(*buffer_index==-85)//AB
							  {
								  	  state_CH_symbol=S0;
								  	  I1=*buffer_index;
							  }
							   else if(*buffer_index!=-85&&sig_CH_symbol2==0)
							   {
						            sig_CH_symbol2++;
							        I1=*buffer_index;
							   }
							    else if(I1>-85&&sig_CH_symbol2==1)//出现一次了
						        {
							       sig_CH_symbol2=0;
							       state_CH_symbol=S0;
						        }
						        else if(I1<-85&&sig_CH_symbol2==1)
						       {
							   sig_CH_symbol2=0;
							   state_CH_symbol=S0;
							     *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=*buffer_index;
					           result++;
							   //out<<I0<<I1<<*buffer_index;
							   result_length=result_length+3;
						       }	        
						   }
						   break;
					case S4:
					{
						if(*buffer_index>=-114)
						      state_CH_symbol=S0;
						else if(*buffer_index<-114)
						{
							     //  out<<I0<<I1<<*buffer_index;
							         *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=*buffer_index;
					           result++;
					           result_length=result_length+3;
							       state_CH_symbol=S0;
					    }
			  	     }   
				  break;
			  }
		     }
	       }
	       break;
	     case TAG_LEFT://state: <
	       {
		       if(*buffer_index=='/')
			     state=TAG_LEFTA;//change state to </
		       else if(*buffer_index!='/'&&*buffer_index!='>'&&*buffer_index!='<'&&SIG_FILL==1)
		       {
			       state=TAG_LEFTS;//change state <xx
                               tag.append(1,*buffer_index);
		       }
	       }
	       break;
	     case TAG_LEFTA://</
	       {
		       if(*buffer_index!='>'&&*buffer_index!='<')
                           tag.append(1,*buffer_index);
		       else if(*buffer_index=='>')
		       {
			      state=TAG_RIGHT_S;//change state </xx>
			      if(tag.compare(old_tag)==0)
			      {
			           SIG_FILL=1;
			           old_tag="";
				   }
			   }
	        }
	       break;
             case TAG_LEFTS://<xx
	       {
		       if(*buffer_index!='>'&&*buffer_index!='<'&&*buffer_index!='/'&&*buffer_index!=' '&&SIG_SPACE==0)
			       tag.append(1,*buffer_index);
		       else if(*buffer_index==' '&&SIG_SPACE==0)//<a 
                       {
			       if((deal_tag(tag)==0))
			       {
					   SIG_FILL=0;
					   old_tag=tag;
				   }
			       else if((deal_tag(tag)==1))SIG_FILL=1;
                               SIG_SPACE=1;
		       }
		       else if(*buffer_index=='>')
		       {
			       if(SIG_SPACE==0)
			       {
				       if(deal_tag(tag)==0)
				       {
						   SIG_FILL=0;
				           old_tag=tag;
					   }
				       else if(deal_tag(tag)==1)SIG_FILL=1;
			       }
			       state=TAG_RIGHT_S;//change state <xx>/<xx >
		       }
	       }
	       break;
      }
          buffer_index++;
         i--;
       }	 
      in.close();
     // out.close();   //for test
     free(fr);
     return result_length;
}



/*----------------此部分为建立内存中的倒排索引的函数-------------------*/
/*函数说明:
 * 传入由n-gram算法得到的词元及词元的位置等信息填充到倒排索引数据结构中
 */
int build(string word,int page_id_i,int pos_i)//把词元和词元的位置等信息构成内存中的倒排索引
{ 
	int sig=0;
	int  entries=post_index.count(word); //计算词元对应value的数量
	multimap<string,postings_list>::iterator iter=post_index.find(word);//有该词元记录,则找位置,看..
	for(int cnt=0;cnt!=entries;++cnt,++iter)
	{
		if(iter->second.page_id==page_id_i)//同一页的不同位置,则
		{
			sig=1;
			iter->second.pos.push_back(pos_i);
                        iter->second.count_of_page++;//在该页面的词频加１
			return 0;
 		}
 	}
	if(sig==0)//同时包含不存在词元记录和有词元无对应页的情况
	{
	   postings_list list;
	   list.page_id=page_id_i;
	   list.count_of_page=1;//在该页面中首次出现该词
	   list.pos.push_back(pos_i);
       post_index.insert(pair<string,postings_list>(word,list));
	} 
	return 0;
}
/*------函数说明
 * n-gram算法,同时存入内存倒排索引中
 * 传入提取的网页正文rawstr,长度,页面id
 */
int build_post_index_m(char* rawstr,int length,int page_id_i)//N-gram 分割和构建倒排索引,内存中
{
     string instr("");
    // ofstream out("dd");
     int pos_i=0;//记录位置,全局位置
     char *p1=rawstr;//set two point to get 2-gram word
     int len_bu=length;
     n_gram_SIG state=INIT;
     int count_ch=0;//记录遍历一个中文字时的已遍历中文的字节数
     while(len_bu>0)//使用有限状态自动机
     {
         switch(state)
	    {
		 case INIT:
		{
			if((*p1>=97&&(*p1)<=122)||((*p1>=65&&(*p1)<=90)))//en or other,先只考虑中英文
			{
				state=EN;
				instr.append(1,*p1);
			}
			else if((*p1)>=-32&&(*p1)<0)
			{
				state=CH;
				count_ch++;//计算出现的中文字符数
				instr.append(1,*p1);
			}
		//	cout<<"in init"<<endl;
		}
		break;
		 case EN:
		{
			if((*p1>=97&&(*p1)<=122)||((*p1>=65&&(*p1)<=90)))//en or other,先只考虑中英文
		            instr.append(1,*p1);
			else if((*p1)>=-32&&(*p1)<0)//若是中文
			{
				//调用填充到索引(构建索引的函数中)
                build(instr,page_id_i,pos_i);
                pos_i++;
				state=CH;
				instr="";
				instr.append(1,*p1);
				count_ch=1;
			}
			else //if(*p1==' '||*p1=='\n')
			{
				//调用函数填充到索引
                build(instr,page_id_i,pos_i);
                if(*p1==' ')
                                pos_i++;
				state=INIT;
		      		count_ch=0;
                                instr="";
            }
          //  cout<<"in en"<<endl;
                         
		}
		break;
		 case CH:
		{
			if(*p1==' '||*p1=='\n')
			{
				//调用函数填充到索引
                                build(instr,page_id_i,pos_i);
                                if(*p1==' ')
                                                pos_i++;
				state=INIT;
				count_ch=0;
				instr="";
			}
			else if(*p1<-32)//n个半个汉字
				instr.append(1,*p1);
			else if(count_ch==1&&*p1>=-32&&*p1<0)//开始第二个汉字
			{
				instr.append(1,*p1);
				count_ch++;
			}
			else if(count_ch==2&&*p1>=-32&&*p1<0)//开始第三个汉字
			{
                            //调用函数填充索引
                        build(instr,page_id_i,pos_i);
                                        pos_i++;
			    instr="";//重新,2-gram
			 //   if((*p1)>=-16&&*p1<0)//如果是四个字节的
			   // {
				  //  cout<<*p1;
				//    p1--;
				//    len_bu++;
			  //  }
			    p1=p1-3;
			    len_bu=len_bu+3;
			    instr.append(1,*p1);
			    count_ch=1;
                        }
			else if((*p1>=97&&(*p1)<=122)||((*p1>=65&&(*p1)<=90)))//en or other,先只考虑中英文
			{
				//调用函数填充索引
                                build(instr,page_id_i,pos_i);
                                                pos_i++;
				//cout<<"here:"<<instr<<endl;
				state=EN;
				instr="";
				instr.append(1,*p1);
                                count_ch=0;
			}
		    else
			{
				//调用函数填充到索引
                                build(instr,page_id_i,pos_i);
                                if(*p1==' ')
                                                pos_i++;
				state=INIT;
				count_ch=0;
				instr="";
			}
		//	out<<instr;
		//	cout<<instr<<"in ch"<<endl;
		}
		break;
          }
	 p1++;
	 len_bu--;
      }
    return 0;
} 
/*------函数说明
 * 1-gram算法,同时存入内存倒排索引中
 * 传入提取的网页正文rawstr,长度,页面id
 */
int build_post_index_o(char* rawstr,int length,int page_id_i)//1-gram 分割和构建倒排索引,内存中
{
     string instr("");
     int pos_i=0;//记录位置,全局位置
     char *p1=rawstr;//set two point to get 2-gram word
     int len_bu=length;
     n_gram_SIG state=INIT;
     while(len_bu>0)//使用有限状态自动机
     {
         switch(state)
	    {
		 case INIT:
		{
			if(*p1>48&&(*p1)<128)//en or other,先只考虑中英文
			{
				state=EN;
				instr.append(1,*p1);
			}
			else if((*p1)>=-32&&(*p1)<0)
			{
				state=CH;
				instr.append(1,*p1);
			}
		}
		break;
		 case EN:
		{
			if(*p1>48&&*p1<128)//若是英文
		            instr.append(1,*p1);
			else if((*p1)>=-32&&(*p1)<0)//若是中文
			{
				//调用填充到索引(构建索引的函数中)
                build(instr,page_id_i,pos_i);
                pos_i++;
				state=CH;
				instr="";
				instr.append(1,*p1);
			}
			else if(*p1==' '||*p1=='\n')
			{
				//调用函数填充到索引
                             build(instr,page_id_i,pos_i);
                                pos_i++;
				state=INIT;
                                instr="";
                         }
		}
		break;
		 case CH:
		{
			if(*p1==' '||*p1=='\n')
			{
				//调用函数填充到索引
                                build(instr,page_id_i,pos_i);
                                                pos_i++;
				state=INIT;
				instr="";
			}
			else if(*p1<-32)//n个半个汉字
				instr.append(1,*p1);
			else if(*p1>=-32&&*p1<0)//开始第二个汉字
			{
				build(instr,page_id_i,pos_i);
				pos_i++;
				state=CH;
			        instr="";
		                instr.append(1,*p1);
			}
			else if(*p1>48&&*p1<128)//EN
			{
				//调用函数填充索引
                                build(instr,page_id_i,pos_i);
                                                pos_i++;
				//cout<<"here:"<<instr<<endl;
				state=EN;
				instr="";
				instr.append(1,*p1);
			}
		}
		break;
         }
	 p1++;
	 len_bu--;
     }
    return 0;
}

/*----------------------此部分函数为将内存中的倒排索引保存到数据库中的相关函数----------*/
/*
 * 函数说明:
 * 传入word作为key,并以buffer返回结果
 */
int map_to_char(char *buffer,string word)//将索引(即key-对应的value)以字符串的形式存到char *buffer中
{
	int allcount=0;
    multimap<string,postings_list>::iterator iter=post_index.find(word);
    int word_count=post_index.count(word);
	postings_list tmp=(*iter).second;
//	cout<<"word--:"<<word<<endl;
	//cout<<"url:"<<tmp.url<<endl; 
     sprintf(buffer,"%d",pagecount_exist_map[word]);
     buffer=buffer+page_id_size+1;
     allcount=allcount+page_id_size+1;
    for(int cnt=0;cnt!=word_count;++cnt,++iter)
    {
	  
	//postings_list tmp=(*iter).second;
       // sprintf(buffer,"%d",(*iter).second.page_id);
        sprintf(buffer,"%d",tmp.page_id);     //20170409修改,考虑到页面较多,4个字节的字符串可表示的数字有限,故用10个字节
	//printf("1:%s\n",buffer);
	buffer=buffer+page_id_size+1;//这里得测试一下
	allcount=allcount+page_id_size+1;
//	for(vector<int>::it=(*iter).second.pos.begin();it!=(*iter))
	sprintf(buffer,"%d",tmp.count_of_page);
	buffer=buffer+pos_size+1;
	allcount=allcount+pos_size+1;
	for(vector<int>::iterator it=tmp.pos.begin();it!=tmp.pos.end();++it)//考虑到网页正文可能很多,分割网页显得很麻烦,分配给位置信息多一些空间,6位
	{
           sprintf(buffer,"%d",*it);
//	     printf("1:%s\n",buffer);
	   buffer=buffer+pos_size+1;
	   allcount=allcount+pos_size+1;
     }
        *buffer='*';//作为分隔符号
	allcount=allcount+1;
//	printf("1:%s\n  %d",buffer,allcount);
    }
    return allcount;
} 
        	

int print_post_index()//输出内存构建的索引
{
    for(multimap<string,postings_list>::iterator it=post_index.begin();it!=post_index.end();++it)
    {
	    cout<<(*it).first<<"=>"<<pagecount_exist_map[(*it).first]<<"\t"<<(*it).second<<"\n";
    }
    return 0;
}

void printchars(char *str,int len)//输出从数据库中得到的记录,测试用
{
	int n=0;
	while(n<len)
	{
		n++;
	  printf("%c",*str);
	   str++;
   }
   cout<<endl;
}
/*此函数和上面的deal函数都是处理传入的文件，上面deal处理的是原始的网页文件，而这个函数的存在是因为做爬虫的时候已经对网页进行了提取正文，
 * 所以这个函数只需要剔除中文标点符号，只保留英文和中文即可．
 * 这里暂时只调用这个函数，可以根据具体情况调整
 */
int deal_hasget_text(string filename,char *result)//传入文件名和结果字符串,把网页文件提取的文本提取到字符串中
{ 
      ifstream in;
      in.open(filename.c_str(),fstream::in);//或者传入的参数是const char *,就不用使用c_str()转换了
	//先读取整个文件,保存在动态分配的内存中
      int  result_length=0;
      char getin;
       int sig_CH_symbol=0,sig_CH_symbol2=0;
	  char I0,I1;//中文字符过滤用
      int signal=0;
      CH_symbol state_CH_symbol=S0;
      while(in.get(getin))
      {
        if((getin<='z'&&getin>='a')||(getin>='A'&&getin<='Z')||(getin==' ')||(getin=='\n'))
	      {
				*result=getin;
				result++;
				result_length++;
			}
			else if(getin<0)//在这里面放置一个剔除中文字符的有限状态机
			{
				switch(state_CH_symbol)
				{
					case S0:
					{
						if(getin==-29)//E3的补码
						{
							  state_CH_symbol=S1;
							  I0=getin;
						  }
						else if(getin==-17)//EF
						{
							  state_CH_symbol=S3;
							  I0=getin;
						 }
						else if(getin>-29&&getin<-17&&signal==0)
						{
							signal=1;
							*result=getin;
					        result++;
						    result_length++;
						}
						else if((getin<-29||getin>-17)&&getin>=-32)//过滤掉其他如下箭头等
						   signal=0;
						   else if(getin<-32&&getin>=-64)signal=0;//比如碰到点C2B7
						 else if(signal==1)
						 {
							 *result=getin;
					        result++;
					//		 out<<*buffer_index;
							 result_length++;
						 }
					 }
					  break;
					 case S1:
					  {
						  if(getin==-122)//86
						  {
							  state_CH_symbol=S2;
							  I1=getin;
						  }
						  else if(getin!=-122&&sig_CH_symbol==0)//<86为中文标点符号等>86,为中文
						  {
							  sig_CH_symbol++;
							  I1=getin;
						  }
						  else if(I1<-122&&sig_CH_symbol==1)//出现一次了
						   {
							   sig_CH_symbol=0;
							   state_CH_symbol=S0;
						   }
						   else if(I1>-122&&sig_CH_symbol==1)
						   {
							   sig_CH_symbol=0;
							   state_CH_symbol=S0;
							   *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=getin;
					           result++;
							 //  out<<I0<<I1<<*buffer_index;
							   result_length=result_length+3;
						   }
						}
					  break;
					  case S2:
					  {
						  if(getin<=-111)
					            state_CH_symbol=S0;
					      else if(getin>-111)
					      {
							  state_CH_symbol=S0;
							    *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=getin;
					           result++;
							 // out<<I0<<I1<<*buffer_index;
							  result_length=result_length+3;
						  }
					  }
					         break;
					  case S3:
					      {
							  if(getin==-85)//AB
							  {
								  	  state_CH_symbol=S0;
								  	  I1=getin;
							  }
							   else if(getin!=-85&&sig_CH_symbol2==0)
							   {
						            sig_CH_symbol2++;
							        I1=getin;
							   }
							    else if(I1>-85&&sig_CH_symbol2==1)//出现一次了
						        {
							       sig_CH_symbol2=0;
							       state_CH_symbol=S0;
						        }
						        else if(I1<-85&&sig_CH_symbol2==1)
						       {
							   sig_CH_symbol2=0;
							   state_CH_symbol=S0;
							     *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=getin;
					           result++;
							   //out<<I0<<I1<<*buffer_index;
							   result_length=result_length+3;
						       }	        
						   }
						   break;
					case S4:
					{
						if(getin>=-114)
						      state_CH_symbol=S0;
						else if(getin<-114)
						{
							     //  out<<I0<<I1<<*buffer_index;
							         *result=I0;
					           result++;
					           *result=I1;
					           result++;
					           *result=getin;
					           result++;
					           result_length=result_length+3;
							       state_CH_symbol=S0;
					    }
			  	     }   
				  break;
			   }
		     }
       }
       in.close();
       return result_length;
 }










