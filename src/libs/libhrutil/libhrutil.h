/* Copyright (C) 2005-2011 M. T. Homer Reid
 *
 * This file is part of SCUFF-EM.
 *
 * SCUFF-EM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SCUFF-EM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * libhrutil.h  -- header for general-purpose utility library
 *
 * homer reid   -- 10/2008
 */
#ifndef LIBHRUTIL_H
#define LIBHRUTIL_H 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <complex>
#include <cmath>

/***************************************************************/
/* various random definitions  *********************************/
/***************************************************************/
#define PLUSMINUS 177   // ascii plus-minus symbol

#ifndef M_PI
#define M_PI		3.1415926535897932384
#endif

#define DEG2RAD M_PI/(180.0) 
#define RAD2DEG 180.0/(M_PI) 

/* isnan was standardized in C in 1999, but in C++ only in 2011,
   and is still not universally available ... punt, since
   checking for nan in IEEE arithmetic is trivial: */
#ifndef ISNAN
#define ISNAN(x) ((x) != (x))
#endif


//typedef _Complex double cdouble;
typedef std::complex<double> cdouble;

/***************************************************************/
/* Timing functions  *******************************************/
/***************************************************************/
double Secs();
void Tic();
double Toc();

/***************************************************************/
/* String functions  *******************************************/
/***************************************************************/
char *RemoveDirectories(char *s);
char *RemoveExtension(char *s);
char *GetFileExtension(char *s);
char *GetFileBase(char *s);
int Tokenize(char *s, char **Tokens, int MaxTokens, const char *Separators);
int Tokenize(char *s, char **Tokens, int MaxTokens);
int StrCaseCmp(const char *s1, const char *s2);

/***************************************************************/
/* Vararg versions of common functions *************************/
/***************************************************************/
int vsnprintfEC(char *str, size_t size, const char *format, va_list ap);
FILE *vfopen(const char *format, const char *mode, ...);
int vmkdir(const char *format, ...);
int vsystem(const char *format, ...);
void vsetenv(const char *VariableName, const char *format, ...);
char *vstrdup(const char *format, ...);
void ErrExit(const char *format, ...);
void Warn(const char *format, ...); 

/***************************************************************/
/* General-purpose status logging ******************************/
/***************************************************************/ 
void SetLogFileName(const char *format, ...);
void SetConsoleLogging();
void Log(const char *format, ...);
void LogC(const char *format, ...);
void MutexLog(const char *format, ...);
void LogPercent(int n, int N, int Gradations=10);

/***************************************************************/
/* POSIX-specific nonportable system information functions *****/
/***************************************************************/
int GetNumProcs();
void SetNumThreads(int pNumThreads);
int GetNumThreads();
unsigned long GetMemoryUsage(unsigned long *MemoryUsage=0);
void SetCPUAffinity(int WhichProcessor);
void EnableAllCPUs();

/***************************************************************/
/* complex arithmetic ******************************************/
/***************************************************************/
cdouble expi(double x);
cdouble csqrt2(cdouble z);
int S2CD(const char *str, cdouble *z);
char *CD2S(cdouble z, const char *format);
char *CD2S(cdouble z);
void SetDefaultCD2SFormat(const char *format);
void z2s(cdouble z, char *zStr);
char *z2s(cdouble z);

/***************************************************************/
/***************************************************************/
/***************************************************************/
void SetCodeMarker(const char *Marker);
void InstallHRSignalHandler();

/***************************************************************/
/* Other functions  ********************************************/
/***************************************************************/
void *mallocEC(size_t size);
void *reallocEC(void *v, size_t size);
char *strdupEC(const char *s);
void KeyPause();
double RD(double x, double y);
double RD(cdouble x, cdouble y);
void *memdup(void *v, size_t size);
char *GetHostName(); 
char *GetTimeString();

FILE *CreateUniqueFile(const char *Base, int ConsoleMessage, char *FileName);
FILE *CreateUniqueFile(const char *Base, int ConsoleMessage); 
FILE *CreateUniqueFile(const char *Base);

bool IsFinite(double d);
bool IsFinite(cdouble z);

/***************************************************************/
/* command-line argument processing ****************************/
/***************************************************************/

/*--------------------------------------------------------------*/
/* values for the TYPE field in the ArgStruct structure        -*/
/*--------------------------------------------------------------*/
#define PA_DOUBLE  0
#define PA_INT     1
#define PA_STRING  2
#define PA_BOOL    3
#define PA_CDOUBLE 4

/*--------------------------------------------------------------*/
/* user creates an array of structures like this for passage   */
/* to ProcessArguments().                                      */
/*--------------------------------------------------------------*/
typedef struct ArgStruct
 { const char *Name;
   int Type;
   void *Storage;
   const char *Default;
   const char *Description;
 } ArgStruct;

void ASUsage(char *ProgName, ArgStruct *ASArray, const char *format, ...);
void ProcessArguments(int argc, char *argv[], ArgStruct *ASArray);

/***************************************************************/
/* 20120123: i am expanding 'argument' processing into 'option'*/
/*           processing, which is different in several ways:   */
/*                                                             */
/*            (1) in addition to command-line arguments,       */
/*                options and their arguments can be specified */
/*                in a little text file piped into stdin       */
/*                                                             */
/*            (2) options may take more than one argument.     */
/*                                                             */
/*            (3) options can be specified more than once with */
/*                different arguments                          */
/***************************************************************/
typedef struct OptStruct
 { const char *Name;
   int Type;
   int NumArgs;
   int MaxInstances;
   void *Storage;
   int *NumInstances;
   const char *Description;
 } OptStruct;

void OSUsage(char *ProgName, OptStruct *OSArray, const char *format, ...);
void ProcessOptions(int argc, char *argv[], OptStruct *OSArray);
void ProcessOptions(const char *ArgString, OptStruct *OSArray);

#endif