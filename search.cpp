#include<iostream>
#include<db.h>
#include<vector>
#include<string>
#include<map>
#include<string.h>
#include<stdlib.h>

using namespace std;
enum n_gram_SIG   
{ 
	 INIT=0,
	 EN=1,
	 CH=2
};
class   postings_list
{
   public:
      int page_id;
      vector<int> pos;
      string url;
    void clear()
    {
	  url="";
	  pos.clear();
      }
};
 ostream& operator<<(ostream& out,const postings_list& s)
	{
		out<<s.page_id<<"\t";
		for(size_t i=0;i!=s.pos.size();i++)
		{
			out<<s.pos[i]<<"\t";
	        }
		out<<s.url<<"\t";
		return out;
	}
multimap<string,postings_list> post_index;
vector<vector<multimap<string,postings_list>::iterator> > pageid_ptrset;//存放在同一个文档都出现但是位置不知道是否相邻的数据指针
int split_searchstr(char* rawstr,int length,vector<string> &search_str)//N-gram分隔请求的字符串
{
      string instr("");
     char *p1=rawstr;//set two point to get 2-gram word
     int len_bu=length;
     int flag_left=0;
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
				//调用填充到索引(构建索引的函数中)
			         //test
				//cout<<"here:"<<instr<<endl;
                                search_str.push_back(instr);
             
				state=CH;
				instr="";
				instr.append(1,*p1);
				count_ch=1;
			}
			else if(*p1==' '||*p1=='\n')
			{
				//调用函数填充到索引
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
				//调用函数填充到索引
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
				//防止最后一个字节遗漏
				flag_left=1;
			}
			else if(count_ch==2&&*p1>=-32&&*p1<0)//开始第三个汉字   //这里的实现方式不太好,可扩展性不强,可以考虑在读了n-gram中的n个中文后转入init状态,遇到下一个中文开头再转入CN状态.这里得固定中文的utf为3字节
			{
                            //调用函数填充索引
                                search_str.push_back(instr);
                                flag_left=0;
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
     if(flag_left==1)search_str.push_back(instr);
    return 0;
}
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

