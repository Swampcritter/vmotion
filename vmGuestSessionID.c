/*
 * vmGuestSessionId.c
 *
 * Display the Session ID for a VM Guest. If the Session ID changes, send an alert.
 *
 * This can be compiled for Linux by doing something like the following:
 * gcc -g -o vmguestsessionid -ldl -I<path to VMware headers> vmGuestSessionId.c
 *
 */


#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#   include <dlfcn.h>
#endif

#include "vmGuestLib.h"

#ifdef _WIN32
#define SLEEP(x) Sleep(x * 1000)
#else
#define SLEEP(x) sleep(x)
#endif

//#define DEBUG
#define OEM

static Bool done = FALSE;

/* Functions to dynamically load from the GuestLib library. */
char const * (*GuestLib_GetErrorText)(VMGuestLibError);
VMGuestLibError (*GuestLib_OpenHandle)(VMGuestLibHandle*);
VMGuestLibError (*GuestLib_CloseHandle)(VMGuestLibHandle);
VMGuestLibError (*GuestLib_UpdateInfo)(VMGuestLibHandle handle);
VMGuestLibError (*GuestLib_GetSessionId)(VMGuestLibHandle handle,
                                         VMSessionId *id);

typedef unsigned long long int uint64_t;
void getSessionAlert(char *FromFile,uint64_t localId);

/*
 * Handle for use with shared library.
 */

#ifdef _WIN32
HMODULE dlHandle = NULL;
#else
void *dlHandle = NULL;
#endif

/*
 * GuestLib handle.
 */
VMGuestLibHandle glHandle;



/*
 * Macro to load a single GuestLib function from the shared library.
 */

