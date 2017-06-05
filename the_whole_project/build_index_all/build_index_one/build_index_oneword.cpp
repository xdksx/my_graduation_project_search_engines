#include "../build_index.h"




int main(int argc,char *argv[])
{

//对数据库的操作
     DB *dbp;
       u_int32_t flags=DB_CREATE;
       int ret;
       ret=db_create(&dbp,NULL,0);
       if(ret!=0)
       {
	       cout<<"error create"<<endl;
              //other to do 
	}
       ret=dbp->open(dbp,NULL,"../../search_server_v05_arrange/index_builded_oneword.db",NULL,DB_BTREE,flags,0);
       if(ret!=0)
	       cout<<"error open"<<endl;
	
	/*------构建索引要传入网页对应的目录名字作为参数--------*/
	DIR *dirp;
	struct dirent *dp;
	dirp=opendir(argv[1]);//打开目录
	int pageid_i;
	string path(argv[1]);
	int num_filename=0;
	char filename[16];
	while((dp=readdir(dirp))!=NULL)//读取目录下的文件，注意按升序来读取，１，２，３，文件名即是pageid
	{
		if(dp->d_name[0]=='.')
		  continue;
		  memset(filename,0,15);
		sprintf(filename,"%d",num_filename);
	    string str(filename);
	    pageid_i=num_filename;
	    num_filename++;
	    str=path+"/"+str;
	    cout<<"reading file"<<str<<"........"<<endl;
	    char *d=NULL;
	    d=(char*)malloc(7000000);
	    char *fr_d=d;
	    int length;
	  //length=deal(str,d);
	  cout<<"dealing file"<<str<<"........"<<endl;
	    length=deal_hasget_text(str,d);
//	   cout<<"------提取网页的结果----"<<endl;
       // printf("%s\n",d);
	//    cout<<"--------------得到的建立在内存中的索引------------"<<endl;
       //这里要多次调用这个函数,然后再进行数据库记录的合并
       post_index.clear();
       build_post_index_o(d,length,pageid_i);
    //   print_post_index();  
        
        //把内存中的记录写入到数据库中
        int count_record=post_index.size();//存入记录总数
        for(multimap<string,postings_list>::iterator it=post_index.begin();it!=post_index.end();)//遍历整个内存索引
	   {
		   int word_count=post_index.count((*it).first);//获得某个词元对应的pageid数量(即多少个页面)
     //   cout<<(*it).first<<"word_count"<<word_count<<endl;
		//it=find((*it).first);
		//由此,取到第一个key
		//从数据库中取得该词元记录
		   char *buffer_get=NULL;
		 //  buffer_get=new char[1000000000];
		   buffer_get=(char *)malloc(8000000);//这里不知道分配多少空间,唉,这里还是设置成一个宏吧
		   char  *fr=buffer_get;
		   int buffer_size=8000000;
		   memset(buffer_get,0,8000000);
	    	int word_size=(*it).first.size();//取得词元的大小(作为key的大小)
	    	const int words_length=word_size+1;
	    	char wordss[words_length];
	    //	wordss[word_size]='\0';
	//   	char *wordss=NULL;
		 //   wordss=(char *)malloc(word_size+100);//key的填充要用char *//注意这里的+100调试了一两个小时错误的原因,
		 //因为malloc或者new要分配比需求大于(几十或者更大的)个字节的空间,否则会出错,即使很小,这可能是malloc机制,最后换成数组，无错
		   char *fr_word=wordss;
		memset(wordss,0,word_size);
		memcpy(wordss,(*it).first.c_str(),word_size);
		//cout<<"word----first"<<(*it).first<<endl;
        //printf("words:size: %d   %s\n",word_size,wordss);	
        //填充key和data
		DBT key,data;
		memset(&key,0,sizeof(DBT));
		memset(&data,0,sizeof(DBT));
		key.data=wordss;
		key.size=word_size;
                
        data.data=buffer_get;
		data.ulen=buffer_size;
		data.flags=DB_DBT_USERMEM;
         if((dbp->get(dbp,NULL,&key,&data,0)==DB_NOTFOUND))//取出词元对应的记录,没找到
		{
		     //把内存的该记录转换为char*
	//	   cout<<"notfound，so puting new record......"<<endl;
		   pagecount_exist_map[(*it).first]=1;//该词出现文档数为１
		     int all;
		     char *show=buffer_get;
		     all=map_to_char(buffer_get,(*it).first);//把词元对应的倒排列表转换为char *格式 并返回长度
		  //  printf("result map to char:%s\n",buffer_get); 
		     data.size=all;
		     dbp->put(dbp,NULL,&key,&data,DB_NOOVERWRITE);
		     //插入新的记录;
		 }
           else
		 {
			// printf("key:%s   ",(char*)(key.data));
		//	cout<<"the record is founded..so adding posting list...."<<endl;
			 pagecount_exist_map[(*it).first]++;//累计该词出现的文档数
		 //    printf("wordsize:%d\n",word_size);
		//     cout<<"key:"<<key.data<<endl;
                 //把内存中的记录转换为char *
            //  printchars(buffer_get,data.size);
		      int all;
		   // char *buffer_new=(char*)malloc(40000);
		      char *tm=buffer_get+data.size;
     
		    //  cout<<"data_size"<<data.size<<endl;
		      all=map_to_char(tm,(*it).first);
		      //再添加到buffer后面
		  //    printchars(tm,all+data.size);
		      
		     //以上包括
		      //删除记录并重新插入
		       dbp->del(dbp,NULL,&key,0);
		       data.data=buffer_get;
		       data.size=data.size+all;
		       dbp->put(dbp,NULL,&key, &data,DB_NOOVERWRITE);
		 }
	if(fr!=NULL)
	{ 
		//free(wordss);
	   free(buffer_get);
	  
    }
	   for(int t=0;t<word_count;t++)it++;
	}
     if(fr_d!=NULL) 	      
     	free(fr_d);
	}
	if(dbp!=NULL)
      dbp->close(dbp,0);
		 return 0;
}