void printchars(char *str,int len)
{
	int n=0;
	while(n<len)
	{
		n++;
	  printf(" %c",*str);
	   str++;
   }
   cout<<endl;
}
int read_db(string k,DB *db)
{
	DBT key,data;
	memset(&key,0,sizeof(DBT));
	memset(&data,0,sizeof(DBT));
	char *words=(char *)malloc(k.size());
	memset(words,0,k.size());
	memcpy(words,k.c_str(),k.size());
	char *buffer=(char *)malloc(40000);
	char *buffer_free=buffer;
	memset(buffer,0,40000);

	key.data=words;
	key.size=k.size();

	data.data=buffer;
	data.ulen=40000;
	data.flags=DB_DBT_USERMEM;
	if((db->get(db,NULL,&key,&data,0)==DB_NOTFOUND))
	{
		cout<<"notfound\n"<<endl;
		return -1;
	}
	else 
	{
  // printchars(buffer,data.size);
		int num=0;
	        postings_list tmp;
	        string str((char *)(key.data));
	        while(num<data.size)
		   {
			tmp.page_id=atoi(buffer);
//			 	printf("pageid:%s\n",buffer); 
			buffer=buffer+5;
			num=num+5;
		
			while((*buffer>='0'&&*buffer<='9')||*buffer=='\0')
			{
				tmp.pos.push_back(atoi(buffer));
				//printf("pos:%s",buffer); 
				buffer=buffer+5;
				num=num+5;
			}
			while(*buffer!='*')
			{
				tmp.url.append(1,*buffer);
				//printf("url%s",buffer); 
				buffer++;
				num++;
			}
			buffer++;
			num++;
			//cout<<"\nddd"<<*buffer<<endl;
                     post_index.insert(pair<string,postings_list>(str,tmp));
	                 tmp.clear();
		}
	}
	free(buffer_free);
	return 0;
}


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
     int is=0;
     vector<int> pos_num;//游标,记录当前词元第几个pageid,防止越界及及时终止
     for(size_t it=0;it!=search_strset.size();it++)
     {
	     pos_num.push_back(0);
	     count.push_back(post_index.count(search_strset[it]));
	     ptrset.push_back(post_index.find(search_strset[it]));
         cout<<" "<<post_index.count(search_strset[it])<<" ";  
	     cout<<post_index.find(search_strset[it])->second.page_id;
	     cout<<endl;
     }
      int flag_get_page=0;
      for(int cnt=0;cnt!=firkey_count;cnt++,fir_iter++)//遍历第一个词元的pageid
      {  
	     pageid=(*fir_iter).second.page_id;//得到pageid,再跟其他词元的比较
	     for(size_t search_num=1;search_num!=search_strset.size();search_num++)//
   	     { 
		     if(pos_num[search_num]==count[search_num])//某个词元pageid已经遍历完
		                    return 0;
		    //cout<<"\n---"<<(*ptrset[search_num]).first<<"=>"<<(*ptrset[search_num]).second.page_id<<" ";
		     if((*ptrset[search_num]).second.page_id==pageid)//若pageid相等,取下一个词元的索引
		     {
		            	 ptrset[search_num]++;
                         pos_num[search_num]++;
			            if(search_num==search_strset.size()-1)
				              flag_get_page=1;//可以提取,所有词元在同一个网页
				        //search_num++;
		        continue;
 		     }
		     while((*ptrset[search_num]).second.page_id<pageid&&pos_num[search_num]<count[search_num]-1)//小于时前进
		     {
                             ptrset[search_num]++;//向前,待测试
			                 pos_num[search_num]++;
 		     }
                if((*ptrset[search_num]).second.page_id==pageid)
		     {
			     pos_num[search_num]++;
			     ptrset[search_num]++;
			     if(search_num==search_strset.size()-1)
				       flag_get_page=1;//可以提取,所有词元在同一个网页
			     continue;
 		     }
                     
			     int pageid_tmp=(*ptrset[search_num]).second.page_id;
                   while(pageid_tmp>pageid&&cnt<firkey_count-1)
			     { 
			  	                 fir_iter++;
                                 pageid=(*fir_iter).second.page_id;
				                 cnt++;
			     }
			     if(cnt<firkey_count-1||pageid_tmp<=pageid)
			     {
			         cnt--;
			         fir_iter--;
				  }
			     break;
		   } 
	     if(flag_get_page==1)
	     {
		     cout<<"-----------"<<endl;
		     //TODO:把词元倒排列表位置和pageid的信息存起来,或者只保存他们的迭代器,即指针,vector<vector<iterator>>
	             vector<multimap<string,postings_list>::iterator> pageid_ptr;
	             multimap<string,postings_list>::iterator tp;
	             pageid_ptr.push_back(fir_iter);
                     for(size_t i=1;i!=ptrset.size();i++)
		     {
				        tp=ptrset[i];
			    		tp--;      
                        pageid_ptr.push_back(tp);
		     }
		     pageid_ptrset.push_back(pageid_ptr);//保存的是那些词元都在该页面的指针,
		     flag_get_page=0;
		     
		 //    for(size_t k=0;k!=pageid_ptr.size();k++)
		//	     cout<<(*pageid_ptr[k]).second.page_id<<"  ";
		     for(size_t j=0;j!=pageid_ptrset[0].size();j++)
			     cout<<(*pageid_ptrset[0][j]).first<<"->"<<(*pageid_ptrset[0][j]).second<<"  ";
            }
     }
     return 0;
} 
/*
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
     int is=0;
     vector<int> pos_num;//游标,记录当前词元第几个pageid,防止越界及及时终止
     for(size_t it=1;it!=search_strset.size();it++)
     {
	     pos_num.push_back(0);
	     count.push_back(post_index.count(search_strset[it]));
	     ptrset.push_back(post_index.find(search_strset[it]));
             cout<<" "<<post_index.count(search_strset[it])<<" ";  
	     cout<<post_index.find(search_strset[it])->second.page_id;
	     cout<<endl;
     }
         
      int flag=0;
      int flag_get_page=0;
      int end_by_count=0;
     for(int cnt=0;cnt!=firkey_count;cnt++,fir_iter++)//遍历第一个词元的pageid
      {  
	     pageid=(*fir_iter).second.page_id;//得到pageid,再跟其他词元的比较
	    // cout<<"\n"<<pageid<<endl;
	     is=0;
	     for(size_t search_num=1;search_num!=search_strset.size();search_num++)//
	     { 
		     
		    // cout<<"\n---"<<(*ptrset[is]).first<<"=>"<<(*ptrset[is]).second.page_id<<" ";
		     if((*ptrset[is]).second.page_id==pageid)//若pageid相等,取下一个词元的索引
		     {
			 ptrset[is]++;
                         pos_num[is]++;
			 if(pos_num[is]==count[is])
				 end_by_count=1;
			 is++;
			 
			 if(search_num==search_strset.size()-1)
				 flag_get_page=1;//可以提取,所有词元在同一个网页
		        continue;
 		     }
		     while((*ptrset[is]).second.page_id<pageid&&pos_num[is]<count[is]-1)//小于时前进
		     {
                             
                             ptrset[is]++;//向前,待测试
			     pos_num[is]++;
 		     } 
                     if((*ptrset[is]).second.page_id==pageid)
		     { 
			     pos_num[is]++;
			     if(pos_num[is]==count[is])
			     {
                                   end_by_count=1;
                              }
		//	     if(pos_num[is]<count[is])
			                   ptrset[is]++;
			     is++;
			 if(search_num==search_strset.size()-1)
				 flag_get_page=1;//可以提取,所有词元在同一个网页
			     continue;
 		     }
                     else if((*ptrset[is]).second.page_id>pageid)
		     {
			     int pageid_tmp=(*ptrset[is]).second.page_id;
                             while(pageid_tmp>pageid&&cnt<firkey_count)
			     { 
				 fir_iter++;
                                 pageid=(*fir_iter).second.page_id;
				 cnt++;
		 	     }
			     flag=1;
  		     } 
		     if(flag==1)//
		     {
			     cnt--;
			     fir_iter--;
			     flag=0;
			     break;
		      } 
 	     }
	     if(flag_get_page==1)
	     {
		     cout<<"-----------"<<endl;
		     //TODO:把词元倒排列表位置和pageid的信息存起来,或者只保存他们的迭代器,即指针,vector<vector<iterator>>
	             vector<multimap<string,postings_list>::iterator> pageid_ptr;
	             multimap<string,postings_list>::iterator tp;
                     for(size_t i=0;i!=ptrset.size();i++)
		     {
				        tp=ptrset[i];
					tp--;      
                        pageid_ptr.push_back(tp);
		     }
		     pageid_ptrset.push_back(pageid_ptr);//保存的是那些词元都在该页面的指针,
		     flag_get_page=0;
		     
		 //    for(size_t k=0;k!=pageid_ptr.size();k++)
		//	     cout<<(*pageid_ptr[k]).second.page_id<<"  ";
		     for(size_t j=0;j!=pageid_ptrset[0].size();j++)
			     cout<<(*pageid_ptrset[0][j]).second.page_id<<"  ";
            }
	     if(end_by_count==1)break;
     }
     return 0;
} 
        */ 
