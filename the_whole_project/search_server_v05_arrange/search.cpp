
#include "search.h"

ostream& operator<<(ostream& out,const postings_list& s)
	{
		out<<s.page_id<<"\t";
		for(size_t i=0;i!=s.pos.size();i++)
			out<<s.pos[i]<<"\t";
		out<<s.count_of_page<<"\t";
		return out;
	}
vector<string> search_str; //存放搜索的词元集合
vector<string>search_str_in; //存放存在于数据库中的词元集合
map<string,int> pagecount_exist_map;   //存放某次元在多少个文档中出现，用于求idf
multimap<string,postings_list> post_index;//存放待检索的词元所有倒排数据，即内存map

//搜索词元在某个文档都相邻时即完全匹配搜索序列的情况使用的数据结构  情况1
vector<vector<multimap<string,postings_list>::iterator> > pageid_ptrset;//存放在同一个文档都出现但是位置不知道是否相邻的post_index数据指针集合的集合
     //第一维是page数量,即第一个词元都在的页面,第二个..,第二维是对应page的词元的倒排列表指针集合,即第一个词元倒排列表所在位置.第二个...,以供后面的判断位置是否相邻
vector<int> pageid_posright;     //存放所有（用户输入的查询字符串提取出来的）词元在数据库中相邻（即准确获取句子）的pageid
vector<int> posright;  //存放上面pageid对应的词元在文档中的位置

//搜索词元均包含在page中却不相邻   情况2
vector<int> pageid_set_notnear;  //保存那些包含所有词元但是不相邻的页面（供召回）
vector<int> pos_allhava;  //存放那些包含所有词元但是不相邻的页面的位置，供返回结果摘要用

//某几个搜索词元出现在某些文档中   情况3
set<int> pageid_set_justabout;//保存那些出现过某个词元的页面（供召回）
vector<int> pageid_set_about;  //供上面的set提供来源（临时保存，set去重）
vector<int> pos_about; 

//返回的结果数据结构
vector<vector<string> > result_str; //存放从第二个数据库中返回的结果字符串

/*---------此部分为检索时分割用户传入的字符串,同样以n-gram 方式分割,主要和构建索引部分的分割方式一致,所以若是构建索引的方式改变,则这里也要改变----*/
/*函数说明:
 * 传入用户输入的字符串和长度,引用方式返回得到的多个字符串
 */ 
 
 void clear_global_ds()
 {
	post_index.clear();
	search_str.clear();
	search_str_in.clear();
	pagecount_exist_map.clear();
    
    pageid_ptrset.clear();
	pageid_posright.clear();
	posright.clear();
		
	pageid_set_notnear.clear();
	pos_allhava.clear();
	
	pageid_set_justabout.clear();
	pageid_set_about.clear();
	pos_about.clear();
	
	result_str.clear();
 }
 
 /*输出读取的数据库记录,供测试----*/
void printchars(char *str,int len)
{
	int n=0;
	while(n<len)
	{
		n++;
	  printf("%c ",*str);
	   str++;
   }
   cout<<endl;
}


/*简单处理url的函数,即把url:中的中文即:像"%E3%E4...+ida...%E2.."这种中英文混合的方式,分割出来中文加英文如:"我喜欢google搜索"*/
//暂时未处理:  fhusd+  
int dealurl(string src,string &des)
{
	int move[2];
	int moveresult;
	char toi[2];
	for(size_t i=0;i!=src.size();i++)
	{
		if(src[i]=='%')
	        {
	         for(int j=0;j<2;j++)
		     {
                     toi[0]=src[++i];//
		     if(toi[0]>='A'&&toi[0]<='F')
		     {
                         move[j]=toi[0]-55;
		 	// printf("move:%x\n",move[j]);
		      }
               else if(toi[0]>='0'&&toi[0]<='9')
		       {
	          	 move[j]=toi[0]-48;
		   //	printf("move2:%x\n",move[j]);
		      }
		 }
               moveresult=(move[0]<<4)+move[1];
		   //  printf("moveresult:%x\n",moveresult);
		     des.append(1,(char)moveresult);
		}
		else if(src[i]=='+')
		      des.append(1,' ');
                else 
		      des.append(1,src[i]);
	}
	return 0;
}
 
 
 /*-----------------------数据库操作相关函数---------------------*/
 /*打开数据库,根据标志判断是否打开一键多值的数据库,0则不为一键多值,1则是*/
