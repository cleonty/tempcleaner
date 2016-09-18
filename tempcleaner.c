/*@@ Wedit generated application. Written Sat Jul 12 21:12:49 2014
@@header: c:\lcc\projects\tempcleaner\tempcleanerres.h
@@resources: c:\lcc\projects\tempcleaner\tempcleaner.rc
Do not edit outside the indicated areas */
/*<---------------------------------------------------------------------->*/
/*<---------------------------------------------------------------------->*/
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string.h>
#include <tchar.h>
#include <stdint.h>
#include <stdarg.h>
#include "tempcleanerres.h"
/*<---------------------------------------------------------------------->*/
HINSTANCE hInst;		// Instance handle
HWND hwndMain;		//Main window handle

const int DEFAULT_NDAYS = 30;
FILE *traceOut = NULL;
int traceLevel = 1;
int reportOnly = FALSE;

typedef struct ExpireEntry_tag ExpireEntry;

struct ExpireEntry_tag {
  char dir[MAX_PATH+1];
  int ndays;
  struct ExpireEntry_tag *next;
};

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
/* --- The following code comes from C:\lcc\lib\wizard\statbar.tpl. */

// Global Variables for the status bar control.

HWND  hWndStatusbar = NULL;
void UpdateStatusBar(LPSTR lpszStatusString, WORD partNumber, WORD displayFlags);


void tracePrintf(LPCTSTR fmt, ...) {
  static int counter = 0;
  va_list ap;
  va_start(ap, fmt);
  vfprintf(traceOut, fmt, ap);
  va_end(ap);
  fprintf(traceOut, "\n");
  counter++;
  if (counter > 10) {
    fflush(traceOut);
    counter = 0;
  }
}

int Min(int a, int b) {
  return (a < b) ? a : b;
}

ExpireEntry *ReadConfig(LPCTSTR fileName){
  ExpireEntry *head = NULL, *tail = NULL;
  FILE *fp = fopen(fileName, "r");
  char buffer[MAX_PATH*2];
  if (fp) {
    memset(buffer, 0, sizeof(buffer));
    while(fgets(buffer, sizeof(buffer), fp)) {
      int len = strlen(buffer);
      char *tab = strchr(buffer, '\t');
      if (tab) {
        ExpireEntry *entry = (ExpireEntry*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ExpireEntry));
        int dirLen = (tab - buffer);
        strncpy(entry->dir, buffer, Min(dirLen, MAX_PATH));
        int ndays = atoi(tab + 1);
        entry->ndays = ndays > 0 ? ndays : DEFAULT_NDAYS;
        if (tail) {
          tail->next = entry;
          tail = entry;
        }
        else {
          head = entry;
          tail = entry;
        }
      }
      memset(buffer, 0, sizeof(buffer));
    }
    fclose(fp);
  }

  return head;
}

int RemoveFileOnRestart(LPCTSTR fileName){
  int success = TRUE;
  if (reportOnly){
    tracePrintf("%s success=%d", "RemoveFileOnRestart", fileName, success);
  }
  else {
    success = MoveFileEx(fileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    if (traceLevel) {
      tracePrintf("%s %s success=%d", "RemoveFileOnRestart", fileName, success);
    }
  }
  return success;
}

char *FileTimeToString(FILETIME *ft){
  SYSTEMTIME st;
  FILETIME ftLocal;
  char szLocalDate[255], szLocalTime[255];
  FileTimeToLocalFileTime(ft, &ftLocal );
  FileTimeToSystemTime( ft, &st );
  GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
    szLocalDate, sizeof(szLocalDate));
  GetTimeFormat( LOCALE_USER_DEFAULT, 0, &st, NULL, szLocalTime, sizeof(szLocalTime));
  char *string = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(szLocalDate) + strlen(szLocalTime) + 2);
  sprintf(string, "%s %s", szLocalDate, szLocalTime);
  return string;
}

#define _SECOND ((ULONGLONG) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)

