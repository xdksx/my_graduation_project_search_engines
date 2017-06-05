#include<iostream>
#include<map>
#include<string>
#include<vector>
#include<fstream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<db.h>
#include<ostream>
#include<dirent.h>
using namespace std;
#define page_id_size 10
#define pos_size 6
enum n_gram_SIG      //n-gram算法用,分隔实现中有限状态的状态
{
	 INIT=0,  
	 EN=1,
	 CH=2
};
class  postings_list     //倒排列表类    //--20170409修改,去掉url
{
   public:
	int page_id;
	vector<int> pos;
	int count_of_page;  //TF
       // friend ostream& operator<<(ostream& out,const postings_list& s);
};

extern map<string,int> pagecount_exist_map;
extern multimap<string,postings_list> post_index;//内存中的倒排索引
/*-------------------------此部分为处理网页的函数,得到只有中文和英文（不包括标点及标签的）的文本,deal_tag可以修改过滤的标签,deal为主要的状态机,主要算法-----------------------*/
/*
 * 函数说明:
 * deal(string filename,char *result)传入文件名,返回结果字符串
 */
enum Tag_State//提取网页文本的状态机状态枚举
{
	TAG_LEFT=0, //<
	TAG_LEFTA=1,   //</
	TAG_LEFTS=2, //<x
	TAG_RIGHT_S=3   //<xx >/<xx>/</xx>初始
};
enum CH_symbol     //剔除网页中的中文标点等特殊符号的状态机的状态
{
	S0=0,//End or init
	S1=1, //E3
	S2=2,//86
	S3=3,//EF
	S4=4//AB
};
int deal_tag(string tag);//html剔除选项,可以扩展如只提取<p>标签

int deal(string filename,char *result);//传入文件名和结果字符串,把网页文件提取的文本提取到字符串中


/*----------------此部分为建立内存中的倒排索引的函数-------------------*/
/*函数说明:
 * 传入由n-gram算法得到的词元及词元的位置等信息填充到倒排索引数据结构中
 */
int build(string word,int page_id_i,int pos_i);//把词元和词元的位置等信息构成内存中的倒排索引

/*------函数说明
 * n-gram算法,同时存入内存倒排索引中
 * 传入提取的网页正文rawstr,长度,页面id
 */
int build_post_index_m(char* rawstr,int length,int page_id_i);//N-gram 分割和构建倒排索引,内存中
/*------函数说明
 * 1-gram算法,同时存入内存倒排索引中
 * 传入提取的网页正文rawstr,长度,页面id
 */
int build_post_index_o(char* rawstr,int length,int page_id_i);//1-gram 分割和构建倒排索引,内存中
/*----------------------此部分函数为将内存中的倒排索引保存到数据库中的相关函数----------*/
/*
 * 函数说明:
 * 传入word作为key,并以buffer返回结果
 */
int map_to_char(char *buffer,string word);//将索引(即key-对应的value)以字符串的形式存到char *buffer中

        	

int print_post_index();//输出内存构建的索引

void printchars(char *str,int len);//输出从数据库中得到的记录,测试用

/*此函数和上面的deal函数都是处理传入的文件，上面deal处理的是原始的网页文件，而这个函数的存在是因为做爬虫的时候已经对网页进行了提取正文，
 * 所以这个函数只需要剔除中文标点符号，只保留英文和中文即可．
 * 这里暂时只调用这个函数，可以根据具体情况调整
 */
int deal_hasget_text(string filename,char *result);//传入文件名和结果字符串,把网页文件提取的文本提取到字符串中