int opendb(DB **dbp,char *db_name,int flag_dup)
{
	////打开数据库
//	DB *dbp;
	u_int32_t flags=DB_CREATE;
	int ret ;
	ret=db_create(dbp,NULL,0);
	if(ret!=0)
	{
	    cout<<"error create\n"<<endl;
            return -1;
	}
	if(flag_dup==1)
              ret=(*dbp)->set_flags(*dbp,DB_DUP);	     
    ret=(*dbp)->open(*dbp,NULL,db_name,NULL,DB_BTREE,flags,0);
	if(ret!=0)
	{
	   cout<<"error open\n";
	   return -1;
	}
	   return 0;
}


/*读取数据库记录,k为词元,读取其对应的value,存入相关信息到post_index------*/
int read_db(string k,DB *db)    
{
	DBT key,data;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));
	char *words=NULL;
	words=(char *)malloc(k.size());//之前这里没错一直没有(malloc,但是在build_index中这么用却错了,具体原因可见那个文件,而这里的malloc却一直没错,
                                        //不过保险起见,还是把它改成以下数组形式吧
	char *words_fr=words;
	//const int words_length=k.size();
	//char words[words_length];
	memset(words,0,k.size());
	memcpy(words,k.c_str(),k.size());
	char *buffer=(char *)malloc(read_db_buffer);
	char *buffer_free=buffer;
	memset(buffer,0,read_db_buffer);

	key.data=words;
	key.size=k.size();

	data.data=buffer;
	data.ulen=read_db_buffer;
	data.flags=DB_DBT_USERMEM;
	if((db->get(db,NULL,&key,&data,0)==DB_NOTFOUND))
	{
		cout<<"sorry ,not found\n"<<endl;
		return -1;
	}
	else //把取出的词元对应的倒排列表存入内存multimap中,数据结构为post_index
	{
		    search_str_in.push_back(k);
    //   printchars(buffer,data.size);
		    int num=0;
	        postings_list tmp;
	        string str((char *)(key.data));	
	        
	        while(num<data.size)
		   {
			pagecount_exist_map[k]=atoi(buffer);
			//printf("pagecount_exist_map:%s\n",buffer); 
			buffer=buffer+page_id_size+1;
			num=num+page_id_size+1;
		
			tmp.page_id=atoi(buffer);//取出pageid
		 //	printf("pageid:%s \n",buffer); 
		 	cout<<"word:"<<k<<endl;
			buffer=buffer+page_id_size+1;
			num=num+page_id_size+1;
			pageid_set_about.push_back(tmp.page_id); //存入词元出现的pageid供召回
			
		//		printf("count_of_page:%s\n",buffer); 
	        tmp.count_of_page=atoi(buffer);
		    buffer=buffer+pos_size+1;
		    num=num+pos_size+1;
		    
			while((*buffer>='0'&&*buffer<='9')||*buffer=='\0')
			{
		          tmp.pos.push_back(atoi(buffer));
			//	printf(":%s",buffer); 
				buffer=buffer+pos_size+1;
				num=num+pos_size+1;
			}
			
		//	printf("is the last signal of a pageid-list:%c",*buffer); 
			buffer++;   //"*"
			num=num+1;
			//cout<<"\nddd"<<*buffer<<endl;
            post_index.insert(pair<string,postings_list>(k,tmp));//这里之前写的是str,内容见
	        tmp.clear();
		}
	}
	free(words_fr);
	free(buffer_free);
	return 0;
}

/*在倒排列表中找到page之后,此函数用于在另一个数据库中,根据建为pageid来查找对应的url,标题和部分文本,以反馈给用户链接和相关信息*/

