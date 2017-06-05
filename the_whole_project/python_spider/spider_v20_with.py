import deal
import urllib2
import re
import urlparse
import robotparser
import time
from datetime import datetime
from bsddb import db
from bs4 import BeautifulSoup
import sys
import ssl
import socket
import codecs
reload(sys)
sys.setdefaultencoding('utf-8')
import gzip
from StringIO import StringIO
#sys.setdefaultencoding('latin1')
type_file=sys.getfilesystemencoding()
class spider:
    """is a spider to scrapy html"""
	#filename_num=0
    crawl_queue=[]
    def __init__ (self,filename_num,filename):
	  self.filename_num=filename_num
          getseed=open(filename,'r')
          seed=getseed.readlines()
          for line in seed:
               self.crawl_queue.append(line.strip('\n'))
              # print line
         # print seed.split('\n')[0]
         # self.crawl_queue=seed.split('\n')
          print self.crawl_queue[0],self.crawl_queue[1]
          getseed.close()

    def download(self,url,user_agent='xidianer',num_retry=3,proxy=None):
          print 'downloading:',url
          headers={'User-agent':user_agent}
          request=urllib2.Request(url,headers=headers)
          opener=urllib2.build_opener()
          if proxy:#set proxy
             proxy_params={urlparse.urlparse(url).scheme:proxy}
             opener.add_hander(urllib2.ProxyHandler(proxy_params))
          try:
	    # print self.__original
             response=urllib2.urlopen(request,timeout=5)#set timeout if a html cannot be download
             #html_nobom=html.decode("utf-8")
             if response.info().get('Content-Encoding')=='gzip':
                 compresseddata=response.read()
                 compressedstream=StringIO(compresseddata)
                 gzipper=gzip.GzipFile(fileobj=compressedstream)
                 html=gzipper.read()
             else:
		 #html=opener.open(request).read()
                 html=response.read()
          except ssl.SSLError as e:
              print 'ssl error'
              html=None
          except socket.timeout as e:
              print 'timeout error'
              html=None
          except urllib2.URLError as e:
             print 'download error:',e.reason
             html=None
             if num_retry>0:
                 if hasattr(e,'code') and 500 <= e.code <600:
                  #retry 5xx http errors
                     return self.download(url,user_agent,num_retry-1)
          return html

    def link_crawler(self,adb,link_regex='.*?',delay=5,headers=None,user_agent='kasnd',proxy=None,num_retry=1):
        """Crawl from the given seed URL following links matched by link_regex"""
        fout=open('linkset.txt','w')
        fout2=open('../build_index_all/html_file/'+str(self.filename_num),'w')
 #       self.crawl_queue=[seed_url]    #make seed_url to crawl_queue
        #keep track which url's have seen before
        seen=set(self.crawl_queue)    #make a has seen url 's set
        throttle=Throttle(delay)
        while self.crawl_queue:
	    if self.filename_num == 10001:
			     	break
			#while url=='\n': here may be read a '\n' ,notice
	    url=self.crawl_queue.pop(0)
           # url='http://rs.xidian.edu.cn' #self.crawl_queue.pop()
            print '-----pop----',url
            while url=='':
				 url=self.crawl_queue.pop(0)
				 print '-----pop----',url
           # rp=self.get_robots(url)
            rp=None
            if rp==None or rp.can_fetch(user_agent,url):#if has no robot or can fetch:
                  #if cannot get robots also..
                  throttle.wait(url)
                  html=self.download(url)
                  if html==None:#the html downloading nothing
                      continue
                  adb.put(("%-10s"%str(self.filename_num)),url)
                  fout.write(url)
                  fout.write('\n')
                  print self.filename_num
                 # print '----html---',html
                  soup=BeautifulSoup(html,"lxml")
                #  print '=====raw_text====',raw_text
                  [script.extract() for script in soup.findAll('script')]
                  [style.extract() for style in soup.findAll('style')]
                  raw_text=soup.get_text()
                 #print raw_text,"---------"
                  right_text=" "
                  right_text=deal.get_correct_text(raw_text)
                #  print right_text 
                  if soup.title!=None:
                     tit=str(soup.title).split('>')[1]
                     titt=str(tit).split('<')[0]
         #            fout2.write(url)
                     fout2.write(str(titt))
                     adb.put(("%-10s"%str(self.filename_num)),soup.title.string)
                     print 'title--------',soup.title.string
                  adb.put(("%-10s"%str(self.filename_num)),right_text)
                  fout2.write(right_text)
                  fout2.close()
                  self.filename_num=self.filename_num+1
                  fout2=open('../build_index_all/html_file/'+str(self.filename_num),'w')
                  #filter for links mathching our regular expression
                  for link in self.get_links(html):
                         #   print link
                       if re.search(link_regex,link):
                           # print 'raw-----',link 
                  #form absolute link
                            domain_raw=urlparse.urlparse(url).netloc
                           # print 'domain_raw--------',domain_raw
                            link=urlparse.urljoin(url,link)
                           # link=urlparse.urljoin(domain_raw,link)
      #            print link
                  #check if have already seen this link
                            if link not in seen:
                                seen.add(link)
                      # if same_domain(seed_url,link):
                                print 'adding...',link
                                self.crawl_queue.append(link)
            else:
                print 'Blocked by robots.txt:',url
        fout.close()
        fout2.close()
        adb.close()

    def get_links(self,html):
        """return a list of links from html"""
       # webpage_regex=re.compile('<a[^>]+href=["\'](.*?\.html)["\']',re.IGNORECASE)
       #list of all links from the webpage
        return re.findall('<a[^>]href=["\']([^ ]+cn|[^ ]+com|[^ ]+net|[^ ]+org|[^ ]+com\.cn|[^ ]+cx|[^ ]+wang|[^ ]+cc|[^ ]+xin|[^ ]+net|[^ ]+top|[^ ]tech|[^ ]+html|[^ \t\n\x0B\f\r]+htm)["\']',html)
     #   return re.findall('<a[^>]href=["\']([^ ]+cn|[^ ]+com|[^ ]+net|[^ ]+org|[^ ]+com\.cn|[^ ]+cx|[^ ]+wang|[^ ]+cc|[^ ]+xin|[^ ]+net|[^ ]+top|[^ ]tech|[^ ]+html|[^ \t\n\x0B\f\r]+htm|[^ ]+\?[^ ]+)["\']',html)
       # return re.findall('<a[^>]+href=["\'](.*?)["\']',html)  
    def get_robots(self,url):
       """Initialize robots parser for this domain"""
       rp=robotparser.RobotFileParser()
       try:  
         rp.set_url(urlparse.urljoin(url, '/robots.txt'))
         rp.read()
       except Exception as  e:
         print 'robot\' error',e,'\n'
       return rp
    def get_correct_text(self,raw_text):
       line_count_set=[]
       i=0
       linei=line_info()
       linei.count=0
       linei.lo=0
       linei.hi=0
       line_count_set.append(linei)
       str2='t'
       j=0
       for str1 in raw_text:
         # print str1,'*'
         if str1==u'\n' and str2!=u'\n':
             i=i+1
           #  print str1,str2,'++===+'
         elif str1==u'\n' and str2==u'\n':
          #   print "in"
             i=i+1
             j=j+1
             line_count_set[j-1].hi=i-1
             linei2=line_info()
             linei2.count=0
             linei2.lo=i
             linei2.hi=i
             line_count_set.append(linei2)
            # print "**************8"
         elif str1!=u' ':
            line_count_set[j].count=line_count_set[j].count+1
         #   print line_count_set[j].count,str2,str1,'lkkkkkkk'
         str2=str1
    #find the max count of byte of line
       max_count=[]
       max_countnum=line_count_set[0].count
 #    max_count.append(line_count_set[0])
       index_li=0
       for li in line_count_set:
         if max_countnum<li.count:#may have two same count??
             del max_count[0:]
             max_count.append(index_li)
             max_countnum=li.count
         elif max_countnum==li.count:
             max_count.append(index_li)
         index_li=index_li+1
       result_line=line_info()
       index_findlo=max_count[0]
       index_findhi=max_count[0]
       result_line.count=line_count_set[index_findhi].count
       result_line.lo=line_count_set[index_findhi].lo
       result_line.hi=line_count_set[index_findhi].hi
   #  for x in max_count:
       while True:
           if index_findlo-2<0:
               break 
           tmpcount=line_count_set[index_findlo-2].count
           if tmpcount!=0:
                index_findlo=index_findlo-2
                result_line.lo=index_findlo
                result_line.count=result_line.count+tmpcount
           else:
                break
       while True:
           if index_findhi+2>=len(line_count_set):
                break
           tmp2count=line_count_set[index_findhi+2].count
           if tmp2count!=0:
                index_findhi=index_findhi+2
                result_line.hi=index_findhi
                result_line.count=result_line.count+tmp2count
           else:
                break
   #  for m in line_count_set:
         # print m.lo," ",m.hi," ",m.count," "    
       k=0
       right_text=""
       for str_result in raw_text:
         if k>=result_line.lo and k<=result_line.hi:
             right_text=right_text+str_result 
           #   print right_text,'lllllllllllllll'
         if str_result==u'\n':
              k=k+1
       return right_text 
    def same_domain(self,url1,url2):
        """return true if both urls belong to same domain"""
        print urlparse.urlparse(url1).netloc
        return urlparse.urlparse(url1).netloc==urlparse.urlparse(url2).netloc

class Throttle:
   """add a delay between downloads to the same domain"""
   def __init__(self,delay):
       #amount of delay between downloads for each domain
       self.delay=delay
       #timestamp of when a domain was last accessed
       self.domains={}
   def wait(self,url):
       domain=urlparse.urlparse(url).netloc
       last_accessed=self.domains.get(domain)
       if self.delay>0 and last_accessed is not None:
           sleep_secs=self.delay-(datetime.now()-last_accessed).seconds
           if sleep_secs>0:
               #domain has been accessed recently
               #so need to sleep
               time.sleep(sleep_secs)
       #update the last accessed time
       self.domains[domain]=datetime.now()


class line_info:
   """store the count of word(byte) of line x to y"""


if __name__=='__main__':
	 adb=db.DB()
	 adb.set_flags(db.DB_DUP)
	 adb.open('../search_server_v05_arrange/pageidsearch2.db',dbtype=db.DB_BTREE,flags=db.DB_CREATE)
     #link_crawler("http://www.xidian.edu.cn/",'xidian',delay=0,num_retry=1,user_agent='ksancesds')
         spider1=spider(0,'seed_with.txt')
         spider1.link_crawler(adb,delay=0,num_retry=1,user_agent='ksancesds')

#print download('http://www.baidu.com')
