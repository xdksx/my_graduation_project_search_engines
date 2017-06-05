import sys
reload(sys)
sys.setdefaultencoding('utf-8')
type_file=sys.getfilesystemencoding()
def get_correct_text(raw_text):
       line_count_set=[]   #store all object ,every object str and count..
       i=0
       linei=line_info()
       linei.count=0
       linei.lo=0
       linei.hi=0
       linei.strr=[]
       all_line=raw_text.split('\n')
       sig=0
       for line in all_line:
 #         print line
          ss=line+'\n'
          num=len(ss)
          if num!=0:
               sig=0
               linei.hi=linei.hi+1
               linei.count=linei.count+num
             #  print ss
               linei.strr.append(ss)
          elif sig==0 and num==0:
               sig=1
              # print 'has'
               loo=linei.hi
               line_count_set.append(linei)
               linei=line_info()
               linei.lo=loo+1
               linei.hi=loo+1
               linei.count=0
               linei.strr=[]
#       for xx in line_count_set:
 #          print '--------\n'
  #         print xx.count
   #        for yy in xx.strr:
#             print yy
       rest=sorted(line_count_set,cmp=lambda x,y:cmp(x.count,y.count),reverse=True)
       result=''
       numi=0
       print 'coooo:',len(rest)*0.6
       for numi in range(int(len(rest)*0.5)):
       #    print '--------\n'
       #    print rest[numi].count
           for yy in rest[numi].strr:
 #            print yy
             result=result+' '+yy
       return result

class line_info:
   """store the count of word(byte) of line x to y"""
fin=open('30','r')
strr=fin.read()
#print strr,'\n'
gett=get_correct_text(strr)
#print '-----',gett