int read_result_db(DBC *cursorp,vector<int> pageid_set_r,vector<int> pos_set_r)
{
	cout<<"in result db:"<<endl;
	DBT key,data;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));
    char *buffer=(char *)malloc(read_result_db_buffer);
	char *buffer_free=buffer;
	memset(buffer,0,read_result_db_buffer);
	int count=0;
	int ret;
	char buffer_word[11];
    for(int i=0;i!=pageid_set_r.size();i++)
    {
		memset(buffer_word,0,11);
		sprintf(buffer_word,"%d",pageid_set_r[i]);	
		for(int k=1;k<10;k++)
		{
			if(buffer_word[k]<'0'||buffer_word[k]>'9')
			    buffer_word[k]=' ';
	    }
	    key.data=buffer_word;
	    key.size=10;
	    data.data=buffer;
	    data.ulen=read_result_db_buffer;
	    data.flags=DB_DBT_USERMEM;
	    ret=cursorp->get(cursorp,&key,&data,DB_SET);
	    if(ret<0)
	       cout<<"in result db ,nothing founded"<<endl;
	    count++;
	    vector<string> tmp; 
	    tmp.push_back(string(buffer));
	//    printf("key:%s   %d \n",buffer_word,sizeof(buffer_word));
        while(ret!=DB_NOTFOUND&&count<3)
       {
	 //  printf("key:%s\n",buffer_word);
	    printf("data:%s\n",data.data);
	    ret=cursorp->get(cursorp,&key,&data,DB_NEXT_DUP);
	    cout<<"in the block:"<<ret<<endl;
        count++;
        if(count==2)
			tmp.push_back(string(buffer));
	    if(count==3) //只反馈部分文本及处理边界问题以避免返回的文本包含乱码,即不完整的中文
	    { 
			string getin("");
			int pos_gettext=pos_set_r[i]-100;
			int flag_gettext=0;
			for(int k=0;k!=data.size;k++,buffer++)
			{
				if(k>=pos_gettext&&k<=pos_gettext+150)
				{
					//getin.append(1,*buffer);
					   if(flag_gettext==1)
					    {
							getin.append(1,*buffer);
							continue;
						}
					    if(*buffer>=-32&&*buffer<=-16&&flag_gettext==0)//if chinese first
					    {
	                        getin.append(1,*buffer);
	                        flag_gettext=1;
						}
	                    else if(*buffer>=0&&flag_gettext==0)
	                    {
	                        getin.append(1,*buffer);
	                        flag_gettext=1;
						}
	                    else if((*buffer<-32||(*buffer>-16&&*buffer<0))&&flag_gettext==0);//filter  
	                         
	            }        
		    }
		    int filt_getin=getin.size()-1;
		    //Exx XEX  xxE XUE //这以下的代码是为了踢出检索出来的摘要部分结尾的乱码
		    if((getin[filt_getin]<-32||(getin[filt_getin]>-16&&getin[filt_getin]<0))&&(getin[filt_getin-2]<-32||(getin[filt_getin-2]>-16&&getin[filt_getin-2]<0)))//判断是否最后三个字节,倒数第一个和第三个都不是中文utf8开头的字符,即最后的字构不成中文
	        {
				getin[filt_getin]=' ';
				getin[filt_getin-1]=' ';
			}
			else if(getin[filt_getin]>=-32&&getin[filt_getin]<=-16)
			    getin[filt_getin]=' ';
	        
	        tmp.push_back(getin);
		 }
       }
        count=0;
        result_str.push_back(tmp);
    }
  /*  for(int i=0;i!=result_str.size();i++)
    {
		;//cout<<"1: "<<result_str[i][0]<<"\n 2: "<<result_str[i][1]<<"\n 3: "<<result_str[i][2]<<endl;
	}*/
	free(buffer_free);
	return 0;
}


/*-------------------------检索过程相关函数-----------------------------*/
//前期处理
int split_searchstr(const char* rawstr,int length)//N-gram分隔请求的字符串
{
     string instr("");
     const char *p1=rawstr;//set two point to get 2-gram word
     int len_bu=length;
     n_gram_SIG state=INIT;
     int count_ch=0;
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
				count_ch++;//计算出现的中文字符数
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
				//cout<<"here:"<<instr<<endl;
                search_str.push_back(instr);
				state=CH;
				instr="";
				instr.append(1,*p1);
				count_ch=1;
			}
			else if(*p1==' '||*p1=='\n')
			{
                search_str.push_back(instr);
				//cout<<"here:"<<instr<<endl;
				state=INIT;
		      	count_ch=0;
                instr="";
             }
		}
		break;
		 case CH:
		{
			if(*p1==' '||*p1=='\n')
			{
             search_str.push_back(instr);                                
				//cout<<"here:"<<instr<<endl;
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
			else if(count_ch==2&&*p1>=-32&&*p1<0)//开始第三个汉字   //这里的实现方式不太好,可扩展性不强,可以考虑在读了n-gram中的n个中文后转入init状态,遇到下一个中文开头再转入CN状态.这里得固定中文的utf为3字节
			{
                            //调用函数填充索引
                                search_str.push_back(instr);
			//	state=CH;
			//	cout<<"here:"<<instr<<endl;
			    instr="";//重新,2-gram
			    if((*p1)>=-16&&*p1<0)//如果是四个字节的
			    {
				  //  cout<<*p1;
				    p1--;
				    len_bu++;
			    }
			    p1--;
			    p1--;
			    p1--;
			    len_bu=len_bu+3;
			    instr.append(1,*p1);
		            count_ch=1;
                       }
			else if(*p1>48&&*p1<128)//EN
			{
				//调用函数填充索引
                                search_str.push_back(instr);
                                               
				//cout<<"here:"<<instr<<endl;
				state=EN;
				instr="";
				instr.append(1,*p1);
                                count_ch=0;
			}
		}
		break;
         }
	 p1++;
	 len_bu--;
     }
     search_str.push_back(instr);				//防止最后一个字节遗漏以及处理了单个词元（即两个汉字）和单个英文单词的情况
     return 0;
}


