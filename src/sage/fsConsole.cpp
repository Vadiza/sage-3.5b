/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: fsConsole.cpp - a UI console to control the Free Space Manager
 * Author : Luc Renambot, Byungil Jeong
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

#include "suil.h"
#include "misc.h"
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <readline/rltypedefs.h>

#if defined(SAGE_NETLOG)
#include "nl_log.h"
#endif

// SAGE object
suil uiLib;

// Application list
typedef struct _application_t
{
  char *name;
  int   id;
  FILE *log;
} application_t;

application_t *Applications[64];
FILE *playback;

// When non-zero, this means the user is done using this program.
int done;
bool zeroPerf;
char execArg[256];
int noRecv, noSend, dispX, dispY, rendX, wholeX, intval = 0;

// A static variable for holding the line.
static char *line_read = (char *)NULL;
static char current_command[256];
static char exec_arg[256];

// The names of functions that actually do the work
int com_help     (char *arg);
int com_exit     (char *arg);
int com_exec     (char *arg);
int com_move     (char *arg);
int com_resize   (char *arg);
int com_bg       (char *arg);
int com_depth    (char *arg);
int com_perf     (char *arg);
int com_zeroperf (char *arg);
int com_stopperf (char *arg);
int com_kill     (char *arg);
int com_shutdown (char *arg);
int com_admin    (char *arg);
int com_rate     (char *arg);
int com_intval   (char *arg);
int com_ptr      (char *arg);
int com_share    (char *arg);



// A structure which contains information on the commands
// this program can understand.

typedef struct {
  char *name;                   // User printable name of the function.
  rl_icpfunc_t *func;           // Function to call to do the job.
  char *doc;                    // Documentation for this function.
} COMMAND;

//###
COMMAND commands[] = {

  { (char*)"exec",     com_exec,
    (char*)"Execute an application: <exec app_name config_num (init_x init_y)>" },
  { (char*)"move",     com_move,
    (char*)"Move the window of an application: <move app_id dx dy>" },
  { (char*)"resize",   com_resize,
    (char*)"Resize the window of an application: <resize app_id left right bottom top>" },
  { (char*)"bg",       com_bg,
    (char*)"Change background color: <bg red green blue>" },
  { (char*)"depth",    com_depth,
    (char*)"Change z-order of the windows: <depth #_of_change app_id1 zVal1 ...>" },
  { (char*)"perf",     com_perf,
    (char*)"Request performance information: <perf app_id report_period(sec)>" },
  { (char*)"stopperf", com_stopperf,
    (char*)"Stop performance information report: <stopperf app_id>" },
  { (char*)"zeroperf", com_zeroperf,
    (char*)"set performance numbers zero: <zeroperf>" },
  { (char*)"kill",     com_kill,
    (char*)"Kill an application: <kill app_id>" },
  { (char*)"shutdown", com_shutdown,
    (char*)"Shutdown SAGE: <shutdown>" },
  { (char*)"admin",    com_admin,
    (char*)"Get administrative information and status: <admin>" },
  { (char*)"ptr", com_ptr, 
    (char*)"Move the pointer: <move dx dy>" },
  { (char*)"help", com_help,
    (char*)"Display this text: <help>" },
  { (char*)"exit", com_exit,
    (char*)"Quit the console: <exit>" },
  { (char*)"quit", com_exit,
    (char*)"Quit the console: <quit>" },
  { (char*)"rate", com_rate,
    (char*)"change the frame rate of apps: <rate app_id frame_rate>" },
  { (char*)"intval", com_intval,
    (char*)"change the pause interval of the udp stream : <intval app_id interval>" },
  { (char*)"share", com_share,
    (char*)"sharing app to another fsManager : <share app_id fs_IP fs_port>" },
  { (char*)"?", com_help,
    (char*)"Synonym for help: <?>" },
  { (char *)NULL, (rl_icpfunc_t *)NULL, (char *)NULL }
};
//###


// Functions locally defined
void*  readThread(void *args);
char*  dupstr (char *s);
char*  rl_gets (char *prompt);
char*  command_generator (const char * text, int state);
char** fileman_completion (const char * text, int start, int end);
char*  stripwhite (char *thestring);
int    execute_line (char *line);

COMMAND* find_command (char *name);

////////////////////////////////////////////////////////////////////////////////