int search_pos_near(vector<int> pos_rest,vector<multimap<string,postings_list>::iterator> pageid_ptr)//返回在同一个文档中位置相邻的指针集合,结构同pageid_ptrset,算法与search()相似,注意按照原始字符串的词元顺序
{
        
    //	vector<vector<multimap<string,postings_list>::iterator> >pageid_ptrset
        vector<vector<int> > posset;
	vector<size_t> posptr;
	vector<int> tmp_pos;
	int p_num;
	int position0;
	for(size_t i=0;i!=pageid_ptr.size();i++)
	{
		p_num=pos_rest[i];//顺序取出原始序列的
		cout<<"*"<<p_num<<" ";
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
		     if(posptr[k]==posset[k].size())//某个pageid的位置已经结束了,终止并返回
		     {
			     cout<<"\nds"<<endl;
			     return 0;
	 	     }
             if(posset[k][posptr[k]]==position0+1)//若是位置为+1,相邻
		    {
			 cout<<"\nyes"<<position0<<endl; 
             
			 position0++;
			 posptr[k]++;
             if(k==posset.size()-1)//可以提取
				 flag=1;
			 continue;
		 }
		 while(posset[k][posptr[k]]<position0+1&&posptr[k]<posset[k].size()-1)
		 {
			 posptr[k]++;
		 }
		 if(posset[k][posptr[k]]==position0+1)
		 {
			 position0++;
			 posptr[k]++;
                         if(k==posset.size()-1)//可以提取
				 flag=1;
			 continue;
		 }
              while(posset[k][posptr[k]]-k>position0&&j<tmp_pos.size()-1)
		 {
			            cout<<"enen:";
                        j++;
                        position0=tmp_pos[j];
		 }
		 if(j<tmp_pos.size()-1||posset[k][posptr[k]]<=position0+1)
		         j--;
                  break;
	      } 
	     if(flag==1)
	     {
			 cout<<"hah"<<endl;
		     flag=0;
		     //TODO:
		     return 1;//页面中有搜索的字符串,即全相邻,该页面可以不用再找了,直接肯定该页面
	     }

	 }
       return 0;//该页面中的词元不相邻
}


    int   get_posindex(vector<string> src_raw,vector<string> src_sort, vector<int> &res)//原始字符串的词元第1个是排好序的字符串序列的第几个词元,..
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

	int opendb(DB **dbp,char *db_name)
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
    	    ret=(*dbp)->open(*dbp,NULL,db_name,NULL,DB_BTREE,flags,0);
	if(ret!=0)
	{
	   cout<<"error open\n";
	   return -1;
	}
	   return 0;
	}