/*--------此部分为快速排序,以排序词元对应的pageid个数进行排序,以减少检索时的时间复杂度-----*/

int partion(vector<string> &strs,int lo,int hi)
{
    int i=lo,j=hi+1;
    size_t v=post_index.count(strs[lo]);
    string tmp;
    while(1)
    {   
	    while(post_index.count(strs[++i])<v)
	    if(i==hi)break;
	    while(post_index.count(strs[--j])>v)
	    if(j==lo)break;
	    if(i>=j)break;
            tmp=strs[i];
	    strs[i]=strs[j];//不知道能不能这样赋值,测试看看
	    strs[j]=tmp;
    }
    tmp=strs[lo];
    strs[lo]=strs[j];
    strs[j]=tmp;
    return j;
}

void sort_by_count(vector<string> &strset,int lo,int hi)
{
          if(hi<=lo)return ;
	  int j=partion(strset,lo,hi);
	  sort_by_count(strset,lo,j-1);
	  sort_by_count(strset,j+1,hi);
}



//xapian
//检索主要函数
/*传入按照pageid个数排序好的词元数组，得到词元均在同一个文档的指针集结构*/
int search(vector<string> search_strset)
{
	int pageid;
     //得到第一个key的value数量
     int firkey_count=post_index.count(search_strset[0]);
     //接着得到它的位置:
     multimap<string,postings_list>::iterator fir_iter=post_index.find(search_strset[0]);
     //将除第一个之外的每个词元的指针包存起来,成游标
     vector<multimap<string,postings_list>::iterator> ptrset;
     vector<int> count;
     int return_value=0;
     int num_pageid_right=0;
     vector<int> pos_num;//游标,记录当前词元第几个pageid,防止越界及及时终止
     for(size_t it=0;it!=search_strset.size();it++)//初始化存入：
      {
	     pos_num.push_back(0);
	     count.push_back(post_index.count(search_strset[it]));//存入每个词元的倒排列表pageid数
	     ptrset.push_back(post_index.find(search_strset[it]));//存入每个词元在内存map的起始位置
        // cout<<" "<<post_index.count(search_strset[it])<<" "; //输出倒排列表pageid数量和第一个pageid 
	  //   cout<<post_index.find(search_strset[it])->second.page_id;
	   //  cout<<endl;
     }
      int flag_get_page=0;
      for(int cnt=0;cnt<firkey_count;cnt++,fir_iter++)//遍历第一个词元的pageid
      {  
	     pageid=(*fir_iter).second.page_id;//得到pageid,再跟其他词元的比较
	     //cout<<"get pageid:"<<pageid<<endl;
	     for(size_t search_num=1;search_num!=search_strset.size();search_num++)//
   	     { 
		     if(pos_num[search_num]>count[search_num])//某个词元pageid已经遍历完
		                    return return_value;
		   // cout<<"\n---"<<(*ptrset[search_num]).first<<"=>"<<(*ptrset[search_num]).second.page_id<<" ";
		         while((*ptrset[search_num]).second.page_id<pageid&&pos_num[search_num]<count[search_num])//小于时前进
		        {
                             ptrset[search_num]++;//向前,待测试
			                 pos_num[search_num]++;
			               //  cout<<"所在词元的pageid小于第一个词元对应的pageid..继续前进..."<<endl;
 		     }
		     if((*ptrset[search_num]).second.page_id==pageid)//若pageid相等,取下一个词元的索引
		     {
				// cout<<"has equ"<<endl;
				      if(pos_num[search_num]<count[search_num])
		            	        ptrset[search_num]++;//本身往下走到一个pageid
                         pos_num[search_num]++;//记录该词元已经遍历的pageid数量
			            if(search_num==search_strset.size()-1)
				              flag_get_page=1;//可以提取,所有词元在同一个网页
				        //search_num++;
		          continue;
 		     }        
			     int pageid_tmp=(*ptrset[search_num]).second.page_id;
			     int signal_cnt=0;
                   while(pageid_tmp>pageid&&cnt<firkey_count-1)//所在词元的pageid大于第一个词元对应的pageid,让第一个词元的pageid-指针往前走
			     { 
			  	                 fir_iter++;
                                 pageid=(*fir_iter).second.page_id;
				                 cnt++;
				                 signal_cnt=1;
 			     }
			     if(signal_cnt==1)//回滚一下,避免越界
			     {
			         cnt--;
			         fir_iter--;
 				  }
			     break;
 		   } 
	     if(flag_get_page==1)
 	     {
			 return_value=1;
		     cout<<"---找到了用户输入序列的所有词元均在同一个page中情况,现在要保存他们的位置等信息了,以看他们位置是否相邻"<<endl;
		     //TODO:把词元倒排列表位置和pageid的信息存起来,或者只保存他们的迭代器,即指针,vector<vector<iterator>>
	             vector<multimap<string,postings_list>::iterator> pageid_ptr;
	             multimap<string,postings_list>::iterator tp;
	             pageid_ptr.push_back(fir_iter);//把第一个词元的和其他词元均在同一个page的倒排列表位置保存起来
                     for(size_t i=1;i!=ptrset.size();i++)
		            {
				        tp=ptrset[i];
			    		tp--;      //这里--是因为现在其他词元的z指针已经指向下一个了
                        pageid_ptr.push_back(tp);//保存其他词元(同在一个page下)对应的倒排列表位置指针
 		            }
		     pageid_ptrset.push_back(pageid_ptr);//保存的是那些,词元都在同一个页面的指针,
		     flag_get_page=0;
		     
		     for(size_t k=0;k!=pageid_ptr.size();k++)
			     cout<<(*pageid_ptr[k]).second.page_id<<"  ";
		//输出在同一个page下的所有词元和他们在该page下的第一个出现位置
		     cout<<"in the page :"<<pageid<<endl; //输出该pageid
		   //  cout<<"他们在该页面的第一个位置是?"<<endl;
		     for(size_t j=0;j!=pageid_ptrset[num_pageid_right].size();j++)
			     cout<<(*pageid_ptrset[num_pageid_right][j]).first<<"->"<<(*pageid_ptrset[num_pageid_right][j]).second.pos[0]<<"  ";
              num_pageid_right++;
           }
      }
     return return_value;
}  
/*返回在同一个文档中位置相邻的指针集合,结构同pageid_ptrset,算法与search()相似,注意按照原始字符串的词元顺序
 * 函数传入  原始序列在排序序列中的索引下标和第n个包含所有词元的页面对应的指针结构,函数返回位置+2*/