void* readThread(void *args)
{
  suil *uiLib = (suil *)args;
  int timestep = 1;

  while (1)
  {
    sageMessage msg;

    msg.init(READ_BUF_SIZE);
    uiLib->rcvMessageBlk(msg);

    if (msg.getCode() == APP_INFO_RETURN)
    {
      char appname[256], cmd[256];
      int ret;
      int appID, left, right, bottom, top, sailID, zValue;

      ret = sscanf((char*)msg.getData(), "%s %d %d %d %d %d %d %d",
                   appname, &appID, &left, &right, &bottom, &top, &sailID, &zValue);

      if (ret != 8)
        printf( "Something wrong, got only %d fields\n", ret);
      else
      {
        if (Applications[appID] == NULL)
        {
          Applications[appID] = (application_t*)malloc(sizeof(application_t));
          Applications[appID]->id   = appID;
          Applications[appID]->name = strdup(appname);

          // Get local time
          time_t local = time(NULL);
          struct tm *current = localtime( & local );

          // Create a file for each application
          char fn[256];
          memset(fn, 0, 256);

          char *sageDir = getenv("SAGE_DIRECTORY");
          if (!sageDir) {
            printf("fsConsole: cannot find the environment variable SAGE_DIRECTORY");
            return NULL;
          }

          data_path path;
          std::string found = path.get_file("log");
          if (found.empty()) {
            printf("fsConsole: cannot find the directory [%s]", "log");
            return NULL;
          }
          const char *logPath = found.c_str();

          /*
            if (noRecv == 0) {
            sprintf(fn, "log/%s-%d-%d-%d-%d-%d-%d.log", // year-month-day hour-minute-second
            Applications[appID]->name,
            current->tm_year, current->tm_mon, current->tm_mday,
            current->tm_hour, current->tm_min, current->tm_sec);
            }
            else {
            sprintf(fn, "log/%s-r%d-s%d-%dx%d-i%d.log", Applications[appID]->name,
            noRecv, noSend, dispX, dispY, intval);
            }
          */

          sprintf(fn, "%s/%s-%s.log", logPath, Applications[appID]->name, execArg);
          printf("fsConsole: opening [%s] log file", fn);
          Applications[appID]->log = fopen(fn, "w+");

          if (noRecv > 0) {
            fprintf(Applications[appID]->log,
                    "%s rcv %d send %d dispRes %d %d rendRes %d %d whole %d %d int %d\n",
                    Applications[appID]->name,
                    noRecv, noSend, dispX, dispY, rendX, dispY, wholeX, dispY, intval);
          }

#if defined(SAGE_NETLOG)
          memset(fn, 0, 256);
          sprintf(fn, "log/%s-%d-%d-%d-%d-%d-%d.nlg", // year-month-day hour-minute-second
                  Applications[appID]->name,
                  current->tm_year, current->tm_mon, current->tm_mday,
                  current->tm_hour, current->tm_min, current->tm_sec);
          NL_logger_module( Applications[appID]->name, fn, NL_LVL_INFO, NL_TYPE_APP, "" );
#endif

          memset(cmd, 0, 256);
          sprintf(cmd, "%d", appID);
          uiLib->sendMessage(STOP_PERF_INFO, cmd);

          memset(cmd, 0, 256);
          sprintf(cmd, "%d %d", appID, 1); // every second
          uiLib->sendMessage(PERF_INFO_REQ, cmd);
        }
      }
    }

    else if (msg.getCode() == UI_APP_SHUTDOWN)
    {
      int ret;
      int appID;

      ret = sscanf((char*)msg.getData(), "%d", &appID);
      if (ret != 1)
        printf( "Something wrong, got only %d fields\n", ret);
      else
      {
        if (Applications[appID]->log)
        {
          printf( "Closing log file for application [%s]\n", Applications[appID]->name);
          fclose(Applications[appID]->log);
        }
      }

    }

    else if (msg.getCode() == UI_PERF_INFO)
    {
      char appname[256], cmd[256];
      int ret;
      int appID, dispNodes, renderNodes, numStreamR;
      float dispBandWidth, dispFrameRate, renderBandWidth, renderFrameRate, packetLoss;

      ret = sscanf((char*)msg.getData(), "%d\nDisplay %f %f %f %d\nRendering %f %f %d",
                   &appID,
                   &dispBandWidth, &dispFrameRate, &packetLoss, &dispNodes,
                   &renderBandWidth, &renderFrameRate, &renderNodes);

      if (ret != 8)
        printf( "Something wrong, got only %d fields\n", ret);
      else
      {
        if (zeroPerf) {
          fprintf(Applications[appID]->log, "%d 0.0 0.0 0.0 0.0 %d %d %s\n", timestep, dispNodes, renderNodes, exec_arg);
        }
        else {
          fprintf(Applications[appID]->log, "%d %f %f %f %f %f %d %d %s\n",
                  timestep, dispBandWidth, dispFrameRate, packetLoss, renderBandWidth,
                  renderFrameRate, dispNodes, renderNodes, exec_arg);
        }

        fflush(Applications[appID]->log);

#if defined(SAGE_NETLOG)
        NL_flush = 1;
        NL_write( Applications[appID]->name, NL_LVL_INFO, "display", "bandwidth=d framerate=d", dispBandWidth, dispFrameRate);
        NL_write( Applications[appID]->name, NL_LVL_INFO, "render",  "bandwidth=d framerate=d", renderBandWidth, renderFrameRate);
#endif

        timestep += 1;
      }
    }

    else
    {
      printf( "\n\tMessage: %d\n\t[%s]\n", msg.getCode(), (char *)msg.getData());
    }

  }

  return NULL;
}



