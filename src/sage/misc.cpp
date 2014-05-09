/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: misc.cpp - miscellaneous functions for SAGE
 * Author : Byungil Jeong
 *
 * Copyright (C) 2004 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to sage_users@listserv.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include <string>
#include "misc.h"
#include <errno.h>
#include <sched.h>
#include <stdarg.h>

#if defined(WIN32)
LARGE_INTEGER perf_freq;
LARGE_INTEGER perf_start;
HANDLE win_err; // stderr in a console
#elif defined(__APPLE__)
#include <mach/mach_time.h>
static double perf_conversion = 0.0;
static uint64_t perf_start;
#else
struct timeval tv_start;
#endif

#if defined(WIN32)
#include <sys/timeb.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <Userenv.h>


int
gettimeofday(struct timeval *tp, void *tzp)
{
  struct _timeb t;

  _ftime(&t);
  tp->tv_sec = t.time;
  tp->tv_usec = t.millitm * 1000;
  return 0;
}
#endif

int getToken(char *str, char *token)
{
  char seps[] = " ,\t\n";
  static char token_buf[MAX_TOKENS][TOKEN_LEN];
  static int token_num = 0;
  static int token_idx = 0;
  char *p_token;

  if (token_num > 0) {
    strcpy(token, token_buf[token_idx]);
    token_num--;
    token_idx++;
  }
  else {
    if (p_token = strtok(str, seps))
      strcpy(token, p_token);
    else {
      token[0] = '\0';
      return 0;
    }

    while(p_token = strtok(NULL, seps)) {
      strcpy(token_buf[token_num], p_token);
      token_num++;
      if (token_num > MAX_TOKENS) {
        std::cout << "SAGE getToken : token num overflow" << std::endl;
        _exit(0);
      }
    }
    token_idx = 0;
  }

  return token_num;
}

int getToken(char *str, char *token, char **buf)
{
  char seps[] = " ,\t\n";
  char *p_token;

  if (p_token = strtok_r(str, seps, buf))
    strcpy(token, p_token);
  else {
    token[0] = '\0';
    return 0;
  }

  return strlen(token);
}

int getToken(FILE *fp, char *token)
{
  char seps[] = " ,\t\n{}()";
  int token_idx = 0;

  int curChar;
  bool flag = true;

  while(token_idx == 0 && flag) {
    curChar = fgetc(fp);
    while(curChar != EOF && !strchr(seps, curChar)) {
      token[token_idx] = curChar;
      token_idx++;
      curChar = fgetc(fp);
    }

    if (curChar == EOF)
      flag = false;

    if (strchr("{}()", curChar)) {
      if (token_idx == 0) {
        token[token_idx] = curChar;
        token_idx++;
      }
      else {
        fseek(fp, -1, SEEK_CUR);
      }
    }
  }

  if (flag == false && token_idx == 0)
    return EOF;

  token[token_idx] = '\0';
  return token_idx;
}


int sage::round(float num)
{
  return (int) floor(num + 0.5);
}

int sage::round(double num)
{
  return (int) floor(num + 0.5);
}


char* sage::tokenSeek(char *buf, int tnum)
{
  if (!buf) {
    SAGE_PRINTLOG("sage::tokenSeek : buffer is null");
    return NULL;
  }

  char seps[] = " ,\t\n{}()";
  int strLen = strlen(buf);
  int tokenCnt = 0;
  bool tokenStart = false;

  for (int i=0; i<strLen; i++) {
    if (!strchr(seps, buf[i])) {
      tokenStart = true;
      if (tokenCnt == tnum)
        return buf+i;
    }
    else {
      if (tokenStart)
        tokenCnt++;
      tokenStart = false;
    }
  }

  return NULL;
}

void sage::initUtil()
{
#if defined(WIN32)
  QueryPerformanceCounter(&perf_start);
  QueryPerformanceFrequency(&perf_freq);
  AllocConsole();
  win_err =  GetStdHandle(STD_ERROR_HANDLE);
#elif defined(__APPLE__)
  if ( perf_conversion == 0.0 ) {
    mach_timebase_info_data_t info;
    kern_return_t err = mach_timebase_info( &info );

    //Convert the timebase into seconds
    if( err == 0  )
      perf_conversion = 1e-9 * (double) info.numer / (double) info.denom;
  }
  // Start of time
  perf_start = mach_absolute_time();

  // Initialize the random generator
  srand(getpid());
#else
  // Start of time
  gettimeofday(&tv_start,0);
  // Initialize the random generator
  srand(getpid());
#endif
}

