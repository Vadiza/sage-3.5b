import os.path, os, sys, shutil
import traceback as tb

opj = os.path.join


CONFIG_DIR = ".sageConfig"
USER_DIR = os.path.expanduser("~")
USER_CONFIG_DIR = opj( USER_DIR, CONFIG_DIR )
SAGE_DIR = os.path.realpath(os.environ["SAGE_DIRECTORY"])    # where sage is located
DEFAULT_CONFIG_DIR = opj( SAGE_DIR, CONFIG_DIR.lstrip(".") )


# first make the user config dir if it doesnt exist
if not os.path.isdir(USER_CONFIG_DIR):
    os.makedirs(USER_CONFIG_DIR)
    os.makedirs( opj(USER_CONFIG_DIR, "applications", "pid") )
    os.makedirs( opj(USER_CONFIG_DIR, "applications", "conf") )
    # prepare the file library
    os.makedirs( opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "image") )
    os.makedirs( opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "pdf") )
    os.makedirs( opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "audio") )
    os.makedirs( opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "video") )
    # copy some sample files
    if os.path.isfile( opj(SAGE_DIR, "resources", "cybercommons_committee1.jpg")):
        shutil.copy(opj(SAGE_DIR, "resources", "cybercommons_committee1.jpg"), opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "image"))
    if os.path.isfile( opj(SAGE_DIR, "resources", "sage_logo.png")):
        shutil.copy(opj(SAGE_DIR, "resources", "sage_logo.png"), opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "image"))
    if os.path.isfile( opj(SAGE_DIR, "resources", "SAGE_interaction_youtube.mov")):
        shutil.copy(opj(SAGE_DIR, "resources", "SAGE_interaction_youtube.mov"), opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "video"))
    if os.path.isfile( opj(SAGE_DIR, "resources", "SAGE-SC06-paper.pdf")):
        shutil.copy(opj(SAGE_DIR, "resources", "SAGE-SC06-paper.pdf"), opj(USER_CONFIG_DIR, "fileServer", "fileLibrary", "pdf"))

# searches two paths: ~/.sageConfig and $SAGE_DIRECTORY/sageConfig
def getPath(*args):
    fileName = ""
    for a in args:
        fileName = opj(fileName, a)

    userPath = opj( USER_CONFIG_DIR, fileName )
    defaultPath = opj( DEFAULT_CONFIG_DIR, fileName )
    currentPath = opj( os.getcwd(), args[len(args)-1])  # last argument

    if os.path.isfile( currentPath ):
        return currentPath
    elif os.path.isfile( userPath ):
        return userPath
    else:
        return defaultPath


# to get a path to a file in the user's home config directory
def getUserPath(*args):
    p = USER_CONFIG_DIR
    for a in args:
        p = opj(p, a)

    # make the directory... if it doesnt exist
    # this will make the directory even if a file is passed in with some directories before it
    if not os.path.exists(p):
        try:
            d, ext = os.path.splitext(p)
            if ext == "":   # it's a directory
                d = os.path.splitext(p)[0]
            else:           # it's a file
                d = os.path.dirname(p)                
            if not os.path.exists(d): os.makedirs(d)
            
        except:
            print "".join(tb.format_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2]))
        
    return p
    


# to get a path to a file in the default config directory
def getDefaultPath(*args):
    p = DEFAULT_CONFIG_DIR
    for a in args:
        p = opj(p, a)
    return p
