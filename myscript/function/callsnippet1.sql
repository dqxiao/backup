create or replace function snippet(snippetid text, targettext text) returns text as $$

def callInit(n):
    d=[]
    for i in xrange(n):
        d.append(0.0)
    return d
def callInit_2(d,d2):
    result=[]
    for i in xrange(d):
        result.append(callInit(d2))
    return result

def intersection(A,B):
    result=map(lambda x: 1.0 if x in B else 0.0,A)
    num=reduce(lambda x,y: x+y,result,0)
    
    # for debuging conveince
    word_result=filter(lambda x: x in B,A)
    if word_result:
        print word_result
    
    return float(num)


class MatrixHelp():
    def __init__(self):
        pass
    
    def distance(self,P1,P2):
        length=len(P1)
        i=0
        distance=0.0
        for i in xrange(length):
            distance+=(P1[i]-P2[i])*(P1[i]-P2[i])
        return distance

    def normal(self,P):
        length=len(P)
        sum_p=0.0
        
        for i in xrange(length):
            sum_p+=P[i]
        for i  in xrange(length):
            P[i]/=sum_p

    def normValue(self,P):
        sum_p=0.0
        for f in P:
            sum_p+=abs(f)
        return sum_p

    def subVec(self,P1,P2):
        length=len(P1)
        result=[]
        for i in xrange(length):
            result.append(P1[i]-P2[i])
        return result
    
    def mergeVec(self,P1,P2):
        
        length=len(P1)
        result=[]
        for i in xrange(length):
            result.append((P1[i]+P2[i])/2)
        return result
    
    def normalize(self,P):
        dMaxValue=-1.0
        dMinValue=2.0
        for f in P:
            dMinValue=min(f,dMinValue)
            dMaxValue=max(f,dMaxValue)
            if dMaxValue-dMinValue< 0.000001:
                return
        result=[]
        
        for f in P:
            f_new=max(f,dMinValue*1.005)
            f_new=min(f_new,dMaxValue*0.995)
            
            f_new=(f_new-dMinValue)/(dMaxValue-dMinValue)
            f_new=((0.5-0.001)/0.5)*f_new+0.001
            
            result.append(f_new)
        return result
    def matrixMultiply(self,A):
        D_1=len(A)
        D_2=len(A[0])
        result=[]
        for i in xrange(D_1):
            #s_result=[0]*D_2
            s_result=callInit(D_2)
            
            for j in xrange(D_2):
                for k in xrange(D_1):
                    s_result[j]+=A[i][k]*A[j][k]
            result.append(s_result)

        return result
    
    def matrixMultipleVector(self,A,P):
        length=len(P)
        #result=[0]*length
        result=callInit(length)
        
        for i in xrange(length):
            for j in xrange(length):
                result[i]+=A[i][j]*P[j]
        return result
    
    def matrixTransposeMultiplyVector(self,A,P):
        length=len(P)
        #result=[0]*length
        result=callInit(length)
        
        for i in xrange(length):
            for j in xrange(length):
                result[i]+=A[j][i]*P[j]
        return result
    
    def printMatrix(self,A):
        for f in A:
            print f
        print "__"*len(A[0])
    
    def printVector(self,P):
        print P
        print "__"*len(P)

class Sentence():
    def __init__(self):
        self.content=""
        self.wordNum=0

class Ranker(object):
    def __init__(self):
        pass
    
    def getRankVec(self,A):
        pass


class HitsRanker(Ranker):
    def __init__(self):
        Ranker.__init__(self)
    
    def getRankVec(self,A):
        return self.hitsIterate(A)
    
    def hitsIterate(self,B):
        length=len(B[0])
        matrixhelp=MatrixHelp()
        L=matrixhelp.matrixMultiply(B)
        H=callInit_2(2,length)
        A=callInit_2(2,length)
        #H=[[0]*length]*2
        #A=[[0]*length]*2
        
        now=0
        other=1
        for i in xrange(length):
            H[now][i]=1.0
            A[now][i]=1.0
        
        threadhold=0.0000001
        while(True):
            A[other]=matrixhelp.matrixTransposeMultiplyVector(L,A[now])
            H[other]=matrixhelp.matrixMultipleVector(L,H[now])
            matrixhelp.normal(A[other])
            matrixhelp.normal(H[other])
            temp=now
            now=other
            other=temp
            if( matrixhelp.normValue(matrixhelp.subVec(H[now],H[other]))< threadhold and
               matrixhelp.normValue(matrixhelp.subVec(A[now],A[other]))<threadhold):
                break
    
        R=matrixhelp.mergeVec(A[now],H[now])
        matrixhelp.normalize(R)
        return R