double sage::getTime()
{
  double timeStamp;

#if defined(WIN32)
  LARGE_INTEGER perf_counter;
#else
  struct timeval tv;
#endif

#if defined(WIN32)
  // Windows: get performance counter and subtract starting mark
  QueryPerformanceCounter(&perf_counter);
  timeStamp = (double)(perf_counter.QuadPart-perf_start.QuadPart)*1000000.0/(double)perf_freq.QuadPart;
#elif defined(__APPLE__)
  uint64_t difference = mach_absolute_time() - perf_start;
  timeStamp = perf_conversion*(double)difference*1000000.0;
#else
  // UNIX: gettimeofday
  gettimeofday(&tv, NULL);
  timeStamp = (double)(tv.tv_sec-tv_start.tv_sec)*1000000.0+(double)(tv.tv_usec-tv_start.tv_usec);
#endif

  return timeStamp;
}



int sage::condition_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                         time_t miliSec)
{
  struct timespec abstime;
  timeval t;
  gettimeofday(&t, NULL);
  abstime.tv_sec = (long)t.tv_sec;
  abstime.tv_nsec = t.tv_usec*1000 + miliSec*1000000;

  if (pthread_cond_timedwait(cond, mutex, &abstime) == ETIMEDOUT)
    return SAGE_TIMEOUT;

  return 0;
}

void sageTimer::reset()
{
  startTime = sage::getTime();
  accumulatedTime = 0.0;
  paused = false;
}

void sageTimer::pausedReset()
{
  startTime = sage::getTime();
  accumulatedTime = 0.0;
  paused = true;
}

void sageTimer::pause()
{
  if (!paused)
    accumulatedTime += sage::getTime() - startTime;
  paused = true;
}

void sageTimer::resume()
{
  startTime = sage::getTime();
  paused = false;
}

double sageTimer::getTimeUS(bool resetFlag)
{
  double curTime = sage::getTime();
  double time_diff = 0.0;
  if (!paused)
    time_diff = curTime - startTime;

  double diff_us = time_diff + accumulatedTime;

  if (resetFlag) {
    startTime = curTime;
    accumulatedTime = 0.0;
    paused = false;
  }

  return diff_us;
}

double sageTimer::getTimeSec()
{
  double time_diff = 0.0;
  if (!paused)
    time_diff = sage::getTime() - startTime;

  double elapsed = (time_diff + accumulatedTime)/1000000.0;

  return elapsed;
}

void sage::usleep(unsigned long usec)
{
#ifdef WIN32
  if (usec <= 1000)
    ::Sleep(1);
  else
    ::Sleep((int)(usec/1000));
#else
  ::usleep(usec);
#endif
}

unsigned int sage::sleep(unsigned int seconds)
{
#ifdef WIN32
  ::Sleep(seconds);
  return seconds;
#else
  return ::sleep(seconds);
#endif
}

void sage::switchThread()
{
#if defined(linux) || defined(__APPLE__)
  sched_yield();
#endif

#ifdef WIN32
  SwitchToThread();
#endif
}

int getMax2n(int val)
{
  int pow = 1;
  float root = (float)val;

  while (root > 1) {
    root /= 2.0;
    pow *= 2;
  }

  return pow;
}

int getPixelSize(sagePixFmt type)
{
  int bytesPerPixel = 0;

  switch(type) {
  case PIXFMT_555:
  case PIXFMT_555_INV:
  case PIXFMT_565:
  case PIXFMT_565_INV:
  case PIXFMT_YUV:
    bytesPerPixel = 2;
    break;
  case PIXFMT_888:
  case PIXFMT_888_INV:
    bytesPerPixel = 3;
    break;

  case PIXFMT_RGBS3D:
    bytesPerPixel = 6;
    break;

  case PIXFMT_8888:
  case PIXFMT_8888_INV:
    bytesPerPixel = 4;
    break;

  case PIXFMT_DXT:
    bytesPerPixel = 8;  // 4x4 block time 0.5 bpp
    break;

  case PIXFMT_DXT5:
  case PIXFMT_DXT5YCOCG:
    bytesPerPixel = 16; // 4x4 block time 1 bpp
    break;

  default:
    bytesPerPixel = 3;
    break;
  }

  return bytesPerPixel;
}

