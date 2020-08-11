// SteamParser.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1

// Global Variables:
HINSTANCE hInst;	// current instance
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szApplicationToolTip[MAX_LOADSTRING];	    // the main window class name
BOOL bDisable = FALSE;							// keep application state

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
bool				IsRunning(std::wstring pName);
bool				CheckPid();
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD1(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD2(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD3(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD4(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD5(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	CD6(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	LoginDialog(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	UpdateFinishDialog(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_STEAMPARSER, szWindowClass, MAX_LOADSTRING);
	
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(IsRunning(L"SteamParser.exe"))
	{
		MessageBox(nidApp.hWnd, L"You cannot run second instance", L"Error", 0);
		exit(0);
	}
	else if(CheckPid())
	{
		MessageBox(nidApp.hWnd, L"You cannot run second instance", L"Error", 0);
		exit(0);
	}

	std::ofstream fp("sp.pid");
	std::stringstream os;
	os << GetCurrentProcessId();
	fp.write(os.str().c_str(), os.str().size());
	fp.flush();
	fp.close();

	//Component initialization
	new EventMgr();
	new Component();
	/*if(sComponent.CheckUpdate(hInst, nidApp.hWnd))
	{
		DialogBox(hInst, MAKEINTRESOURCE(IDD_UPDATE_FINISH), nidApp.hWnd, UpdateFinishDialog);
		sComponent.Dispose();
		delete EventMgr::getSingletonPtr();
		delete Component::getSingletonPtr();
		DeleteFile(L"sp.pid");
		registry_int<int> val(L"Software\\Sombrenuit\\Steam Parser\\Running", HKEY_LOCAL_MACHINE);
		val = 0;
		system("start Start.exe 1 1");
		exit(0);
	}*/

	if(sComponent.GetPasswordHash() == L"")
	{
		DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), nidApp.hWnd, LoginDialog);
	}

	//First run config
	registry_int<int> fi(L"Software\\Sombrenuit\\Steam Parser\\FirstInstall", HKEY_LOCAL_MACHINE);
	if(fi == 1)
	{
		fi = 0;
		DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_1), nidApp.hWnd, CD1);
	}

	//Install file watcher driver
	HANDLE driver = ExtractAndInstallDrv();

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_STEAMPARSER));

#define MAKE_TASK(sp, ptr) tl.AddTask(new Task(new CallbackP0<sp>(sp::getSingletonPtr(), &sp::ptr)))
	TaskList tl;
	MAKE_TASK(Component, Update);
	MAKE_TASK(Component, UpdateFile);
	tl.spawn();
#undef MAKE_TASK
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	sComponent.Dispose();

	// wait for the events to complete.
	tl.wait();

	// wait for them to exit, now.
	tl.kill();
	tl.waitForThreadsToExit();

	delete EventMgr::getSingletonPtr();
	//ShellExecute(GetDesktopWindow(), "open", "SteamParser.exe", NULL, NULL, SW_SHOWNORMAL);
	delete Component::getSingletonPtr();

	DeleteFile(L"sp.pid");
	registry_int<int> val(L"Software\\Sombrenuit\\Steam Parser\\Running", HKEY_LOCAL_MACHINE);
	val = 0;

	StopAndUninstallDrv(driver);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STEAMPARSER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_STEAMPARSER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	HICON hMainIcon;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	hMainIcon = LoadIcon(hInstance,(LPCTSTR)MAKEINTRESOURCE(IDI_STEAMPARSER)); 

	nidApp.cbSize = sizeof(NOTIFYICONDATA); // sizeof the struct in bytes 
	nidApp.hWnd = (HWND) hWnd;              //handle of the window which will process this app. messages 
	nidApp.uID = IDI_STEAMPARSER;           //ID of the icon that willl appear in the system tray 
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; //ORing of all the flags 
	nidApp.hIcon = hMainIcon; // handle of the Icon to be displayed, obtained from LoadIcon 
	nidApp.uCallbackMessage = WM_USER_SHELLICON; 
	LoadString(hInstance, IDS_APPTOOLTIP,nidApp.szTip,MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp); 

	return TRUE;
}

