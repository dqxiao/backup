create or replace function cluster(instanceid text, targettext text) returns text as $$

import nltk 
from nltk.cluster import KMeansClusterer, euclidean_distance
from sklearn.feature_extraction.text import HashingVectorizer
from nltk.corpus import stopwords
import numpy
import cPickle as pickle
#import sys

class KMeansClusterer:
    def __init__(self):
        self.stopList=stopwords.words('english')
    def loadBestWords(self):
        self.bestwords=pickle.load(open("/Users/dongqingxiao/pythonEx/basicLearning/birdModel/bestWord.p","rb"))

    def loadModel(self):
        self.clusterer=pickle.load(open("/Users/dongqingxiao/pythonEx/basicLearning/kmeanClusterModel.p","rb"))

    def featureExtraction(self,d,bestwords):

        freqDict={}

        wordSet=[word for word in d.lower().split()]
        wordCount=0
        for word in wordSet:
            if word in self.stopList:
                continue
            if word in bestwords:
                if word in freqDict:
                    freqDict[word]+=1
                else:
                    freqDict[word]=1

                wordCount+=1
        result=map(lambda x: float(freqDict[x])/wordCount if x in freqDict else 0,bestwords)
        return result

    def clusterify(self,document):
        vector=numpy.array(self.featureExtraction(document,self.bestwords))
        result=self.clusterer.classify(vector)

        return result


class EMClusterer:
    def __init___(self):
        self.stopList=stopwords.words('english')

    def loadBestWords(self):
        self.bestwords=pickle.load(open("/Users/dongqingxiao/pythonEx/basicLearning/birdModel/bestWord.p","rb"))

    def loadModel(self):
        self.clusterer=pickle.load(open("/Users/dongqingxiao/pythonEx/basicLearning/EMClustererModel.p","rb"))

    def featureExtraction(self,d,bestwords):

        freqDict={}

        wordSet=[word for word in d.lower().split()]
        wordCount=0
        for word in wordSet:
            if word in self.stopList:
                continue
            if word in bestwords:
                if word in freqDict:
                    freqDict[word]+=1
                else:
                    freqDict[word]=1

                wordCount+=1
        result=map(lambda x: float(freqDict[x])/wordCount if x in freqDict else 0,bestwords)
        return result

    def clusterify(self,document):
        vector=numpy.array(self.featureExtraction(line,self.bestwords))
        result=self.clusterer.classify(vector)

        return result









def run_mode(instanceid,targettext):
    
    if(instanceid=="cluster_1"):
        cluster=KMeansClusterer()
    else:
        cluster=EMClusterer()
    cluster.loadBestWords()
    cluster.loadModel()
    result=cluster.clusterify(targettext)
    return result



#return instanceid
return run_mode(instanceid,targettext)
$$ LANGUAGE plpythonu;