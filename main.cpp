#define WINVER 0x0600
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"

//20181012 1.5Сʱ �������
//20181013 2Сʱ ���ֲ��Ų������
typedef struct{
	MCI_OPEN_PARMS open;
	MCI_PLAY_PARMS play;
	MCI_STATUS_PARMS StatusParms;
	UCHAR playing;
}MCIGLOBAL;

struct{
	HWND timeline;
	HWND lrclist;
	HWND time;
	int duration;
	char file[MAX_PATH];
	char lrc[MAX_PATH];
}ui;

HINSTANCE hInst;
MCIGLOBAL mci;

void OpenLRC(HWND hwndDlg){
	OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = ui.lrc;//ȫ·��
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.lrc);
    ofn.lpstrFilter = TEXT("����ļ�(*.txt)\0*.txt\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    GetOpenFileName(&ofn);
}

void OpenMusic(HWND hwndDlg){
	OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = ui.file;//ȫ·��
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.file);
    ofn.lpstrFilter = TEXT("��Ƶ�ļ�(*.mp3;*.aac)\0*.mp3;*.aac\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_EXPLORER |OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    GetOpenFileName(&ofn);
    if(ui.file[0])
    {
        SetDlgItemText(hwndDlg,EDIT_FILE,ui.file);
    }
}

void MCIOpenMusic(MCIGLOBAL *mci){
	mci->open.lpstrDeviceType=0;
	mci->open.lpstrElementName=ui.file;
	mciSendCommand(0,MCI_OPEN,MCI_OPEN_ELEMENT,(DWORD)&mci->open);
}

void CALLBACK TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime){
	static int t=0;
	static char buf[32];
	//��ȡ����ʱ��
	mci.StatusParms.dwItem=MCI_STATUS_POSITION;
	mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&(mci.StatusParms));
	//
	#define sec (mci.StatusParms.dwReturn/1000)
	//printf("%d\n",mci.StatusParms.dwReturn);
	SendMessage(ui.timeline,TBM_SETPOS,1,sec);
	sprintf(buf,"%02d:%02d/%02d:%02d\n",sec/60,sec%60,ui.duration/60000,ui.duration/1000%60);
	SetWindowText(ui.time,buf);
}

void ListInit(HWND hlist){
	LV_COLUMN lvc;
	lvc.mask=LVCF_TEXT|LVCF_WIDTH;
	#define _col(s,w,n) lvc.pszText=s;lvc.cx=w;SendMessage(hlist,LVM_INSERTCOLUMN,n,(LPARAM)&lvc);
	_col("��ʼʱ��",120,0);
	_col("����ʱ��",120,1);
	_col("����"    ,360,2);
}

void ListInsert(HWND hlist){
	LV_ITEM lvi;
	ZeroMemory(&lvi,sizeof(lvi));
	lvi.mask=LVIF_TEXT;
	lvi.cchTextMax=256;
	lvi.iItem=100;
	lvi.iSubItem=0;
	lvi.pszText="00";
	SendMessage(hlist,LVM_INSERTITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=1;
	lvi.pszText="01";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=2;
	lvi.pszText="����";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	
	lvi.iItem=100;
	lvi.iSubItem=0;
	lvi.pszText="01";
	SendMessage(hlist,LVM_INSERTITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=1;
	lvi.pszText="02";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=2;
	lvi.pszText="����";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
}

BOOL CALLBACK DlgMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
    	//��ȡ�ؼ�
    	ui.lrclist =GetDlgItem(hwndDlg,IDC_LIST1);
    	ui.timeline=GetDlgItem(hwndDlg,SLIDER_MAIN);
    	ui.time    =GetDlgItem(hwndDlg,IDC_TIME);
    	
    	ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg,IDC_LIST1),LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    	ListInit(ui.lrclist);
    	//ListInsert(GetDlgItem(hwndDlg,IDC_LIST1));
    	SetTimer(hwndDlg,114,500,TimerProc);
    }
    return TRUE;

    case WM_CLOSE:
    {
    	mciSendCommand(mci.open.wDeviceID,MCI_CLOSE,0,0);
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
			case IDM_OPENLRC:
				OpenLRC(hwndDlg);
				break;
        	case IDM_MUSIC:
        		//ѡ���ļ�
        		OpenMusic(hwndDlg);
        		//������
        		MCIOpenMusic(&mci);
        		//��ȡ���ֳ���
        		mci.StatusParms.dwItem=MCI_STATUS_LENGTH;
        		mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
        		ui.duration=mci.StatusParms.dwReturn;
        		//���û���
        		SendMessage(ui.timeline,TBM_SETRANGE,1,(LPARAM)MAKELONG(0,ui.duration/1000));
				break;
			case BTN_PLAY:
				//��ѯ��ǰ״̬
				mci.StatusParms.dwItem=MCI_STATUS_MODE;
				mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
				if(mci.StatusParms.dwReturn==MCI_MODE_PLAY){
					mciSendCommand(mci.open.wDeviceID,MCI_PAUSE,0,(DWORD)&mci.play);
					SetDlgItemText(hwndDlg,BTN_PLAY,">");
				}else{
					mciSendCommand(mci.open.wDeviceID,MCI_PLAY,0,(DWORD)&mci.play);
					SetDlgItemText(hwndDlg,BTN_PLAY,"||");
				}
				break;
			case BTN_STOP:
				mciSendCommand(mci.open.wDeviceID,MCI_STOP,0,(DWORD)&mci.play);
				break;
			default:
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