void Init()
{
	// user defined message that will be sent as the notification message to the Window Procedure 
}

bool IsRunning(std::wstring pName)
{
	unsigned long aProcesses[1024], cbNeeded, cProcesses;
	if(!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return false;

	cProcesses = cbNeeded / sizeof(unsigned long);
	for(unsigned int i = 0; i < cProcesses; i++)
	{
		if(aProcesses[i] == 0)
			continue;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, aProcesses[i]);
		LPWSTR buffer = new WCHAR[50];
		GetModuleBaseName(hProcess, 0, buffer, 50);
		DWORD pid = GetProcessId(hProcess);
		CloseHandle(hProcess);
		if(pName == std::wstring(buffer) 
			&& pid != GetCurrentProcessId())
		{
			delete [] buffer;
			return true;
		}
		delete [] buffer;
	}
	return false;
}

bool CheckPid()
{
	std::ifstream fp("sp.pid");
	registry_int<int> val(L"Software\\Sombrenuit\\Steam Parser\\Running", HKEY_LOCAL_MACHINE);
	if(fp)
	{
		fp.close();

		if(val == 1)
		{
			DeleteFile(L"sp.pid");
			return false;
		}
		else
			return true;
	}
	else if(val == 1)
		return true;

	return false;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
    POINT lpClickPoint;

	switch (message)
	{
	case WM_USER_SHELLICON: 
		// systray msg callback 
		switch(LOWORD(lParam)) 
		{   
			case WM_RBUTTONDOWN: 
				UINT uFlag = MF_BYPOSITION|MF_STRING;
				GetCursorPos(&lpClickPoint);
				hPopMenu = CreatePopupMenu();
				InsertMenu(hPopMenu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDM_ABOUT,_T("About"));
				if ( bDisable == TRUE )
				{
					uFlag |= MF_GRAYED;
				}
				InsertMenu(hPopMenu,0xFFFFFFFF,uFlag,IDM_CONFIGURATIONDIALOG,_T("Configuration Wizard"));
				InsertMenu(hPopMenu,0xFFFFFFFF,uFlag,IDM_LOGIN,_T("Login"));			
				InsertMenu(hPopMenu,0xFFFFFFFF,MF_SEPARATOR,IDM_SEP,_T("SEP"));							
				InsertMenu(hPopMenu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,IDM_EXIT,_T("Exit"));
									
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hPopMenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_BOTTOMALIGN,lpClickPoint.x, lpClickPoint.y,0,hWnd,NULL);
				return TRUE; 

		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_CONFIGURATIONDIALOG:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_1), hWnd, CD1);
				break;
			case IDM_LOGIN:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, LoginDialog);
				break;
			case IDM_EXIT:
				Shell_NotifyIcon(NIM_DELETE,&nidApp);
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
		/*
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;*/
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || (LOWORD(wParam) == IDCANCEL))
		{
			EndDialog(hDlg, LOWORD(wParam));
		}
		if (LOWORD(wParam) == IDOK)
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_2), nidApp.hWnd, CD2);
		}

		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_PATHDIALOG_PATH, sComponent.GetPath().c_str());
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == ID_PATHDIALOG_OK || LOWORD(wParam) == ID_PATHDIALOG_CANCEL)
			EndDialog(hDlg, LOWORD(wParam));
		if (LOWORD(wParam) == ID_PATHDIALOG_OK)
		{
			LPWSTR path = new WCHAR[1500];
			GetDlgItemText(hDlg, IDC_PATHDIALOG_PATH, path, 1500);
			sComponent.AcquirePathLock();
			sComponent.SetPath(path);
			sComponent.UpdateConfig();
			sComponent.ReleasePathLock();
			delete [] path;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_3), nidApp.hWnd, CD3);
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD3(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || (LOWORD(wParam) == IDCANCEL))
		{
			EndDialog(hDlg, LOWORD(wParam));
		}
		if (LOWORD(wParam) == IDOK)
		{
			std::wstring path = sComponent.GetPath();
			std::wstring pathTf = path + L"tf\\cfg\\autoexec.cfg";
			std::wifstream fp(pathTf.c_str());
			bool exists = false;
			if(fp)
			{
				fp.close();
				exists = true;
			}
			if(!exists)
			{
				
				std::wofstream ofp(pathTf.c_str());
				if(ofp)
				{
					std::wstringstream ss;
					ss << L"//Sombrenuit";
					ofp.write(ss.str().c_str(), ss.str().size());
				}
			}
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_4), nidApp.hWnd, CD4);
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD4(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_RADIO1, 1);
		return (INT_PTR)TRUE;
	case WM_NOTIFY:
		{
			if(IsDlgButtonChecked(hDlg, IDC_RADIO1))
			{
				CheckDlgButton(hDlg, IDC_RADIO1, 0);
				CheckDlgButton(hDlg, IDC_RADIO2, 1);
			}
			else if(IsDlgButtonChecked(hDlg, IDC_RADIO2))
			{
				CheckDlgButton(hDlg, IDC_RADIO1, 1);
				CheckDlgButton(hDlg, IDC_RADIO2, 0);
				DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_6), nidApp.hWnd, CD6);
			}
			return (INT_PTR)TRUE;
		}break;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, LOWORD(wParam));
		if (LOWORD(wParam) == IDOK)
		{
			if(IsDlgButtonChecked(hDlg, IDC_RADIO1))
			{
				//Do automatic insert
			}
			else if(IsDlgButtonChecked(hDlg, IDC_RADIO2))
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_5), nidApp.hWnd, CD5);
			}
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD5(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			std::wstringstream ss;
			ss << L"// Sombrenuit Steam Parser\r\n";
			ss << L"con_logfile sp.log \r\n";
			ss << L"con_timestamp 1 \r\n";
			ss << L"// Sombrenuit Steam Parser\r\n";
			SetDlgItemText(hDlg, IDC_EDIT_SAMPLE, ss.str().c_str());

			std::wstring path = sComponent.GetPath();
			std::wstring pathTf = path + L"tf\\cfg\\autoexec.cfg";

			std::ifstream ifp(pathTf.c_str());
			if(ifp)
			{
				std::wstringstream ss;
				while(!ifp.eof())
				{
					std::string lineS;
					std::getline(ifp, lineS);
					std::wstring line = StringUtil::FromUtf8(lineS);
					//line = line.substr(0, line.size() - 2);
					line = StringUtil::FixStringW(line);
					ss << line << L"\r\n";
				}
				SetDlgItemInt(hDlg, IDC_EDIT_LINE_COUNT, ifp.tellg(), true);
				ifp.close();

				SetDlgItemText(hDlg, IDC_EDIT_AUTOEXEC, ss.str().c_str());
			}
			else
			{
				MessageBox(hDlg, L"Cannot open autoexec.cfg, please try to edit it manually", L"Error", 0);
			}
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			EndDialog(hDlg, LOWORD(wParam));
		if (LOWORD(wParam) == IDOK)
		{
			HWND hEdit = GetDlgItem (hDlg, IDC_EDIT_AUTOEXEC);
			int iChars = GetWindowTextLength (hEdit)+1; // Room for '\0'
			WCHAR* pstrText;

			if ((pstrText = (WCHAR*) malloc (sizeof (WCHAR) * iChars)) != NULL) 
			{
				GetWindowText (hEdit, pstrText, iChars);
				
				std::wstring path = sComponent.GetPath();
				std::wstring pathTf = path + L"tf\\cfg\\autoexec.cfg";

				DeleteFile(pathTf.c_str());
				std::wofstream ofp(pathTf.c_str());
				if(ofp)
				{
					ofp.write(pstrText, iChars);
					ofp.flush();
					ofp.close();
				}
				else
				{
					MessageBox(hDlg, L"Cannot open autoexec.cfg, please try to edit it manually", L"Error", 0);
				}

				free (pstrText);
			}
			else
			{
				MessageBox(hDlg, L"Cannot open autoexec.cfg, please try to edit it manually", L"Error", 0);
			}
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIGURATION_DIALOG_6), nidApp.hWnd, CD6);
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK CD6(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK)
			EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK LoginDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			CURL *curl;
			CURLcode res;
			bool error = true;

			curl = curl_easy_init();
			if(curl)
			{
				std::wstring username;
				std::wstring password;

				LPWSTR text = new WCHAR[1500];
				GetDlgItemText(hDlg, IDC_LOGIN_ACCOUNT, text, 1500);
				username = text;
				GetDlgItemText(hDlg, IDC_LOGIN_PASSWORD, text, 1500);
				password = text;
				delete [] text;

				std::string data;
				std::wstringstream urlu;
				std::wstringstream qs;
				time_t tt = time(NULL);
//#define USEPOST
#ifndef USEPOST
				urlu << L"http://dk.tf2tr.com/stat/statlogin.php?";
#else
				curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.17.129/stat/phpbb3/stat/statlogin.php?");
#endif
				qs << L"&u=";
				qs << username;
				qs << L"&p=";
				qs << password;
				qs << L"&ed=";
				qs << tt;
				std::wstringstream urlch;
				urlch << qs.str();
				urlch << L":" << tt << L":";
				std::string md5u = StringUtil::Trim(StringUtil::ToUtf8(urlch.str()));
				std::string md5 = sComponent.GetMD5()->getHashFromString(md5u);
				urlu << qs.str();
				urlu << L"&ru=";
				urlu << StringUtil::FromUtf8(md5);
#ifndef USEPOST
				curl_easy_setopt(curl, CURLOPT_URL, StringUtil::Trim(StringUtil::ToUtf8(urlu.str())).c_str());
#else
				curl_easy_setopt(curl, CURLOPT_POST, 1);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, StringUtil::Trim(StringUtil::ToUtf8(urlu.str())));