int search_pos_near(vector<int> pos_rest,vector<multimap<string,postings_list>::iterator> pageid_ptr)
{     
    //	vector<vector<multimap<string,postings_list>::iterator> >pageid_ptrset
    vector<vector<int> > posset;  //存放位置集合,第一个词元的位置集合,第二个....按照原始序列
	vector<size_t> posptr;  
	vector<int> tmp_pos;
	int p_num;
	int position0;
	for(size_t i=0;i!=pageid_ptr.size();i++)//顺序取出原始序列
	{
		p_num=pos_rest[i];
	//	cout<<"*"<<p_num<<" ";
	          tmp_pos=(*pageid_ptr[p_num]).second.pos;
              posset.push_back(tmp_pos);
              posptr.push_back(0);
        }
        tmp_pos=posset[0];//取首个数据结构
       // cout<<"first:"<<tmp_pos[0];
	int flag=0;//是否可以提取的标志即位置均相邻
	for(size_t j=0;j!=tmp_pos.size();++j)//遍历首个词元的某个pageid的所有位置
	{
              position0=tmp_pos[j];
             for(size_t k=1;k!=posset.size();k++)
	     {
		     if(posptr[k]>posset[k].size()||flag==1)//某个pageid的位置已经结束了,终止并返回
		     {
			   //  cout<<"\nover end pos"<<endl;
			     if(flag==1)
			        return flag;
			     else return position0+2;
	 	     }
      
		 while(posset[k][posptr[k]]<position0+1&&posptr[k]<posset[k].size()-1) //若该词元此位置比上一个词元靠前较多,则看此词元的下一个位置
		 {
			 posptr[k]++;
		 }
		     if(posset[k][posptr[k]]==position0+1)//若是位置为+1,相邻,继续看下一个
		    {
			 cout<<"\n词元"<<k<<"和词元"<<k-1<<"相邻:位置:: "<<position0<<endl;  
			   if(posptr[k]<posset[k].size())
			       position0++;
			 posptr[k]++;
             if(k==posset.size()-1)//可以提取
				 flag=1;
			 continue;
		 }
		 int signal_j=0;
              while(posset[k][posptr[k]]-k>position0&&j<tmp_pos.size()-1)//若该词元此位置比上一个词元靠后很多,则让第一个词元位置设置往后
		 {
                        j++;
                        position0=tmp_pos[j];
                        signal_j=1;
		 }
		 if(signal_j==1) //防止越界
		          j--;
                  break;
	      } 
	     if(flag==1)
	     {
			 //cout<<"该页面下所有词元都相邻\n"<<endl;
		     //TODO:
		     return flag;//页面中有搜索的字符串,即全相邻,该页面可以不用再找了,直接肯定该页面
	     }

	 }
       return position0+2;//该页面中的词元不相邻
}