class PageRanker(Ranker):
    
    def __init__(self):
        Ranker.__init__(self)
    
    
    def getRankVec(self,A):
        return self.PageRankIterate(self.getInitMatrix(A),0.85)
    
    def PageRankIterate(self,A,d):
        # A[][],d:double
        length=len(A[0])
        #P=[[0]*length]*2
        P=callInit_2(2,length)
        now=0
        other=1
        
        for i in xrange(length):
            P[now][i]=1.0/length
        threadhold=0.0000001
        matrixhelp=MatrixHelp()
    
        while True:
            P[other]=matrixhelp.matrixMultipleVector(A,P[now])
            for i in xrange(length):
                P[other][i]=d*P[other][i]+(1-d)
            #swap
            temp=now
            now=other
            other=temp
            
            if(matrixhelp.distance(P[now],P[other])< threadhold):
                break
        matrixhelp.normalize(P[now])
        return P[now]

    def getInitMatrix(self,A):
        length=len(A)
        #B=[[0]*length]*length
        B=callInit_2(length,length)
        
        for j in xrange(length):
            weightSum=0.0
            for k in xrange(length):
                weightSum+=A[j][k]
            for i in xrange(length):
                if(weightSum>0.000001):
                    B[i][j]=A[i][j]/weightSum
                else:
                    B[i][j]=1.0/(length-1)
                if i==j:
                    B[i][j]=0
        
        #print B
        
        return B



class Summarizer():
    
    
    def __init__(self,inputtext,ranker):
        #self.outputFileName=outputFileName
        self.fileContentString=self.readFileToString(inputtext)
        self.hmStopWord=self.initStopWordMap()
        self.ranker=ranker
        self.sentenceList=[]
    
    def fileSummarize(self):
        self.sentenceList=self.initSentenceList()
        A=self.getSentenceWeightMatrix()
        P=self.ranker.getRankVec(A)
        return self.sortAndWriteFile(P)
    
    
    
    def initSentenceList(self):
        sentenceList=[]
        #print self.fileContentString
        for line in self.fileContentString:
            sentence=Sentence()
            sentence.content=self.parseString(line)
            sentenceList.append(sentence)
    
        return sentenceList

    
    def filter_stop_word(self,s_list):
        
        new_list=filter(lambda x: len(x)>2,s_list)
        
        return filter(lambda x: x not in self.hmStopWord, new_list )
    
    def getSentenceWeightMatrix(self):
        
        sentenceList=self.sentenceList
        sentenceNum=len(sentenceList)
        #old_A=[[0.0]*sentenceNum]*sentenceNum
        #A=copy.copy(old_A)
        A=callInit_2(sentenceNum,sentenceNum)
        
        for i in xrange(sentenceNum):
            c_sentence=sentenceList[i].content
            c_raw_list=c_sentence.split()
            c_fine_list=self.filter_stop_word(c_raw_list)
            sentenceList[i].wordNum=len(c_fine_list)
            
            for j in xrange(sentenceNum):
                
                if i==j:
                    A[i][j]=float(sentenceList[i].wordNum)
                
                else:
                    n_sentence=sentenceList[j].content
                    n_raw_list=n_sentence.split()
                    n_fine_list=self.filter_stop_word(n_raw_list)
                    
                    A[i][j]=intersection(c_fine_list,n_fine_list)
        
        #here ok !
        for i in xrange(sentenceNum):
            for j in xrange(i+1,sentenceNum):
                A[i][j]=min(A[i][j],A[j][i])
    
    
        for i in xrange(sentenceNum):
            for j in xrange(sentenceNum):
                if(sentenceList[i].wordNum!=0):
                    # avoid divided by 0
                    A[i][j]=float(A[i][j])/sentenceList[i].wordNum
            #print A
    
        return A

    
    
    def parseString(self,s):
        return s.lower().replace('\t',' ').replace(',',' ').replace('.',' ').replace('"',' ')
    
    def readFileToString(self,inputtext):
        return inputtext.split('.')
    
    def initStopWordMap(self):
        
        stopWords=["a", "about", "an", "and", "are", "as", "at", "be", "but", "by",
                   "for", "has", "have", "he", "his", "her", "in", "is", "it", "its",
                   "of", "on", "or", "she", "that", "the", "their", "they", "this",
                   "to", "was", "which", "who", "will", "with", "you","when","meanwhile","if","would","other","under","out","off","than","more","not"];
        return stopWords

    def sortAndWriteFile(self,P):
        
        #outputfile=file(self.outputFileName,'w')
        length=len(P)
        #print P
        new_p=[]
        for i in xrange(length):
            new_p.append((P[i],i))
        
        new_p.sort()
        new_p.reverse()
        
        line=""
        sort_num=""
        for (pro,num) in new_p:
            line+=str(num)+":"+str(pro)+":"+self.fileContentString[num]+'\n'
            sort_num+=str(num)+' '
        #outputfile.write(line)
        #outputfile.close()
        #print line
        return sort_num 

# try to debuging it, then we can apply it

def run_mode(snippetid,targettext):
    
    ranker=PageRanker()
    inputtext=targettext
    #inputtext="above wrong. above here. wrong idea. good idea"
    #outputFileName="/Users/dongqingxiao/Desktop/Summarizer/s_report.txt"
    summarizer=Summarizer(inputtext,ranker)
    #print summarizer.fileSummarize()
    return summarizer.fileSummarize()

return run_mode(snippetid,targettext)
$$ LANGUAGE plpythonu;