int main()
{
	//传入数据库名并打开数据库
	DB *dbp;
	char *dbname="search12.db";
	opendb(&dbp,dbname);
	//由输入的文本分隔

	//TODO:
	vector<string> result_split;//分隔的结果
	vector<string> result_sort;
	char *tmp="浮生若梦人世事";
	string getfromuser(tmp);
	split_searchstr(tmp,getfromuser.size(),result_split);
	cout<<result_split.size()<<endl;
	result_sort=result_split;
	for(vector<string>::iterator it=result_sort.begin();it!=result_sort.end();it++)
	      cout<<(*it)<<endl;
	for(size_t i=0;i<result_split.size();i++)
	     cout<<i<<" => "<<result_split[i]<<endl;
        //得到用户的输入-----------------notice----------
	//string key="xx";
	//从数据库取得数据填充到map中
  for(size_t i=0;i<result_split.size();i++)
	read_db(result_split[i],dbp);//读取某个key的数据填充到map中
	
		//print the result:
		for(multimap<string,postings_list>::iterator it=post_index.begin();it!=post_index.end();++it)
		      cout<<(*it).first<<"=>"<<(*it).second.page_id<<" "<<(*it).second.pos[0]<<" "<<(*it).second.url<<endl;
	//开始检索过程
	//排序
	sort_by_count(result_sort,0,result_sort.size()-1);
	for(size_t i=0;i<result_sort.size();i++)
	     cout<<i<<" => "<<result_sort[i]<<endl;
        search(result_sort);//查找在同一个文档的
	//查找相邻位置的文档
	vector<int> pos_res;
       get_posindex(result_split,result_sort,pos_res);
       int result_pos;
       vector<int> pageid_posright;
       for(size_t y=0;y!=pos_res.size();y++)
            cout<<"p--"<<pos_res[y];
       //对于所有可用页面id,遍历看词元位置是否相邻
       for(size_t x=0;x!=pageid_ptrset.size();++x)
       {
	      result_pos=search_pos_near(pos_res,pageid_ptrset[x]);
          if(result_pos==1)
          {
		      pageid_posright.push_back((*pageid_ptrset[x][0]).second.page_id);
              cout<<"\n++++++"<<endl;
		  }
       }
       for(size_t y=0;y!=pageid_posright.size();y++)
            cout<<"ppppppppp=+++++++:"<<pageid_posright[y];
		      
//int search_pos_near(vector<int> pos_rest,vector<multimap<string,postings_list>::iterator> pageid_ptr)//返回在同一个文档中位置相邻的指针集合,结构同pageid_ptrset,算法与search()相似,注意按照原始字符串的词元顺序
      return 0;
  }