sageToken::sageToken(char *str)
{
  strLen = strlen(str);
  tokenBuf = new char[strLen+1];
  idx = 0;

  strcpy(tokenBuf, str);
}

sageToken::sageToken(int len)
{
  tokenBuf = new char[len+1];
  idx = 0;
}

sageToken::~sageToken()
{
  if (tokenBuf)
    delete [] tokenBuf;
}

int sageToken::getToken(char *token)
{
  char seps[] = " ,\t\n\0";
  int tokenIdx = 0;
  int i;

  for (i=idx; i<strLen; i++) {
    char curChar = tokenBuf[i];
    if (!strchr(seps, curChar)) {
      token[tokenIdx] = curChar;
      tokenIdx++;
    }
    else if (tokenIdx > 0) {
      break;
    }
  }

  idx = i;
  token[tokenIdx] = '\0';

  return strLen-idx;
}

int sageToken::putToken(char *token)
{
  char *tokenPt = tokenBuf + idx;
  sprintf(tokenPt, "%s ", token);
  idx = strlen(tokenBuf);

  return idx;
}

int sage::toupper(char *str)
{
  int len = strlen(str);

  for (int i=0; i<len; i++) {
    str[i] = ::toupper(str[i]);
  }

  return 0;
}

int sage::tolower(char *str)
{
  int len = strlen(str);

  for (int i=0; i<len; i++) {
    str[i] = ::tolower(str[i]);
  }

  return 0;
}

bool sage::isDataReady(int sockFd, int sec, int usec)
{
  fd_set readFds;

  FD_ZERO(&readFds);
  FD_SET(sockFd, &readFds);

  struct timeval timeOut;
  timeOut.tv_sec = sec;
  timeOut.tv_usec = usec;

  int ready_fd = select(sockFd+1, &readFds, NULL, NULL, &timeOut);
  if (ready_fd <= 0 || !FD_ISSET(sockFd, &readFds))
    return false;
  else
    return true;
}

int sage::send(int fd, void *buf, int len)   // on both platforms.
{
  if (!buf) {
    SAGE_PRINTLOG("sage::send() - data buffer is null");
    return -1;
  }
  else if (fd < 0) {
    SAGE_PRINTLOG("sage::send() - invalid file descriptor: %d", fd);
    return -1;
  }
  else if (len <= 0) {
    SAGE_PRINTLOG("sage::send() - message length should be larger then zero");
    return -1;
  }

  int bytesSent = 0;
  int bytes = len;
  int actualSend = 0;
  char *buffer = NULL;

  while (bytesSent < len) {
    buffer = ((char *)buf) + bytesSent;
#if defined(WIN32)
    actualSend = ::send(fd, (char *)buffer, bytes, 0);
#else
    actualSend = ::send(fd, (void *)buffer, bytes, 0);
#endif
    if (actualSend < 0) {
      SAGE_PRINTLOG("sage::send() - error in send() system call");
      SAGE_PRINTLOG("=== sent data: %d bytes, remaining %d bytes", bytesSent, bytes);
      return -1;
    }
    else if (actualSend == 0) {
      SAGE_PRINTLOG("sage::send() : connection shutdown");
      return 0;
    }

    bytesSent += actualSend;
    bytes = len - bytesSent;
  }

  return bytesSent;
} // end of sage::send()

int sage::recv(int fd, void *buf, int len, int flags)
{
  int bytesRead = 0;
  int bytes = len;
  int actualRead = 0;

  while (bytesRead < len) {
    char *buffer = (char *)buf + bytesRead;
#if defined(WIN32)
    actualRead = ::recv(fd, (char *)buffer, bytes, flags);
#else
    actualRead = ::recv(fd, (void *)buffer, bytes, flags);
#endif
    if (actualRead < 0) {
      SAGE_PRINTLOG("sage::recv() - error in recv() system call");
      SAGE_PRINTLOG("=== received data: %d bytes, remaining %d bytes", bytesRead, bytes);
      return -1;
    }
    else if (actualRead == 0) {
      SAGE_PRINTLOG("sage::recv() : connection shutdown");
      return 0;
    }

    // BJ bug fix (cube rev 7167)
    if ((flags|MSG_PEEK) && actualRead < len)
      bytesRead = 0;
    else {
      bytesRead += actualRead;
      bytes = len - bytesRead;
    }
  }

  return bytesRead;
} // end of sage::recv()