void FileTimeAddNDays(FILETIME *ft, FILETIME *ftResult, int ndays){
  ULONGLONG qwResult;
  qwResult = (((ULONGLONG) ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
  qwResult += (ULONGLONG)ndays * _DAY;
  ftResult->dwLowDateTime  = (DWORD) (qwResult & 0xFFFFFFFF );
  ftResult->dwHighDateTime = (DWORD) (qwResult >> 32 );
}

int isExpiried(FILETIME *ft, int ndays){
  SYSTEMTIME stNow;
  FILETIME ftNow, ftPlusNDays;
  GetSystemTime(&stNow);
  SystemTimeToFileTime(&stNow, &ftNow);
  if (traceLevel > 2) tracePrintf("File creation time = %s(UTC)", FileTimeToString(ft));
  FileTimeAddNDays(ft, &ftPlusNDays, ndays);
  if (traceLevel > 2) tracePrintf("File creation time + %d days = %s(UTC)", ndays, FileTimeToString(&ftPlusNDays));
  if (traceLevel > 2) tracePrintf("Now = %s(UTC)", FileTimeToString(&ftNow));
  return (CompareFileTime(&ftPlusNDays, &ftNow) <= 0);
}

int RemoveFilesOnRestart(LPCTSTR dir, int ndays){

  WIN32_FIND_DATA ffd;
  LARGE_INTEGER filesize;
  TCHAR szDir[MAX_PATH];
  TCHAR szFileName[MAX_PATH];
  size_t length_of_arg;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  DWORD dwError=0;
  char buffer[1024];

  length_of_arg = strlen(dir);

  if (length_of_arg > (MAX_PATH - 3))   {
    tracePrintf("Directory path is too long.");
    return (-1);
  }

  tracePrintf("Target directory is %s, ndays %d", dir, ndays);
  strncpy(szDir, dir, MAX_PATH);
  strcat(szDir, "\\*");

  hFind = FindFirstFile(szDir, &ffd);

  if (INVALID_HANDLE_VALUE == hFind)   {
    tracePrintf("FindFirstFile error");
    return dwError;
  }

  do
      {
    sprintf(szFileName, "%s\\%s", dir, ffd.cFileName);
    FILETIME creationTime = ffd.ftCreationTime;
    int expiried = isExpiried(&creationTime, ndays);
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
        if (traceLevel) {
          tracePrintf("Go into a directory %s", ffd.cFileName);
        }
        RemoveFilesOnRestart(szFileName, ndays);
      }
    }
    if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
      if (traceLevel) {
        tracePrintf("%s expiried %s (created %s)", szFileName, expiried ? "YES" : "NO", FileTimeToString(&creationTime));
      }
      if (expiried) {
        RemoveFileOnRestart(szFileName);
      }
    }
  }
  while (FindNextFile(hFind, &ffd) != 0);

  dwError = GetLastError();
  if (dwError != ERROR_NO_MORE_FILES)   {
    tracePrintf("FindFirstFile error 2");
  }

  FindClose(hFind);
  UpdateStatusBar("Ready", 0, 0);
  return dwError;
}



/*------------------------------------------------------------------------
Procedure:     UpdateStatusBar ID:1
Purpose:       Updates the statusbar control with the appropiate
text
Input:         lpszStatusString: Charactar string that will be shown
partNumber: index of the status bar part number.
displayFlags: Decoration flags
Output:        none
Errors:        none

------------------------------------------------------------------------*/
void UpdateStatusBar(LPSTR lpszStatusString, WORD partNumber, WORD displayFlags){
  if (hWndStatusbar) {
    SendMessage(hWndStatusbar,
      SB_SETTEXT,
      partNumber | displayFlags,
      (LPARAM)lpszStatusString);
  }
}


/*------------------------------------------------------------------------
Procedure:     MsgMenuSelect ID:1
Purpose:       Shows in the status bar a descriptive explaation of
the purpose of each menu item.The message
WM_MENUSELECT is sent when the user starts browsing
the menu for each menu item where the mouse passes.
Input:         Standard windows.
Output:        The string from the resources string table is shown
Errors:        If the string is not found nothing will be shown.
------------------------------------------------------------------------*/
LRESULT MsgMenuSelect(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam){
  static char szBuffer[256];
  UINT   nStringID = 0;
  UINT   fuFlags = GET_WM_MENUSELECT_FLAGS(wparam, lparam) & 0xffff;
  UINT   uCmd    = GET_WM_MENUSELECT_CMD(wparam, lparam);
  HMENU  hMenu   = GET_WM_MENUSELECT_HMENU(wparam, lparam);

  szBuffer[0] = 0;                            // First reset the buffer
  if (fuFlags == 0xffff && hMenu == NULL)     // Menu has been closed
        nStringID = 0;

  else if (fuFlags & MFT_SEPARATOR)           // Ignore separators
        nStringID = 0;

  else if (fuFlags & MF_POPUP)                // Popup menu
    {
      if (fuFlags & MF_SYSMENU)               // System menu
            nStringID = IDS_SYSMENU;
      else
        // Get string ID for popup menu from idPopup array.
        nStringID = 0;
    }  // for MF_POPUP
  else                                        // Must be a command item
  nStringID = uCmd;                       // String ID == Command ID

  // Load the string if we have an ID
  if (0 != nStringID)        LoadString(hInst, nStringID, szBuffer, sizeof(szBuffer));
  // Finally... send the string to the status bar
  UpdateStatusBar(szBuffer, 0, 0);
  return 0;
}