#ifdef _WIN32
#define LOAD_ONE_FUNC(funcname)                                      \
   do {                                                              \
      (FARPROC)funcname = GetProcAddress(dlHandle, "VM" #funcname);  \
      if (funcname == NULL) {                                        \
         error = GetLastError();                                     \
         printf("Failed to load \'%s\': %d\n",                       \
                #funcname, error);                                   \
         return FALSE;                                               \
      }                                                              \
   } while (0)

#else

#define LOAD_ONE_FUNC(funcname)                           \
   do {                                                   \
      funcname = dlsym(dlHandle, "VM" #funcname);         \
      if ((dlErrStr = dlerror()) != NULL) {               \
         printf("Failed to load \'%s\': \'%s\'\n",        \
                #funcname, dlErrStr);                     \
         return FALSE;                                    \
      }                                                   \
   } while (0)

#endif


Bool
LoadFunctions(void)
{
   /*
    * First, try to load the shared library.
    */
#ifdef _WIN32
   DWORD error;

   dlHandle = LoadLibrary("vmGuestLib.dll");
   if (!dlHandle) {
      error = GetLastError();
      printf("LoadLibrary failed: %d\n", error);
      return FALSE;
   }
#else
   char const *dlErrStr;

   dlHandle = dlopen("libvmGuestLib.so", RTLD_NOW);
   if (!dlHandle) {
      dlErrStr = dlerror();
      printf("dlopen failed: \'%s\'\n", dlErrStr);
      return FALSE;
   }
#endif

   /* Load all the individual library functions. */
   LOAD_ONE_FUNC(GuestLib_GetErrorText);
   LOAD_ONE_FUNC(GuestLib_OpenHandle);
   LOAD_ONE_FUNC(GuestLib_CloseHandle);
   LOAD_ONE_FUNC(GuestLib_UpdateInfo);
   LOAD_ONE_FUNC(GuestLib_GetSessionId);

   return TRUE;
}

/* Compare the two file data sets and see if they are exact or not */
void getSessionAlert(char *FromFile,uint64_t localId )
{
	char bufferId[256];
        int draw_line;
	time_t rawtime;
	FILE *save_output, *update_file;

	time ( &rawtime );
        draw_line = 0;
	sprintf(bufferId,"0x%"FMT64"x\n",localId);

#ifdef DEBUG
        printf("DEBUG : Reading ID from File: %s", FromFile);
        printf("DEBUG : Current Local ID: 0x%"FMT64"x\n", localId);
#endif
	if(strcmp(FromFile,bufferId) == 0)
	{
#ifdef DEBUG
		printf("DEBUG : Session ID remains the same. No vMotion activity has taken place.\n");
#endif
#ifdef OEM
		printf("em_result=0\nem_message=No vMotion activity detected.\n");
#endif
	}
	//else printf("em_result=1\n");
	else
        {
	/* Save results found for later fact-finding mission needs */
#ifdef _WIN32
        save_output = fopen("C:\\Temp\\vm_vmotion_log.txt","a+");
#else
        save_output = fopen("/var/tmp/vm_vmotion_log.txt","a+");
#endif
        fprintf(save_output, "=============================================================================\n", draw_line);
        fprintf(save_output, "Session Reading Time: %s", ctime (&rawtime) );
        fprintf(save_output, "Reading Session ID from File: %s", FromFile);
        fprintf(save_output, "Current session ID is 0x%"FMT64"x\n", localId);
        fprintf(save_output, "WARNING! Session ID has CHANGED! vMotion activity has been detected!\n", draw_line);
        fprintf(save_output, "Updating Session ID file with newly discovered Session ID.\n", draw_line);
        fprintf(save_output, "=============================================================================\n\n", draw_line);
        fclose(save_output);

#ifdef DEBUG
		printf("DEBUG : Session ID has CHANGED! vMotion activity has been detected!\n");
		printf("DEBUG : Updating Session ID file with newly discovered Session ID.\n");
#endif
#ifdef OEM
		printf("em_result=1\nem_message=WARNING! vMotion detected!\n");
#endif

                /* Update Session ID file with new discovered Session ID */
#ifdef DEBUG
                printf("DEBUG : Updating Session ID file with newly discovered Session ID.\n");
#endif
#ifdef _WIN32
                update_file = fopen("C:\\Temp\\vm_session_id.txt","w+");
#else
                update_file = fopen("/var/tmp/vm_session_id.txt","w+");
#endif
                fprintf(update_file, "0x%"FMT64"x\n",localId);
                fclose(update_file);
	}
}

Bool
VMSessionIDCheck(void)
{
   VMGuestLibError glError;
   Bool success = TRUE;
   VMSessionId sessionId = 0;
   char resourcePoolPath[512];
   size_t poolBufSize;
   char data[256];

   FILE *read_file, *initial_file, *copy_file;

   /* Try to load the library. */
   glError = GuestLib_OpenHandle(&glHandle);
   if (glError != VMGUESTLIB_ERROR_SUCCESS) {
      printf("OpenHandle failed: %s\n", GuestLib_GetErrorText(glError));
      return FALSE;
   }

   /* Attempt to retrieve info from the host. */
   while (!done) {
      VMSessionId tmpSession;

      glError = GuestLib_UpdateInfo(glHandle);
      if (glError != VMGUESTLIB_ERROR_SUCCESS) {
         printf("UpdateInfo failed: %s\n", GuestLib_GetErrorText(glError));
         success = FALSE;
         goto out;
      }

      /* Retrieve and check the session ID */
      glError = GuestLib_GetSessionId(glHandle, &tmpSession);
      if (glError != VMGUESTLIB_ERROR_SUCCESS) {
         printf("Failed to get session ID: %s\n", GuestLib_GetErrorText(glError));
         success = FALSE;
         goto out;
      }

      if (tmpSession == 0) {
         printf("Error: Got zero sessionId from GuestLib\n");
         success = FALSE;
         goto out;
      }
      if (sessionId == 0) {
         sessionId = tmpSession;
#ifdef DEBUG
         printf("DEBUG : Initial session ID is 0x%"FMT64"x\n", sessionId);
#endif
      } else if (tmpSession != sessionId) {
         sessionId = tmpSession;
#ifdef DEBUG
         printf("DEBUG : SESSION CHANGED: New session ID is 0x%"FMT64"x\n", sessionId);
#endif
      }

      /* Check to see if Session ID file exists first */
#ifdef _WIN32
      if ( (read_file = fopen ( "C:\\Temp\\vm_session_id.txt", "r" ) ) == NULL )
      {
#else
      if ( (read_file = fopen ( "/var/tmp/vm_session_id.txt", "r" ) ) == NULL )
      {
#endif
#ifdef DEBUG
      printf ( "DEBUG : VM Guest Session ID non-existant. Creating new one now...\n" );
#endif
#ifdef _WIN32
      initial_file = fopen("C:\\Temp\\vm_session_id.txt","w");
#else
      initial_file = fopen("/var/tmp/vm_session_id.txt","w");
#endif
      fprintf(initial_file, "0x%"FMT64"x\n", sessionId);
      fclose(initial_file);
#ifdef OEM
      printf("em_result=0\n");
#endif
      }
      else
      {
#ifdef DEBUG
      printf ( "DEBUG : VM Guest Session ID file exists. Running comparison...\n" );
#endif

      fgets(data, 80, read_file);
      getSessionAlert(data, sessionId);
      fclose ( read_file );
      }

      done = TRUE;

   }

  out:
   glError = GuestLib_CloseHandle(glHandle);
   if (glError != VMGUESTLIB_ERROR_SUCCESS) {
      printf("Failed to CloseHandle: %s\n", GuestLib_GetErrorText(glError));
      success = FALSE;
   }

   return success;
}


int
main(int argc, char *argv[])
{
   /* Try to load the library. */
   if (!LoadFunctions()) {
      printf("GuestLibTest: Failed to load shared library\n");
      exit(1);
   }

   /* Test the VMware Guest API itself. */
   if (!VMSessionIDCheck()) {
      printf("VMSessionIDCheck: GuestLib testing failed\n");
      exit(1);
   }

#ifdef _WIN32
   if (!FreeLibrary(dlHandle)) {
      DWORD error = GetLastError();
      printf("Failed to FreeLibrary: %d\n", error);
      exit(1);
   }
#else
   if (dlclose(dlHandle)) {
      printf("dlclose failed\n");
      exit(1);
   }
#endif

#ifdef DEBUG
   printf("DEBUG : Success!\n");
#endif
   exit(0);
}