/*由于之前的排序是根据词元对应倒排列表中pageid个数来排序的,不同于原始用户输入的字符串中词元的顺序,而在看词元是否相邻时是基于原始序列的,
 * 所以此时要取得原始序列词元是排序好的词元的第几个*/
 /*函数传入参数原始序列,排好序序列,结果序列,即取得结果*/
int  get_posindex(vector<string> src_raw,vector<string> src_sort, vector<int> &res)//原始字符串的词元第1个是排好序的字符串序列的第几个词元,..
{
     for(size_t i=0;i!=src_raw.size();++i)
     {
	     for(size_t j=0;j!=src_sort.size();++j)
	     {
		     if(src_raw[i].compare(src_sort[j])==0)//若相等
			     res.push_back(j);
	     }
     }
     return 0;
}




/*取得情况3的情况的词元在页面中的位置*/
 void get_pos_about()
	  {
	   vector<int> page_set_about(pageid_set_justabout.begin(),pageid_set_justabout.end());
	   //得出词元在页面中的位置
	   vector<multimap<string,postings_list>::iterator> itset;//保存每个关键词到达哪个页面索引
	   vector<int> itsetcount; 
	   vector<int> pos_p;
	    for(int i=0;i!=pagecount_exist_map.size();i++)
        {
		itsetcount.push_back(post_index.count(search_str_in[i]));
		itset.push_back(post_index.find(search_str_in[i]));//得到迭代器的位置
		pos_p.push_back(0);
		cout<<(*itset[i]).first<<" "<<endl;
	    }
	   for(int t=0;t<page_set_about.size();t++)//遍历页面
	   {
		   int signal_2=0;
		  pos_p.clear();
		   for(int j=0;j<pagecount_exist_map.size();j++) //遍历词元
		   {
                          while((*itset[j]).second.page_id<page_set_about[t]&&pos_p[j]<itsetcount[j]-1)
                          {
							  itset[j]++;
							  pos_p[j]++;
						  }
			             if((*itset[j]).second.page_id==page_set_about[t])
		              {
                             pos_about.push_back((*itset[j]).second.pos[0]);
                             signal_2=1;
			                 break;
			      }
		    }
		    if(signal_2==0)pos_about.push_back(5);
		}
		cout<<"position:"<<endl;
	   for(int p=0;p!=pos_about.size();p++)
	   cout<<pos_about[p]<<" ";
	   cout<<endl;
   }
     //去除召回的pageid_set_justabout中的相邻和包含所有词元的pageid
void filter_set_justabout()
    {
       int near_index=0,include_index=0;
         cout<<"原始的页面集合：　";
       for(set<int>::iterator it = pageid_set_justabout.begin(); it != pageid_set_justabout.end(); it++)cout<<*it<<" ";
       for(set<int>::iterator it = pageid_set_justabout.begin(); it != pageid_set_justabout.end(); )
      {
		      if(near_index==pageid_posright.size())break;
	          if(pageid_posright[near_index]==*it)
	          {
                 pageid_set_justabout.erase(it);
                 near_index++;
			 }
             else if(pageid_posright[near_index]>*it)
                 ++it;
             else if(pageid_posright[near_index]<*it)
                 near_index++;
	  }
	  cout<<"去除掉相邻的那些页面后的结果：　";
	 for(set<int>::iterator it = pageid_set_justabout.begin(); it != pageid_set_justabout.end(); it++)cout<<*it<<" ";
	     for(set<int>::iterator iti = pageid_set_justabout.begin(); iti != pageid_set_justabout.end(); )
      {
		      if(include_index==pageid_set_notnear.size())break;
	          if(pageid_set_notnear[include_index]==*iti)
	          {
                 pageid_set_justabout.erase(iti);
                 include_index++;
			 }
             else if(pageid_set_notnear[include_index]>*iti)
                 ++iti;
             else if(pageid_set_notnear[include_index]<*iti)
                 include_index++;
	  }
	  cout<<"去除掉包含的那些页面后的结果：　";
	 for(set<int>::iterator it = pageid_set_justabout.begin(); it != pageid_set_justabout.end(); it++)cout<<*it<<" ";
	 //for(set<int>::iterator it = pageid_set_justabout.begin(); it != pageid_set_justabout.end();it++)
	   //        pos_justabout.push_back(post_index)
   }

 
 //检索管理函数
