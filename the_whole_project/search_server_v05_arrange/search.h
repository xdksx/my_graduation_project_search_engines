#include<iostream>
#include<db.h>
#include<vector>
#include<string>
#include<map>
#include<string.h>
#define page_id_size 10    //设置pageid 的长度，即在数据库中保存的长度，为了统一，容纳最多１０位的pageid编号，例子：＇１２３　　　　　　　　　　＇
#define pos_size 6//设置词元在文档中的位置：如文档开头＇谷歌是个好的搜索．．．＇　词元位置谷歌为０　歌是为１．．．，每个占６字节
#define page_all 100
#define read_db_buffer 400000   //读取数据库中某次元倒排索引缓冲区大小,用于read_db函数
#define read_result_db_buffer 60000000    //读取结果数据库中某次元倒排索引缓冲区大小,用于read_result_db函数
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<set>
using namespace std;

enum n_gram_SIG  
{ 
	 INIT=0,
	 EN=1,
	 CH=2
}; 
class postings_list //倒排索引全局数据结构
{
   public:
      int page_id;  
      int count_of_page;   //TF 词频
      vector<int> pos;
   
    void clear()
    {
	  pos.clear();
      }
};
 
extern vector<string> search_str_in;
extern vector<string> search_str; //存放搜索的词元集合
extern map<string,int> pagecount_exist_map;   //存放某次元在多少个文档中出现，用于求idf
extern multimap<string,postings_list> post_index;//存放待检索的词元所有倒排数据，即内存map

//搜索词元在某个文档都相邻时即完全匹配搜索序列的情况使用的数据结构  情况1
extern vector<vector<multimap<string,postings_list>::iterator> > pageid_ptrset;//存放在同一个文档都出现但是位置不知道是否相邻的post_index数据指针集合的集合
     //第一维是page数量,即第一个词元都在的页面,第二个..,第二维是对应page的词元的倒排列表指针集合,即第一个词元倒排列表所在位置.第二个...,以供后面的判断位置是否相邻
extern vector<int> pageid_posright;     //存放所有（用户输入的查询字符串提取出来的）词元在数据库中相邻（即准确获取句子）的pageid
extern vector<int> posright;  //存放上面pageid对应的词元在文档中的位置

//搜索词元均包含在page中却不相邻   情况2
extern vector<int> pageid_set_notnear;  //保存那些包含所有词元但是不相邻的页面（供召回）
extern vector<int> pos_allhava;  //存放那些包含所有词元但是不相邻的页面的位置，供返回结果摘要用

//某几个搜索词元出现在某些文档中   情况3
extern set<int> pageid_set_justabout;//保存那些出现过某个词元的页面（供召回）
extern vector<int> pageid_set_about;  //供上面的set提供来源（临时保存，set去重）
extern vector<int> pos_about;

//返回的结果数据结构
extern vector<vector<string> > result_str; //存放从第二个数据库中返回的结果字符串

void clear_global_ds(); //清除以上全局数据结构，避免下一次请求查询混淆上一次的

void printchars(char *str,int len);/*输出读取的数据库记录,供测试----*/

/*简单处理url的函数,即把url:中的中文即:像"%E3%E4...ida...%E2.."这种中英文混合的方式,分割出来中文加英文如:"我喜欢google搜索"*/
int dealurl(string src,string &des);

/*-----------------------数据库操作相关函数---------------------*/

/*打开数据库,根据标志判断是否打开一键多值的数据库,0则不为一键多值,1则是*/
int opendb(DB **dbp,char *db_name,int flag_dup);
 /*读取数据库记录,k为词元,读取其对应的value,存入相关信息到------*/
int read_db(string k,DB *db);
/*在倒排列表中找到page之后,此函数用于在另一个数据库中,根据建为pageid来查找对应的url,标题和部分文本,以反馈给用户链接和相关信息*/
int read_result_db(DBC *cursorp,vector<int> pageid_set_r,vector<int> pos_set_r);



/*-------------------------检索过程相关函数-----------------------------*/
//前期处理
int split_searchstr(const char* rawstr,int length);//N-gram分隔请求的字符串,传入用户输入的字符串和长度,引用方式返回得到的多个字符串



/*此部分为快速排序,以排序词元对应的pageid个数进行排序,以减少检索时的时间复杂度*/

int partion(vector<string> &strs,int lo,int hi);
void sort_by_count(vector<string> &strset,int lo,int hi);

//检索主要函数

//xapian
/*传入按照pageid个数排序好的词元数组，得到词元均在同一个文档的指针集结构*/
int search(vector<string> search_strset);

/*返回在同一个文档中位置相邻的指针集合,结构同pageid_ptrset,算法与search()相似,注意按照原始字符串的词元顺序
 * 函数传入  原始序列在排序序列中的索引下标和第n个包含所有词元的页面对应的指针结构*/
int search_pos_near(vector<int> pos_rest,vector<multimap<string,postings_list>::iterator> pageid_ptr);


/*由于之前的排序是根据词元对应倒排列表中pageid个数来排序的,不同于原始用户输入的字符串中词元的顺序,而在看词元是否相邻时是基于原始序列的,
 * 所以此时要取得原始序列词元是排序好的词元的第几个*/
 /*函数传入参数原始序列,排好序序列,结果序列,即取得结果*/
int  get_posindex(vector<string> src_raw,vector<string> src_sort, vector<int> &res);//原始字符串的词元第1个是排好序的字符串序列的第几个词元,..




/*取得情况3的情况的词元在页面中的位置*/
 void get_pos_about();
 //去除召回的pageid_set_justabout中的相邻和包含所有词元的pageid
void filter_set_justabout();
 
 //检索管理函数
/*检索主过程,得到页面id(所有词元存在该页面且相邻*/
int search_all(DB *dbp,DB *dbp1,string getfromuser);//不要总是把问题甩给程序,考虑出错的原因可能是输入输出的原因:比如没有合适的输入???


//排序相关函数
//把各个得到的结果页的度计算出来并保存；（度：页中包含的关键词的词频和idf的关系式子）
int get_tfidf(vector<double> &tfidfset,vector<int> &pageid_to_sort);
int tf_sort_pageid(vector<int> &pageid_set_to_sort);//对结果的pageid进行排序





