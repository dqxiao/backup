create or replace function snippet(snippetid text, targettext text) returns text as $$
	# create one function to deal with this problme 
import sumy 
from sumy.parsers.plaintext import PlaintextParser
from sumy.nlp.tokenizers import Tokenizer
from sumy.summarizers.lsa import LsaSummarizer
from sumy.summarizers.text_rank import TextRankSummarizer
from sumy.nlp.stemmers.czech import stem_word
from sumy.utils import get_stop_words
from nltk.tokenize import sent_tokenize

def run_mode(snippetid,targettext):
	sentences=sent_tokenize(targettext)

	if(len(sentences)<5):
		return "original"

	#skipp 
	parser=PlaintextParser.from_string(targettext,Tokenizer("english"))
	if snippetid=="snippet_1":
		summarizer = LsaSummarizer(stem_word) # initation input
    	summarizer.stop_words = get_stop_words("english")
    	result=" ".join(summarizer(parser.document, 5))
    	#return result 
    
	if snippetid=="snippet_2":
		summarizer=TextRankSummarizer()
		summarizer.stop_words=get_stop_words("english")
		result=" ".join(summarizer(parser.document,5))
	return result
return run_mode(snippetid,targettext)
$$ LANGUAGE plpythonu;