void sage::printLog(const char* file, int line, const char *format, ...)
{
  va_list vl;
  char msg[2048];

  va_start(vl,format);
  vsprintf(msg,format,vl);
  va_end(vl);

#if defined(WIN32)
  unsigned long res;
  WriteFile(win_err, msg, strlen(msg), &res, NULL);
  WriteFile(win_err, "\n", 2, &res, NULL);
#endif
  fprintf(stderr,"[%6.3f] %s %s %-20s %4d %s\n", sage::getTime()/1000000.0, __DATE__, __TIME__, file, line, msg);
  fflush(stderr);
}

void sage::printLogNL(const char* file, int line, const char *format, ...)
{
  va_list vl;
  char msg[2048];

  va_start(vl,format);
  vsprintf(msg,format,vl);
  va_end(vl);

#if defined(WIN32)
  unsigned long res;
  WriteFile(win_err, msg, strlen(msg), &res, NULL);
  WriteFile(win_err, "\n", 2, &res, NULL);
#endif
  fprintf(stderr,"[%6.3f] %s %s %-20s %4d %s", sage::getTime()/1000000.0, __DATE__, __TIME__, file, line, msg);
  fflush(stderr);
}

void sage::transformFrameNum(int blockFrame, int &curFrame)
{
  int dev = blockFrame - (curFrame%10000);
  if (dev > 0)
    curFrame += dev;
  else if (dev < -9000)
    curFrame += (dev + 10000);
}

int execRemBin(char *ip, char *com, char *xid)
{
  char sshCmd[SAGE_CMD_LEN];

  if (xid)
    sprintf(sshCmd, "/usr/bin/ssh -fx %s \"env DISPLAY=:%s %s \" ", ip, xid, com);
  else
    sprintf(sshCmd, "/usr/bin/ssh -fx %s \"env DISPLAY=:0.0 %s \" ", ip, com);

  // Launch the process
  system(sshCmd);

  std::cout << " sshCmd = " << sshCmd << std::endl;
  std::cout << std::endl;

  return 0;
}

#if defined(WIN32)
void sage::win32Init()
{
  // Initialize Winsock
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2,2), &wsaData);
}
#endif


// Function from Stuart Levy, NCSA
// Reads a string with a unit, K or M
long getnumber( char *str )
{
  char *ep;
  long v = strtoul( str, &ep, 0 );
  switch(*ep) {
  case '\0': case '/': return v;
  case 'k': case 'K': return v << 10;
  case 'm': case 'M': return v << 20;
  default:
    SAGE_PRINTLOG("expected a number (with optional 'k'/'m' suffix), not \"%s\"\n", str);
    exit(1);
  }
}

int sageIndexTable::generateTable()
{
  int entrySize = entries.size();
  if (entrySize < 1) {
    SAGE_PRINTLOG("sageIndexTable::generateTable : no table entry");
    return -1;
  }

  minID = entries[0];
  maxID = entries[0];
  for (int i=0; i<entrySize; i++) {
    minID = MIN(minID, entries[i]);
    maxID = MAX(maxID, entries[i]);
  }

  int size = maxID - minID + 1;
  if (size > 0) {
    table = new int[size];
    for (int i=0; i<size; i++)
      table[i] = -1;
    for (int i=0; i<entrySize; i++)
      table[entries[i]-minID] = i;
  }
  else {
    table = NULL;
    SAGE_PRINTLOG("sageIndexTable::generateTable : invalid table size");
    return -1;
  }

  return 0;
}

int sageIndexTable::setTable(int id, int index)
{
  if (id > maxID || id < minID) {
    SAGE_PRINTLOG("sageIndexTable::setIndex : ID is out of range");
    return -1;
  }

  table[id-minID] = index;

  return 0;
}

int sageIndexTable::getIndex(int id)
{
  if (id > maxID || id < minID) {
    SAGE_PRINTLOG("sageIndexTable::getIndex : ID is out of range");
    return -1;
  }

  return table[id-minID];
}

int sageIndexTable::getID(int idx)
{
  if (idx >= entries.size() || idx < 0) {
    SAGE_PRINTLOG("sageIndexTable::getID : Index is out of range");
    return -1;
  }

  return entries[idx];
}

sageIndexTable::~sageIndexTable()
{
  if (table)
    delete [] table;

  entries.clear();
}



// Inspired by Nvidia toolkit

using namespace std;