char *dupstr (char *s)
{
  char *r;

  r = (char*)malloc (strlen (s) + 1);
  strcpy (r, s);
  return (r);
}

// Read a string, and return a pointer to it.
// Returns NULL on EOF.
char *rl_gets (char *prompt)
{
  /* If the buffer has already been allocated,
     return the memory to the free pool. */
  if (line_read)
  {
    free (line_read);
    line_read = (char *)NULL;
  }

  /* Get a line from the user. */
  line_read = readline (prompt);

  /* If the line has any text in it,
     save it on the history. */
  if (line_read && *line_read) add_history (line_read);

  return (line_read);
}

// Generator function for command completion.  STATE lets us
//   know whether to start from scratch; without any state
//   (i.e. STATE == 0), then we start at the top of the list.
char*
command_generator (const char * text, int state)
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
                                                          initializing the index variable to 0. */
  if (!state)
  {
    list_index = 0;
    len = strlen (text);
  }

  /* Return the next name which partially matches from the
     command list. */
  while (name = commands[list_index].name)
  {
    list_index++;

    if (strncmp (name, text, len) == 0)
    {
      memset(current_command, 0, 256);
      strcpy(current_command, name);
      return (dupstr(name));
    }
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

// Attempt to complete on the contents of TEXT.  START and END
//   bound the region of rl_line_buffer that contains the word to
//   complete.  TEXT is the word to complete.  We can use the entire
//   contents of rl_line_buffer in case we want to do some simple
//   parsing.  Returnthe array of matches, or NULL if there aren't any.
char**
fileman_completion (const char * text, int start, int end)
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
  {
    matches = rl_completion_matches (text, command_generator);
  }
  else
  {
    // If match found, display the corresponding documentation
    COMMAND *cm = find_command(current_command);
    if (cm)
    {
      matches = (char**)malloc(3*sizeof(char*));
      matches[0] = dupstr((char*)"");
      matches[1] = dupstr(cm->doc);
      matches[2] = NULL;
    }
  }

  return (matches);
}

// Look up NAME as the name of a command, and return a pointer to that
//   command.  Return a NULL pointer if NAME isn't a command name.
COMMAND *find_command (char *name)
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

// Strip whitespace from the start and end of STRING.  Return a pointer
//   into STRING.
char* stripwhite (char *thestring)
{
  register char *s, *t;

  for (s = thestring; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}



// Execute a command line
int execute_line (char *line)
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
  {
    printf("%s: No such command.\n", word);
    return (-1);
  }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}


int
main(int argc, char **argv)
{
  pthread_t thId;
  char *line, *s;
  char com[256];

  sage::initUtil();

  zeroPerf = false;

  // File containing commands
  playback = NULL;
  if (argc >= 2)
  {
    playback = fopen(argv[1], "r");
    if (playback == NULL)
      printf( "Cannot open playback file <%s>\n", argv[1]);
  }

  if (argc > 2) {
    strcpy(execArg, argv[2]);
  }
  else
    strcpy(execArg, "no");

  if (argc > 8)
    intval = atoi(argv[8]);

  // Setup the completion function
  rl_attempted_completion_function = fileman_completion;

  // Clear application list
  for (int k=0;k<64;k++) Applications[k] = NULL;

  // Connect to SAGE
  uiLib.init((char*)"fsManager.conf");
  uiLib.connect(NULL);

  uiLib.sendMessage(SAGE_UI_REG,(char*)" ");

  // Start the receiving thread
  if(pthread_create(&thId, 0, readThread, (void*)&uiLib) != 0)
  {
    return -1;
  }

#if defined(SAGE_NETLOG)
  // Get local time
  time_t local = time(NULL);
  struct tm *current = localtime( & local );
  char fn[256];

  memset(fn, 0, 256);
  sprintf(fn, "log/%s-%d-%d-%d-%d-%d-%d.nlg", // year-month-day hour-minute-second
          "fsConsole",
          current->tm_year, current->tm_mon, current->tm_mday,
          current->tm_hour, current->tm_min, current->tm_sec);
  NL_logger_module( "fsConsole", fn, NL_LVL_INFO, NL_TYPE_APP, "" );
#endif

  /* Loop reading and executing lines until the user quits. */
  for ( ; done == 0; )
  {
    if (playback && !feof(playback))
    {
      memset(com, 0, 256);
      if ( fgets(com, 256, playback) )
      {
        // trailing \n
        com[ strlen(com)-1 ] = '\0';
        s = stripwhite (com);
        int v, pause;
        v = sscanf(s, "pause %d", &pause);
        if (v == 1)
        {
          printf( "PB> Pausing %d sec\n", pause);
          sleep(pause);
        }
        else
          if (*s)
          {
            add_history (s);
            printf( "PB> Exec <%s>\n", s);
            execute_line (s);
          }
      }
    }
    else
    {
      line =  rl_gets((char*)">>> ");

      if (!line) break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (line);

      if (*s)
      {
        add_history (s);
        execute_line (s);
      }
    }
  }

  // Close the command file
  if (playback != NULL)
    fclose(playback);

  return 0;
}





