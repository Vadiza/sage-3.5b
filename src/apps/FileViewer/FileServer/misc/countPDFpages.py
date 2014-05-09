import re

def getPDFPageCount(filename):    
    """Counts pages in a PDF document. 
    
    This is GPLed code written by J.Alet on 2004/06/19
    """
    try:
        infile = open(filename, "rb")
        regexp = re.compile(r"(/Type) ?(/Page)[/ \r\n]")
        pagecount = 0
        for line in infile.xreadlines(): 
            pagecount += len(regexp.findall(line))
        infile.close()
        return pagecount
    except:
        return -1