data_path::data_path(std::string subdir)
{
  std::string homedir;
  std::string sagedir;

  char *sageDir = getenv("SAGE_DIRECTORY");
#if defined(WIN32)
  TCHAR szHomeDirBuf[MAX_PATH] = { 0 };
  HANDLE hToken = 0;
  OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );
  DWORD BufSize = MAX_PATH;
  GetUserProfileDirectory( hToken, szHomeDirBuf, &BufSize );
  CloseHandle( hToken );
  homedir = std::string( (char*)szHomeDirBuf ) + std::string("/.sageConfig") + subdir;// Declare variables
#else
  homedir = std::string( getenv("HOME") ) + std::string("/.sageConfig") + subdir;
#endif
  if (sageDir)
    sagedir = std::string( sageDir ) + string("/sageConfig") + subdir;
  // First search in current directory
  //path.push_back( "." );
#if defined(WIN32)
  char *cwd = _getcwd( NULL, 0 );
#else
  char *cwd = getcwd( NULL, 0 );
#endif
  if (cwd) {
    path.push_back( cwd );
    free(cwd);
  }
  // Then search in ~/.sageConfig/ directory
  path.push_back( homedir );
  // Finally search in SAGE_DIRECTORY/sageConfig directory
  if (sageDir) {
    path.push_back( sagedir );
    // add images directory in SAGE installation directory
    path.push_back( sageDir + string("/images") );
  }
}

string data_path::get_path(std::string filename)
{
  FILE* fp;
  bool found = false;
  for(unsigned int i=0; i < path.size(); i++)
  {
    path_name = path[i] + string("/") + filename;
    fp = ::fopen(path_name.c_str(), "r");

    if(fp != 0)
    {
      fclose(fp);
      found = true;
      break;
    }
  }

  if (found == false)
  {
    path_name = filename;
    fp = ::fopen(path_name.c_str(),"r");
    if (fp != 0)
    {
      fclose(fp);
      found = true;
    }
  }

  if (found == false)
    return "";

  int loc = path_name.rfind('\\');
  if (loc == -1)
  {
    loc = path_name.rfind('/');
  }

  if (loc != -1)
    file_path = path_name.substr(0, loc);
  else
    file_path = ".";
  return file_path;
}

string data_path::get_file(std::string filename)
{
  FILE* fp;

  for(unsigned int i=0; i < path.size(); i++)
  {
    path_name = path[i] + string("/") + filename;
    fp = ::fopen(path_name.c_str(), "r");

    if(fp != 0)
    {
      fclose(fp);
      return path_name;
    }
  }

  path_name = filename;
  fp = ::fopen(path_name.c_str(),"r");
  if (fp != 0)
  {
    fclose(fp);
    return path_name;
  }
  return "";
}

// Print the list of directories
void data_path::print()
{
  std::cout << "---------------------" << std::endl;
  std::cout << "File search list:" << std::endl;
  for(unsigned int i=0; i < path.size(); i++)
  {
    std::cout << "\t" << path[i] << std::endl;
  }
  std::cout << "---------------------" << std::endl;
}


// data files, for read only
FILE * data_path::fopen(std::string filename, const char * mode)
{

  for(unsigned int i=0; i < path.size(); i++)
  {
    std::string s = path[i] + "/" + filename;
    FILE * fp = ::fopen(s.c_str(), mode);

    if(fp != 0)
      return fp;
    else if (!strcmp(path[i].c_str(),""))
    {
      FILE* fp = ::fopen(filename.c_str(),mode);
      if (fp != 0)
        return fp;
    }
  }
  // no luck... return null
  return 0;
}

//  fill the file stats structure
//  useful to get the file size and stuff
int data_path::fstat(std::string filename,
#ifdef WIN32
                     struct _stat
#else
                     struct stat
#endif
                     * stat)
{
  for(unsigned int i=0; i < path.size(); i++)
  {
    std::string s = path[i] + string("/") + filename;
#ifdef WIN32
    int fh = ::_open(s.c_str(), _O_RDONLY);
#else
    int fh = ::open(s.c_str(), O_RDONLY);
#endif

    if(fh >= 0)
    {
#ifdef WIN32
      int result = ::_fstat( fh, stat );
#else
      int result = ::fstat (fh,stat);
#endif
      if( result != 0 )
      {
        SAGE_PRINTLOG("An fstat error occurred.\n" );
        return 0;
      }
#ifdef WIN32
      ::_close( fh );
#else
      ::close (fh);
#endif
      return 1;
    }
  }
  // no luck...
  return 0;
}
