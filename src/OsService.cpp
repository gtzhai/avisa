#include "OsService.hpp"

namespace hippo
{
OsService* OsService::m_service = nullptr;
#ifdef _WIN32
#include <Windows.h>
#define SERVICE_DEF_START_TYPE (STANDARD_RIGHTS_REQUIRED | SERVICE_CHANGE_CONFIG | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_DEF_ERROR_TYPE (SERVICE_AUTO_START)
bool OsServiceInstaller::install(const OsService& service) {

  SC_HANDLE scm = ::OpenSCManager(nullptr, nullptr,
                                                    SC_MANAGER_CONNECT | 
                                                    SC_MANAGER_CREATE_SERVICE);
  if (!scm) {
    printf(("can not open scm: %d\n"), GetLastError());
    return false;
  }
  
  SC_HANDLE s = ::CreateService(scm,
                                             service.name().c_str(),
                                             service.desc().c_str(),
                                             SERVICE_QUERY_STATUS,
                                             SERVICE_WIN32_OWN_PROCESS,
                                             SERVICE_DEF_START_TYPE,          
                                             SERVICE_DEF_ERROR_TYPE,
                                             __argv[0],
                                             nullptr,
                                             nullptr,
                                             nullptr,
                                             nullptr,
                                             nullptr);
  if (!s) {
    printf("can not create service: %d\n", ::GetLastError());
    CloseServiceHandle(scm)
    return false;
  }

  CloseServiceHandle(s);
  CloseServiceHandle(scm);

  return true;
}

bool OsServiceInstaller::uninstall(const OsService& service) {
  SC_HANDLE scm = ::OpenSCManager(nullptr, nullptr,
                                                    SC_MANAGER_CONNECT);

  if (!scm) {
    printf(("can not open scm: %d\n"), GetLastError());
    return false;
  }

  SC_HANDLE s = ::OpenService(scm, service.name().c_str(),
                                           SERVICE_QUERY_STATUS | 
                                           SERVICE_STOP |
                                           DELETE);

  if (!s) {

    CloseServiceHandle(scm);
    printf("can not open service: %d\n", ::GetLastError());
    return false;
  }

  SERVICE_STATUS status = {};
  if (::ControlService(s, SERVICE_CONTROL_STOP, &status)) {

    while (::QueryServiceStatus(s, &status)) {
      if (status.dwCurrentState != SERVICE_STOP_PENDING) {
        break;
      }
    }

    if (status.dwCurrentState != SERVICE_STOPPED) {
      printf(("stop service failed\n"));
    } else {
      printf(("service stopped\n"));
    }
  } else {
    printf(("control service failed: %d\n"), ::GetLastError());
  }

  if (!::DeleteService(s)) {
    printf(("delete service failed : %d\n"), GetLastError());
    CloseServiceHandle(scm);
    return false;
  }
  CloseServiceHandle(scm);

  return true;
}

SERVICE_STATUS        g_ServiceStatus = { 0 };
/// handle to the status information structure for the current service.
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
/// handle for a stop event
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

OsService::OsService(const CString& name,
                         const CString& displayName,
                         DWORD dwStartType,
                         DWORD dwErrCtrlType,
                         DWORD dwAcceptedCmds,
                         const CString& depends,
                         const CString& account,
                         const CString& password)
 : m_name(name),
   m_displayName(displayName),
   m_dwStartType(dwStartType),
   m_dwErrorCtrlType(dwErrCtrlType),
   m_depends(depends),
   m_account(account),
   m_password(password),
   m_svcStatusHandle(nullptr) {  

}

int OsService::run()
{
    m_service = this;

	/// Structure that specifies the ServiceMain function for a service that can run in the calling process
	/// It is used by the StartServiceCtrlDispatcher function
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ name_.c_str(), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	/// Connects the main thread of a service process to the service control manager, 
	/// which causes the thread to be the service control dispatcher thread for the calling process
	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;
	/// Registers a function to handle service control requests.
	g_StatusHandle = RegisterServiceCtrlHandler(m_service->name().c_str(), ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		return;
	}

	/// Informing the smc that the service is starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	/// Creating the stop event
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	/// if SetServiceStatus returned error
	if (g_ServiceStopEvent == NULL)
	{
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
		return;
	}

	/// Informing the smc that the service started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	/// Starting the  worker thread
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	// Waiting for worker thread exit signal
	WaitForSingleObject(hThread, INFINITE);

	/// Cleanup
	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;
	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

}

/// Controller for the service, managing the stop operation
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
	{
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/// Stopping the service
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;
		SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

		/// Signalling the worker thread to shut down
		SetEvent(g_ServiceStopEvent);

		break;
	}

