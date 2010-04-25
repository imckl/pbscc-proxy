#include <windows.h>
#include <stdio.h>
#include "scc.h"
#include "mstring.h"

#define PBSCC_REVLEN  100
//#define PBSCC_VERLEN  64
//#define PBSCC_DDBLEN  120
#define PBSCC_MSGLEN  100
#define PBSCC_CMTLENALL 350 //PBSCC_VERLEN+PBSCC_DDBLEN+PBSCC_CMTLEN+60 for specsymbols like: <ddb></ddb>...
#define MAXFULLPATH 4000

#define PBSCC_UID  30


#define WM_MANAGEOK		WM_USER+2
#define WM_COMMENTHST	WM_USER+3
//#define WM_LOADDDB      WM_USER+4
#define UM_SETFOCUS     WM_USER+5

//approximately sructure of the second parameter of SccQueryInfoEx callback function
typedef struct {
	DWORD        cb;      //structure size?
	DWORD        dw1;     //unknown = 0
	DWORD        status;  //object status? always = 1 
	LPSTR        object;  //object (full path)
	LPSTR        version; //object version
	DWORD        dw2;     //unknown = 0
	DWORD        dw3;     //unknown = 0
	DWORD        dw4;     //unknown = 0
	DWORD        dw5;     //unknown = 0
}INFOEXCALLBACKPARM;

typedef int  (*INFOEXCALLBACK)(LPVOID cbParm,INFOEXCALLBACKPARM * parm);

SCCEXTERNC SCCRTN EXTFUN SccQueryInfoEx(LPVOID pContext, 
			LONG nFiles, 
			LPCSTR* lpFileNames, 
			LPLONG lpStatus,
			INFOEXCALLBACK cbFunc,  //callback function to notify PB about version for each object
			LPVOID cbParm           //the reference to PB internal structure (used in callback function as first parm)
		);


typedef struct {
	CHAR          lpCallerName[SCC_PRJPATH_LEN+1];
	CHAR          lpProjName[SCC_PRJPATH_LEN+1]; //path to the SVN folder
	int           cbProjName;       //the length of the project name
	CHAR          lpProjPath[SCC_PRJPATH_LEN+1]; //local project path of PB
	int           cbProjPath;       //the lenght of the project path
	CHAR          lpUser[SCC_USER_LEN+1];
	CHAR          lpComment[PBSCC_CMTLENALL+1];
	LPTEXTOUTPROC lpOutProc;        //the output client procedure
	DWORD         dwLastUpdateTime; //the time(tick) of the last update operation
	DWORD         dwLastCommitTime; //the time(tick) of the last commit operation
	bool          isLastAddRemove;  //contains true if last operation was add or remove
//	FILE          *fddb;
	CHAR          PBVersion[MAX_PATH+1]; //the version of the powerbuilder
	CHAR          PBTarget[MAXFULLPATH]; //the path to the local target file
	//the information for which files we need the comment
	LONG          nFiles; 
	LPCSTR        *lpFileNames;
	SCCCOMMAND    eSCCCommand;
	CHAR          lpTargetsTmp[MAXFULLPATH]; //the path where this context will store target filenames
	CHAR          lpOutTmp[MAXFULLPATH]; //the path where this context will store svn stdout
	CHAR          lpErrTmp[MAXFULLPATH]; //the path where this context will store svn stderr
	CHAR          lpMsgTmp[MAXFULLPATH]; //the path where this context will store user message
//	bool          noDDB; //is true if no ddb is set in registry
	bool          doLock; //info from registry: shows if we want to lock on checkout.
	unsigned long cacheTtlMs;  //info from registry: time to live for cache in milliseconds
	mstring*      pipeOut;     //here we are storing stdout of the child process
	mstring*      pipeErr;     //here we are storing stderr of the child process
	CHAR          uid[PBSCC_UID+1];
	CHAR          pwd[PBSCC_UID+1];
	HWND          parent;
}THECONTEXT;


typedef struct {
	int    len;
	char*  ptr;
} PASCALSTR;

#define EOPS(x)	(x->ptr+x->len)


#define DELAYFORNEWCOMMENT 2999
//#define DELAYFORUPDATE 180000
//#define DEFDDBITEM "(Anomalie ou evolution sans DDB)"
#define PBSCC_REGPATH "SOFTWARE\\FM2i\\PBSCC Proxy"
#define PBSCC_REGKEY HKEY_LOCAL_MACHINE


void log(const char* szFmt,...);
bool ShowSysError(char*info,int err=0);
bool _loginscc(THECONTEXT*ctx);
void _msg(THECONTEXT*ctx,char * s);
extern HINSTANCE	hInstance;