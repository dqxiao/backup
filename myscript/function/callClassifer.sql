create or replace function classify(instanceid text, targettext text) returns text as $$

import cPickle as pickle
from nltk.classify import NaiveBayesClassifier
from nltk.corpus import movie_reviews, stopwords
from nltk.collocations import BigramCollocationFinder
from nltk.metrics import BigramAssocMeasures
import collections, itertools
import nltk.classify.util, nltk.metrics


class TextClassifier:
    
    def __init__(self):
        self.loadBestWords()

    def loadModel(self):
        self.classifier=pickle.load(open("/Users/dongqingxiao/Documents/ResearchProject2014/pgsqlProject/myscript/function/movieModel/classifierModel.p","rb"))

    def processingText(self,line):
        return [word for word in line.lower().split()]

    def textClassify(self,inputS):
        feats=self.best_bigram_word_feats(self.processingText(inputS))
        result=self.classifier.classify(feats)
        return result 
        
        
    def loadBestWords(self):
        self.bestwords=pickle.load(open("/Users/dongqingxiao/Documents/ResearchProject2014/pgsqlProject/myscript/function/movieModel/bestWord.p","rb"))
        
    def best_word_feats(self,words):
        #filter words based on loaded bestWord
        return dict([(word, True) for word in words if word in self.bestwords])
        
    def best_bigram_word_feats(self,words, score_fn=BigramAssocMeasures.chi_sq, n=200):
        bigram_finder = BigramCollocationFinder.from_words(words)
        bigrams = bigram_finder.nbest(score_fn, n)
        d = dict([(bigram, True) for bigram in bigrams]) # bigram 
        d.update(self.best_word_feats(words)) # update d 
        return d

class BirdTextClassifier:
    
    def __init__(self):
        self.loadBestWords()

    def processingText(self,line):
        return [word for word in line.lower().split()]

    def loadModel(self):
        self.classifier=pickle.load(open("/Users/dongqingxiao/Documents/ResearchProject2014/pgsqlProject/myscript/function/birdModel/classifierModel.p","rb"))

    def textClassify(self,inputS):
        feats=self.best_bigram_word_feats(self.processingText(inputS))
        result=self.classifier.classify(feats)
        return result 
        
        
    def loadBestWords(self):
        self.bestwords=pickle.load(open("/Users/dongqingxiao/Documents/ResearchProject2014/pgsqlProject/myscript/function/birdModel/bestWord.p","rb"))
        
    def best_word_feats(self,words):
        #filter words based on loaded bestWord
        return dict([(word, True) for word in words if word in self.bestwords])
        
    def best_bigram_word_feats(self,words, score_fn=BigramAssocMeasures.chi_sq, n=40):
        bigram_finder = BigramCollocationFinder.from_words(words)
        bigrams = bigram_finder.nbest(score_fn, n)
        d = dict([(bigram, True) for bigram in bigrams]) # bigram 
        d.update(self.best_word_feats(words)) # update d 
        return d



def run_mode(classfierID,targettext):
    
    if(classfierID=="classifier_1"):
	   classifier=TextClassifier()
	   classifier.loadModel()
	   result=classifier.textClassify(targettext)
	   return result
    else:
       classifier=BirdTextClassifier()
       classifier.loadModel()
       result=classifier.textClassify(targettext)
       return result



#return instanceid
return run_mode(instanceid,targettext)
$$ LANGUAGE plpythonu;