/*检索主过程,得到页面id(所有词元存在该页面且相邻*/
int search_all(DB *dbp,DB *dbp1,string getfromuser)//不要总是把问题甩给程序,考虑出错的原因可能是输入输出的原因:比如没有合适的输入???
{
//	vector<string> result_split;//分隔的结果
	vector<string> result_sort; //排序好的结果
	int signal_hasright_pageid=0;
	split_searchstr(getfromuser.c_str(),getfromuser.size());//根据n-gram 分割用户输入的序列
//	cout<<"分割的得到的长度和结果"<<endl;
	cout<<search_str.size()<<endl;
	/*--------处理单个词元-------*/
	if(search_str.size()==1)//如果是单个词元
	{
	   if(search_str[0].size()==3&&search_str[0][1]<0)//单个汉字时
	   {
	         if(read_db(search_str[0],dbp1)<0)
		   {
			   cout<<"在数据库中没找到该序列中的词元:"<<search_str[0]<<endl;
	           return -1;
		   }
		}  
	   else if(read_db(search_str[0],dbp)<0)//读取某个key的数据填充到map中
        {
			cout<<"在数据库中没找到该序列中的词元:"<<search_str[0]<<endl;
			return -1;
		}
		//保存pageid和位置
       for(multimap<string,postings_list>::iterator it=post_index.begin();it!=post_index.end();++it)
       {
	    cout<<(*it).first<<"=>"<<(*it).second.page_id<<"\n";
	     pageid_posright.push_back((*it).second.page_id);
	     for(int i=0;i!=(*it).second.pos.size();i++)
		       posright.push_back((*it).second.pos[i]);
       }
       return 7;
   }
	
	
  /*----多个词元的情况------分成几种情况来讨论*/		
	
	//for(vector<string>::iterator it=result_sort.begin();it!=result_sort.end();it++)
	//      cout<<(*it)<<endl;
	//for(size_t i=0;i<result_split.size();i++)
	 //    cout<<i<<" => "<<result_split[i]<<endl;
	     
        //得到用户的输入-----------------notice----------
	//从数据库取得数据填充到map中
	   int has_result=0,result_1=0,result_2=0,result_3=0;//结果的标示，结果１为 在数据库中没找到该序列中的某个词元　　2:找到页面包含全部词元,3:正确的
	//---------------第1种情况：
	//-第1.１种情况：不完全匹配，即返回的页面只包含某些词
        cout<<"读取数据库倒排列表到内存中........."<<endl;
       for(size_t i=0;i<search_str.size();i++)
       {
        	if(read_db(search_str[i],dbp)<0)//读取某个key的数据填充到map中
	        {
			cout<<"在数据库中没找到该序列中的词元:"<<search_str[0]<<endl;     
                     result_1=1;
		    }
		    else
		      has_result=1;
	   }
	   pageid_set_justabout.insert(pageid_set_about.begin(),pageid_set_about.end());//存入不重复的相关pageid,供召回
	   if(result_1==1&&has_result==1)
	   {
		   get_pos_about(); 
		   return -2;  //直接返回，即只能找到某些词元
	    }
	   if(has_result==0)return -1;  //不可能有结果的情况，最糟糕，一个词元都没有
	   //情况1.1结束
	   
	   //测试代码
       // cout<<"从数据库中取得的multimap的结果 ,只包含第一个位置"<<search_str[0]<<search_str[1]<<endl;
	//	for(multimap<string,postings_list>::iterator it=post_index.begin();it!=post_index.end();++it)
		//      cout<<(*it).first<<"=>"<<(*it).second.page_id<<" "<<(*it).second.pos[0]<<endl;
		
	//-第1.2种情况：
	//排序
	   result_sort=search_str;
	  sort_by_count(result_sort,0,result_sort.size()-1);
	  cout<<"开始排序........排序的结果:    "<<endl;
//	for(size_t i=0;i<result_sort.size();i++)
	//     cout<<i<<" => "<<result_sort[i]<<endl;
	     
	 cout<<"开始进行查找词元是否有都在同一个页面....."<<endl;
     signal_hasright_pageid=search(result_sort);//查找在同一个文档的
        if(signal_hasright_pageid==0)
        {
			cout<<"找不到页面包含全部词元..."<<endl;
	        get_pos_about();
			return -2;    
		}
		
	//-------情况1结束
	
	//-------情况2:
	//查找相邻位置的文档
	vector<int> pos_res; //作为中间变量传入下函数
       get_posindex(search_str,result_sort,pos_res);
       int result_pos;
      
       /*for(size_t y=0;y!=pos_res.size();y++)
            cout<<"p--"<<pos_res[y];*/
       //对于所有可用页面id,遍历看词元位置是否相邻
       cout<<"找到了对的页面,开始看这些词元是否相邻....."<<endl;
       int sig=0;
       for(size_t x=0;x!=pageid_ptrset.size();++x)
       {
	      result_pos=search_pos_near(pos_res,pageid_ptrset[x]);
          if(result_pos==1)
          {
		      pageid_posright.push_back((*pageid_ptrset[x][0]).second.page_id);
		      posright.push_back(result_pos);
              sig=1;
		  }
		  else if(result_pos>=2)
		  {
		     pageid_set_notnear.push_back((*pageid_ptrset[x][0]).second.page_id);//否则存入不相邻但是包含所有词元的页面集合，供召回
		     pos_allhava.push_back(result_pos);
		 }
		     
       }
       if(sig==0)
       {
		   cout<<"虽然找到了包含所有词元的页面,但是不是所有词元都相邻"<<endl;
		   result_2=1;
		   filter_set_justabout();
		    get_pos_about();
		   return -3;
	   }
	   //情况2结束
	   //情况3:
	    cout<<"\n找到页面:"<<"它包含所有词元且他们相邻";
       for(size_t y=0;y!=pageid_posright.size();y++)
            cout<<"\n找到: "<<pageid_posright[y]<<endl;
       filter_set_justabout();     
        get_pos_about();
     

	 
       return 0;
 }	    
    
    