/*------------------------------------------------------------------------
Procedure:     InitializeStatusBar ID:1
Purpose:       Initialize the status bar
Input:         hwndParent: the parent window
nrOfParts: The status bar can contain more than one
part. What is difficult, is to figure out how this
should be drawn. So, for the time being only one is
being used...
Output:        The status bar is created
Errors:
------------------------------------------------------------------------*/
void InitializeStatusBar(HWND hwndParent,int nrOfParts){
  const int cSpaceInBetween = 8;
  int   ptArray[40];   // Array defining the number of parts/sections
  RECT  rect;
  HDC   hDC;

  /* * Fill in the ptArray...  */

  hDC = GetDC(hwndParent);
  GetClientRect(hwndParent, &rect);

  ptArray[nrOfParts-1] = rect.right;
  //---TODO--- Add code to calculate the size of each part of the status
  // bar here.

  ReleaseDC(hwndParent, hDC);
  SendMessage(hWndStatusbar,
    SB_SETPARTS,
    nrOfParts,
    (LPARAM)(LPINT)ptArray);

  UpdateStatusBar("Ready", 0, 0);
  //---TODO--- Add code to update all fields of the status bar here.
  // As an example, look at the calls commented out below.

  //    UpdateStatusBar("Cursor Pos:", 1, SBT_POPOUT);
  //    UpdateStatusBar("Time:", 3, SBT_POPOUT);
}


/*------------------------------------------------------------------------
Procedure:     CreateSBar ID:1
Purpose:       Calls CreateStatusWindow to create the status bar
Input:         hwndParent: the parent window
initial text: the initial contents of the status bar
Output:
Errors:
------------------------------------------------------------------------*/
static BOOL CreateSBar(HWND hwndParent,char *initialText,int nrOfParts){
  hWndStatusbar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER|SBARS_SIZEGRIP,
    initialText,
    hwndParent,
    IDM_STATUSBAR);
  if(hWndStatusbar)    {
    InitializeStatusBar(hwndParent,nrOfParts);
    return TRUE;
  }

  return FALSE;
}