	default:
	{
		break;
	}
	}

}

/// ServiceWorkerThread function starts writing the timestamp to a file every 3 seconds
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    if(m_service->callback != nullptr){
	    ///  Checking if the services has issued a stop request
	    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	    {
            m_service->callback();
	    }
    }

	return ERROR_SUCCESS;
}
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#define DAEMON_PID_FILE_DIR "/var/run"
void signal_handler_SIGHUP (int signum)
{
	//to do: hook for rereading config (not supported on Windows);
}

void signal_handler_SIGTERM (int signum)
{
    OsService::m_service->abort = 1;
}

bool OsServiceInstaller::install(const OsService& service) {
    return false;
}

bool OsServiceInstaller::uninstall(const OsService& service) {
    return false;
}

int OsService::run(){

    int pid_file;
	char buf[16];
	int bufpos;
    const char *name = name_.c_str();

	pid_t pid;
	//check if pid file exists
	char* pid_filename = (char*)malloc(strlen(DAEMON_PID_FILE_DIR) + strlen(name) + 6);
	strcpy(pid_filename, DAEMON_PID_FILE_DIR);
	strcat(pid_filename, "/");
	strcat(pid_filename, name);
	strcat(pid_filename, ".pid");
	if ((pid_file = open(pid_filename, O_RDONLY)) >= 0) {
		//check if pid file contains a valid process id
		bufpos = read(pid_file, buf, sizeof(buf) - 1);
		close(pid_file);
		if (bufpos > 0) {
			buf[bufpos] = 0;
			bufpos = 0;
			while (isdigit(buf[bufpos]))
				bufpos++;
			buf[bufpos] = 0;
			//exit if another process is already running
			if (kill(pid = atoi(buf), SIGCONT) == 0) {
				free(pid_filename);
				fprintf(stderr, "\nAnother process (%i) is already running, aborting\n", (int)pid);
				return 1;
			}
		}
		//remove stale pid file
		unlink(pid_filename);
	}
	//block all signals and keep old signal mask
	sigset_t signal_mask;
	sigset_t old_signal_mask;
	sigfillset(&signal_mask);
	sigprocmask(SIG_BLOCK, &signal_mask, &old_signal_mask);
	//spawn daemon
	if ((pid = fork()) == -1) {
		free(pid_filename);
		fprintf(stderr, "\nError forking daemon process\n");
		return 1;
	} else if (pid != 0) {
		//daemon spawned: create pid file and exit
		if ((pid_file = open(pid_filename, O_CREAT | O_EXCL | O_WRONLY, 0644)) < 0) {
			fprintf(stderr, "\nError creating pid file: %s\n", pid_filename);
		} else {
			bufpos = sizeof(buf);
			buf[--bufpos] = '\n';
			do {
				buf[--bufpos] = '0' + (char)(pid % 10);
			} while ((pid /= 10) > 0);
			write(pid_file, buf + bufpos, sizeof(buf) - bufpos);
			close(pid_file);
		}
		return 0;
	}
	//restore old signal mask
	sigprocmask(SIG_SETMASK, &old_signal_mask, NULL);
	//become session leader, detach from parent tty
	if (setsid() == -1) {
		fprintf(stderr, "\nError detaching daemon process\n");
		exit(1);
	}

	//install signal handlers
	signal(SIGHUP, signal_handler_SIGHUP);
	signal(SIGTERM, signal_handler_SIGTERM);
	signal(SIGINT, signal_handler_SIGTERM);
	signal(SIGQUIT, signal_handler_SIGTERM);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);

    if(cb_ != nullptr){
	    ///  Checking if the services has issued a stop request
	    while (!abort)
	    {
           cb_();
	    }
    }

	//remove pid file
	unlink(pid_filename);
	free(pid_filename);
	exit(0);
}
#endif

}