#endif
#undef USEPOST

				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Component::write_data);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

				/* Perform the request, res will get the return code */
				res = curl_easy_perform(curl);

				/* always cleanup */
				curl_easy_cleanup(curl);

				if(res == CURLE_OK)
				{
					std::vector<std::string> results;
					StringUtil::SplitString(data, ",", results, false);
					if(results.size() > 1)
					{
						if(results[0] == "1")
						{
							sComponent.SetUsername(username);
							sComponent.SetPassword(password);
							sComponent.SetPasswordHash(StringUtil::FromUtf8(results[1]));
							sComponent.UpdateConfig();

							MessageBox(hDlg, L"Login Ok...", L"Login Result", 0);
							error = false;
						}
						else
							MessageBox(hDlg, L"Login error try again...", L"Login Result", 0);
					}
					else
						MessageBox(hDlg, L"Login error try again...", L"Login Result", 0);
				}
				else
					MessageBox(hDlg, L"Login error try again...", L"Login Result", 0);
			}
			if(!error)
				EndDialog(hDlg, LOWORD(wParam));
		}
		else if(LOWORD(wParam) == IDSET)
		{
			std::wstring username;
			std::wstring password;

			LPWSTR text = new WCHAR[1500];
			GetDlgItemText(hDlg, IDC_LOGIN_ACCOUNT, text, 1500);
			username = text;
			GetDlgItemText(hDlg, IDC_LOGIN_PASSWORD, text, 1500);
			password = text;
			delete [] text;

			sComponent.SetUsername(username);
			sComponent.SetPassword(password);

			EndDialog(hDlg, LOWORD(wParam));
		}
		else if(LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
		}

		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK UpdateFinishDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK)
			EndDialog(hDlg, LOWORD(wParam));
		if(LOWORD(wParam) == IDCANCEL)
		{
			std::string link = "start " + sComponent.GetUpdateLink();
			system(link.c_str());
		}
		return (INT_PTR)TRUE;
		break;
	}
	return (INT_PTR)FALSE;
}