int GetFileName(char *buffer,int buflen){
  char tmpfilter[40];
  int i = 0;
  OPENFILENAME ofn;

  memset(&ofn,0,sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hInstance = GetModuleHandle(NULL);
  ofn.hwndOwner = GetActiveWindow();
  ofn.lpstrFile = buffer;
  ofn.nMaxFile = buflen;
  ofn.lpstrTitle = "Open";
  ofn.nFilterIndex = 2;
  ofn.lpstrDefExt = "txt";
  strcpy(buffer,"*.txt");
  strcpy(tmpfilter,"All files,*.*,Text Files,*.txt");
  while(tmpfilter[i]) {
    if (tmpfilter[i] == ',')			tmpfilter[i] = 0;
    i++;
  }
  tmpfilter[i++] = 0;
  tmpfilter[i++] = 0;
  ofn.Flags = 539678;
  ofn.lpstrFilter = tmpfilter;
  return GetOpenFileName(&ofn);

}

/*<---------------------------------------------------------------------->*/
/*@@0->@@*/
static BOOL InitApplication(void){
  WNDCLASS wc;

  memset(&wc,0,sizeof(WNDCLASS));
  wc.style = CS_HREDRAW|CS_VREDRAW |CS_DBLCLKS ;
  wc.lpfnWndProc = (WNDPROC)MainWndProc;
  wc.hInstance = hInst;
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszClassName = "tempcleanerWndClass";
  wc.lpszMenuName = MAKEINTRESOURCE(IDMAINMENU);
  wc.hCursor = LoadCursor(NULL,IDC_ARROW);
  wc.hIcon = LoadIcon(NULL,IDI_APPLICATION);
  if (!RegisterClass(&wc))		return 0;
  /*@@0<-@@*/
  // ---TODO--- Call module specific initialization routines here

  return 1;
}

/*<---------------------------------------------------------------------->*/
/*@@1->@@*/
HWND CreatetempcleanerWndClassWnd(void){
  return CreateWindow("tempcleanerWndClass","tempcleaner",
    WS_MINIMIZEBOX|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZEBOX|WS_CAPTION|WS_BORDER|WS_SYSMENU|WS_THICKFRAME,
    CW_USEDEFAULT,0,CW_USEDEFAULT,0,
    NULL,
    NULL,
    hInst,
    NULL);
}
/*@@1<-@@*/
/*<---------------------------------------------------------------------->*/
/* --- The following code comes from C:\lcc\lib\wizard\aboutdlg.tpl. */
BOOL _stdcall AboutDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
  case WM_CLOSE:
    EndDialog(hwnd,0);
    return 1;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      EndDialog(hwnd,1);
      return 1;
    }
    break;
  }
  return 0;
}

/*<---------------------------------------------------------------------->*/
/* --- The following code comes from C:\lcc\lib\wizard\defOnCmd.tpl. */
/* --- The following code comes from C:\lcc\lib\wizard\aboutcmd.tpl. */
void MainWndProc_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify){
  char buffer[MAX_PATH];

  switch(id) {
    // ---TODO--- Add new menu commands here
    /*@@NEWCOMMANDS@@*/
  case IDM_ABOUT:
    DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),
      hwndMain,AboutDlg);
    break;

  case IDM_OPEN:
    //GetFileName(buffer,sizeof(buffer));
    //int success = RemoveFileOnRestart(buffer);
    break;

  case IDM_EXIT:
    PostMessage(hwnd,WM_CLOSE,0,0);
    break;
  }
}

/*<---------------------------------------------------------------------->*/
/*@@2->@@*/
LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
  switch (msg) {
    /*@@3->@@*/
  case WM_SIZE:
    SendMessage(hWndStatusbar,msg,wParam,lParam);
    InitializeStatusBar(hWndStatusbar,1);
    break;
  case WM_MENUSELECT:
    return MsgMenuSelect(hwnd,msg,wParam,lParam);
  case WM_COMMAND:
    HANDLE_WM_COMMAND(hwnd,wParam,lParam,MainWndProc_OnCommand);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hwnd,msg,wParam,lParam);
  }
  /*@@3<-@@*/
  return 0;
}
/*@@2<-@@*/

/*<---------------------------------------------------------------------->*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow){
  MSG msg;
  HANDLE hAccelTable;
  int runGUI = FALSE;
  hInst = hInstance;

  if (runGUI) {
    if (!InitApplication())		return 0;
    hAccelTable = LoadAccelerators(hInst,MAKEINTRESOURCE(IDACCEL));
    if ((hwndMain = CreatetempcleanerWndClassWnd()) == (HWND)0)		return 0;
    CreateSBar(hwndMain,"Ready",1);
    ShowWindow(hwndMain,SW_SHOW);
  }
  traceOut = fopen("tempcleaner.log", "w");
  if (traceLevel) {
    tracePrintf("Command line: '%s'", lpCmdLine);
  }
  if (strstr(lpCmdLine, "-reportOnly")) {
    reportOnly = TRUE;
  }
  if (reportOnly) {
    tracePrintf("Program is running in report only mode");
  }
  int ndays = 30;
  ExpireEntry *entries = ReadConfig("tempcleaner.conf");
  for (ExpireEntry *entry = entries; entry; entry = entry->next) {
    RemoveFilesOnRestart(entry->dir, entry->ndays);
  }

  if (runGUI) {
    while (GetMessage(&msg,NULL,0,0)) {
      if (!TranslateAccelerator(msg.hwnd,hAccelTable,&msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
  if (traceOut) {
    fclose(traceOut);
  }
  return msg.wParam;
}