// ****************************************************************
//
//                               Commands
//
// ****************************************************************


int com_exec (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "exec", "arg=s", arg);
#endif
  uiLib.sendMessage(EXEC_APP, arg);
  strcpy(exec_arg, arg);
  return 0;
}

int com_move (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "move", "arg=s", arg);
#endif
  uiLib.sendMessage(MOVE_WINDOW, arg);
  return 0;
}

int com_ptr (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "ptr", "arg=s", arg);
#endif
  uiLib.sendMessage(MOVE_OBJECT, arg);
  return 0;
}

int com_resize (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "resize", "arg=s", arg);
#endif
  uiLib.sendMessage(RESIZE_WINDOW, arg);
  return 0;
}

int com_bg (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "bg", "arg=s", arg);
#endif
  uiLib.sendMessage(SAGE_BG_COLOR, arg);
  return 0;
}

int com_depth (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "depth", "arg=s", arg);
#endif
  uiLib.sendMessage(SAGE_Z_VALUE, arg);
  return 0;
}

int com_zeroperf (char *arg)
{
  zeroPerf = true;
  return 0;
}

int com_rate (char *arg)
{
  uiLib.sendMessage(APP_FRAME_RATE, arg);
  return 0;
}

int com_intval (char *arg)
{
  uiLib.sendMessage(APP_PAUSE_INTV, arg);
  return 0;
}

int com_share (char *arg)
{
  uiLib.sendMessage(SAGE_APP_SHARE, arg);
  return 0;
}

int com_perf (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "perf", "arg=s", arg);
#endif
  zeroPerf = false;
  uiLib.sendMessage(PERF_INFO_REQ, arg);
  return 0;
}

int com_stopperf (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "stopperf", "arg=s", arg);
#endif
  uiLib.sendMessage(STOP_PERF_INFO, arg);
  return 0;
}

int com_kill (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "kill", "arg=s", arg);
#endif
  uiLib.sendMessage(SHUTDOWN_APP, arg);
  return 0;
}

int com_shutdown (char *arg)
{
#if defined(SAGE_NETLOG)
  //NL_flush = 1;
  //NL_write( "fsConsole", NL_LVL_INFO, "shutdown", "arg=s", arg);
#endif
  // Close all the log files
  for (int k=0;k<64;k++)
    if (Applications[k])
      fclose(Applications[k]->log);

#if defined(SAGE_NETLOG)
  NL_logger_del();
#endif

  // Send the message
  uiLib.sendMessage(SAGE_SHUTDOWN, (char*)"shutdown");
  done = 1;

  return 0;
}

int com_admin (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, (char*)"admin", (char*)"arg=s", arg);
#endif
  uiLib.sendMessage(SAGE_ADMIN_CHECK, (char*)"admin");
  return 0;
}


// Print out help for ARG, or for all of the commands
//   if ARG is not present.
int com_help (char *arg)
{
  register int i;
  int printed = 0;

#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, (char*)"help", (char*)"arg=s", arg);
#endif

  for (i = 0; commands[i].name; i++)
  {
    if (!*arg || (strcmp (arg, commands[i].name) == 0))
    {
      printf("%-8s\t\t%s.\n", commands[i].name, commands[i].doc);
      printed++;
    }
  }

  if (!printed)
  {
    printf ("No commands match `%s'.  Possibilties are:\n", arg);

    for (i = 0; commands[i].name; i++)
    {
      /* Print in six columns. */
      if (printed == 6)
      {
        printed = 0;
        printf ("\n");
      }

      printf("%s\t", commands[i].name);
      printed++;
    }

    if (printed)
      printf("\n");
  }
  return (0);
}

// The user wishes to quit using this program.
//   Just set DONE non-zero.
int com_exit (char *arg)
{
#if defined(SAGE_NETLOG)
  NL_flush = 1;
  NL_write( "fsConsole", NL_LVL_INFO, "exit", "arg=s", arg);
#endif

  std::cout << "\t Done with SAGE..." << std::endl;

  done = 1;
  return (0);
}