//排序相关函数    
//把各个得到的结果页的度计算出来并保存；（度：页中包含的关键词的词频和idf的关系式子）
int get_tfidf(vector<double> &tfidfset,vector<int> &pageid_to_sort)
{
	vector<multimap<string,postings_list>::iterator> itset;//保存每个关键词到达哪个页面索引
	int numofword=pagecount_exist_map.size();
	int numofpage=pageid_to_sort.size();
	cout<<numofword<<"  "<<"----"<<numofpage<<" "<<endl;
	//exit(0);
	vector<int> itset_count;
    for(int i=0;i!=numofword;i++)
    {
		//cout<<"da"<<search_str[i]<<endl;
		itset.push_back(post_index.find(search_str_in[i]));//得到迭代器的位置
		cout<<(*itset[i]).first<<" "<<endl;
		itset_count.push_back(post_index.count(search_str_in[i]));
		cout<<"herwee"<<endl;
	}
	//exit(0);
	double tfidfs;
	//vector<double> tfidfset;
	int num_it=0;
	for(int t=0;t!=numofpage;t++)
	{
		num_it=0;
	   for(int j=0;j!=numofword;j++)//遍历词元
	   {
		  while((*itset[j]).second.page_id<pageid_to_sort[t]&&(num_it<itset_count[j]))
		  {
			  num_it++;
			  itset[j]++;
		  }
		  if((*itset[j]).second.page_id==pageid_to_sort[t])
		  {
			 double idf=page_all*1.0/(pagecount_exist_map[search_str_in[j]]*1.0);
			 double idfs=log2(idf);
			 tfidfs=tfidfs+(*itset[j]).second.count_of_page*1.0*idfs;
		  }
	  }
	  tfidfset.push_back(tfidfs);
  }
   return 0;
}  
int tf_sort_pageid(vector<int> &pageid_set_to_sort)//对结果的pageid进行排序
{
	vector<double> tfidfset;
	int numpage=pageid_set_to_sort.size();
	if(numpage==0) return 0;
	get_tfidf(tfidfset,pageid_set_to_sort);//将根据这个结果进行排序
	cout<<"hereew"<<endl;
	
	for(int i=0;i<numpage;i++)
	{
		for(int j=i;j<numpage;j++)
		{
			if(tfidfset[i]<tfidfset[j])
			{
				int temp=pageid_set_to_sort[i];
				pageid_set_to_sort[i]=pageid_set_to_sort[j];
				pageid_set_to_sort[j]=temp;
				double tmp=tfidfset[i];
				tfidfset[i]=tfidfset[j];
				tfidfset[j]=tmp;
			}
		}
	}
	for(int i=0;i<numpage;i++)cout<<pageid_set_to_sort[i]<<":=>"<<tfidfset[i]<<endl;
	return 0;
}



//int main()
//{
	////传入数据库名并打开数据库
	//DB *dbp;
	//char *dbname="index_builded.db";
	//opendb(&dbp,dbname,0);
	////由输入的文本分隔

	////TODO:
	
	//char *tmp="关于菜鸟教程用户中心内测说明";
        //search_all(dbp,tmp);
           ////if(dbp!=NULL)
                                ////dbp->close(dbp,0);
           ////char *dbname2="pageida2.db";
			                ////opendb(&dbp,dbname2,1);
			                ////DBC *cursorp; 
			                ////cout<<"result:"<<dbp->cursor(dbp,NULL,&cursorp,0)<<endl;
			                
	                        ////read_result_db(&cursorp);


	//return 0;
  //}


