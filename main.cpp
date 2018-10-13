#define WINVER 0x0600
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"

typedef struct{
	MCI_OPEN_PARMS open;
	MCI_PLAY_PARMS play;
	MCI_STATUS_PARMS StatusParms;
	UCHAR playing;
}MCIGLOBAL;

struct{
	HWND timeline;
	HWND lrclist;
}ui;

HINSTANCE hInst;
MCIGLOBAL mci;
char szFile[MAX_PATH];

void OpenMusic(HWND hwndDlg){
	OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;//全路径
    ofn.lpstrFile[0] = TEXT('\0');
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = TEXT("音频文件(*.mp3;*.aac)\0*.mp3;*.aac\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_EXPLORER |OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    GetOpenFileName (&ofn) ;
    if(szFile[0])
    {
        SetDlgItemText(hwndDlg,EDIT_FILE,szFile);
    }
}

void MCIOpenMusic(MCIGLOBAL *mci){
	mci->open.lpstrDeviceType=0;
	mci->open.lpstrElementName=szFile;
	mciSendCommand(0,MCI_OPEN,MCI_OPEN_ELEMENT,(DWORD)&mci->open);
}

void PlayMusic(MCIGLOBAL *mci,int from){
	//mci->play.dwFrom=from;
	//mciSendCommand(mci->open.wDeviceID,MCI_PLAY,MCI_FROM,(DWORD)&mci->play);
	mciSendCommand(mci->open.wDeviceID,MCI_PLAY,0,(DWORD)&mci->play);
}

void CALLBACK TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime){
	static int t=0;
	
	//获取音乐时间
	mci.StatusParms.dwItem=MCI_STATUS_POSITION;
	mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&(mci.StatusParms));
	//
	printf("%d\n",mci.StatusParms.dwReturn);
}

void ListInit(HWND hlist){
	LV_COLUMN lvc;
	lvc.mask=LVCF_TEXT|LVCF_WIDTH;
	#define _col(s,w,n) lvc.pszText=s;lvc.cx=w;SendMessage(hlist,LVM_INSERTCOLUMN,n,(LPARAM)&lvc);
	_col("开始时间",120,0);
	_col("结束时间",120,1);
	_col("内容"    ,360,2);
}

void ListInsert(HWND hlist){
	LV_ITEM lvi;
	ZeroMemory(&lvi,sizeof(lvi));
	lvi.mask=LVIF_TEXT;
	lvi.cchTextMax=256;
	lvi.iItem=0;
	lvi.iSubItem=0;
	lvi.pszText="00";
	SendMessage(hlist,LVM_INSERTITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=1;
	lvi.pszText="01";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=2;
	lvi.pszText="哈哈";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	
	lvi.iItem=1;
	lvi.iSubItem=0;
	lvi.pszText="01";
	SendMessage(hlist,LVM_INSERTITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=1;
	lvi.pszText="02";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=2;
	lvi.pszText="哈哈";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
}

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
    	//DWORD dwStyle=GetWindowLong(GetDlgItem(hwndDlg,IDC_LIST1),GWL_STYLE);
    	//SetWindowLong(GetDlgItem(hwndDlg,IDC_LIST1),GWL_STYLE,dwStyle|LVS_EX_FULLROWSELECT);
    	//SendDlgItemMessage(hwndDlg,IDC_LIST1,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);
    	ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg,IDC_LIST1),LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    	mci.playing=0;
    	SetTimer(hwndDlg,114,500,TimerProc);
    	ListInit(GetDlgItem(hwndDlg,IDC_LIST1));
    	ListInsert(GetDlgItem(hwndDlg,IDC_LIST1));
    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;
    
    case WM_KEYDOWN:
	{
		printf("%d\n",lParam);
	}
	return TRUE;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        	case IDM_MUSIC:
        		OpenMusic(hwndDlg);
        		MCIOpenMusic(&mci);
				break;
			case BTN_PLAY:
				if(mci.playing){
					mciSendCommand(mci.open.wDeviceID,MCI_PAUSE,0,(DWORD)&mci.play);
					mci.playing=0;
				}else{
					PlayMusic(&mci,mci.play.dwFrom);
					mci.playing=1;
				}
				break;
        }
    }
    return TRUE;
    }
    return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hInst=hInstance;
    InitCommonControls();
    return DialogBox(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)DlgMain);
}
