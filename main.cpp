#define WINVER 0x0600
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"

//20181012 1.5Сʱ �������
//20181013 2Сʱ ���ֲ��Ų������
//20181013 2Сʱ �����ʲ������ kʱ�����
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
	HWND lrc;
	HDC lrcdc;
	HFONT lrcfont;
	int duration;
	char file[MAX_PATH];
	char lrcfile[MAX_PATH];
	WNDPROC listproc;
}ui;

struct{
	int s[100];//��ʼʱ��
	int e[100];//����ʱ��
	int index[100];//����
	char lrc[256];
	char dst[256];
	int si,ei,syls,sel;//��ǰλ��
	int doindex;
}ktime;

char buf[256];
HINSTANCE hInst;
MCIGLOBAL mci;
BOOL CALLBACK DlgAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ListInsert(char *lrc,int n);

void OpenLRC(HWND hwndDlg){
	OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = ui.lrcfile;//ȫ·��
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.lrcfile);
    ofn.lpstrFilter = TEXT("����ļ�(*.txt)\0*.txt\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    //GetOpenFileName(&ofn);
    if(GetOpenFileName(&ofn)&&ui.lrcfile[0]){
		FILE *fp;
		char buf[256],*p;
		int n=0;
		fp=fopen(ui.lrcfile,"r");
		if(!fp){
			MessageBox(hwndDlg,"���ļ�ʧ�ܣ�","����",MB_ICONSTOP);
			return;
		}
		while(!feof(fp)){
			fgets(buf,256,fp);
			//Ԥ����
			for(p=buf;*p;p++){
				if(*p=='\n')*p=0;
			}
			ListInsert(buf,n++);
		}
    }
}

void OpenMusic(HWND hwndDlg){
	OPENFILENAME ofn;
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = ui.file;//ȫ·��
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.file);
    ofn.lpstrFilter = TEXT("��Ƶ�ļ�(*.mp3;*.aac;*.wav)\0*.mp3;*.aac;*.wav\0�����ļ�\0*.*\0");
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

void IndexLRC(){
	int i=0;
	char *p=ktime.lrc;
	//�������ĸ ��Ϊ��ʼ
	while(*p){
		ktime.index[i++]=p-ktime.lrc;//��¼λ��
		if(*p>0){//����Ӣ���ҽ�β
			while(*p>0){
				p++;
				//if(*p==' '||*p==','||*p=='.'){//�����β��+1
				if(*p==' '){//�����β��+1
					p++;
					break;
				}
			}
		}else{//����+2
			p+=2;
		}
		if(*p=='"'||*p==' '||*p=='('||*p==')')p++;//���Եķ���
	}
	ktime.index[i]=p-ktime.lrc;
	ktime.syls=i;
}

BOOL CALLBACK DlgList(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
		case 0x10C1:
			//��ȡ��ǰѡ����
			ktime.sel=ListView_GetNextItem(ui.lrclist,-1,LVIS_SELECTED);
			if(ktime.sel>=0&&ktime.doindex){
				//��ʼ�� �������
				ktime.si=ktime.ei=0;
				ListView_GetItemText(ui.lrclist,ktime.sel,2,ktime.lrc,256);
				IndexLRC();
				SetWindowText(ui.lrc,ktime.lrc);
				//printf("���ȣ�%d\n",ktime.syls);
			}
			//
			ktime.doindex^=1;
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			//����Ƿ����ڲ���
			mci.StatusParms.dwItem=MCI_STATUS_MODE;
			mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
			//���ȷʵ���ڲ��� �������� ����Ƿ��Ѿ�kֵ
			if(mci.StatusParms.dwReturn==MCI_MODE_PLAY&&lParam<0x1000000&&ktime.lrc[0]!='{'){
				//��ȡλ��
				mci.StatusParms.dwItem=MCI_STATUS_POSITION;
				mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&(mci.StatusParms));
				//����ǰ������� ��¼Ϊ��ʼʱ��
				if(uMsg==WM_KEYDOWN){
					ktime.s[ktime.si++]=mci.StatusParms.dwReturn;
					//strncpy(buf,ktime.lrc,ktime.index[ktime.si]);
					//buf[ktime.index[ktime.si]]=0;
					//SetWindowText(ui.lrc,buf);
					//��ʾ����
					TextOut(ui.lrcdc,0,0,ktime.lrc,ktime.index[ktime.si]);
				}
				//�����¼Ϊ����ʱ��
				else{
					ktime.e[ktime.ei++]=mci.StatusParms.dwReturn;
					//���һ�仰�����
					if(ktime.ei>=ktime.syls){
						//д������
						char *pd=ktime.dst;
						int lend=mci.StatusParms.dwReturn;//ʱ��ת���������
						ktime.s[ktime.si]=lend;
						for(int i=0,j=0;i<ktime.syls;i++){
							//дkʱ��
							sprintf(buf,"{\\k%d}",(ktime.s[i+1]-ktime.s[i])/10);
							//puts(buf);
							for(j=0;buf[j];j++){
								*pd++=buf[j];
							}
							//д���
							for(j=ktime.index[i];j<ktime.index[i+1];j++){
								*pd++=ktime.lrc[j];
							}
						}
						*pd=0;
						//puts(ktime.dst);
						//����д���б�
						ListView_SetItemText(ui.lrclist,ktime.sel,2,ktime.dst);
						sprintf(buf,"%02d:%02d.%02d",ktime.s[0]/60000,ktime.s[0]%60000/1000,ktime.s[0]%1000/10);
						ListView_SetItemText(ui.lrclist,ktime.sel,0,buf);
						sprintf(buf,"%02d:%02d.%02d",lend/60000,lend%60000/1000,lend%1000/10);
						ListView_SetItemText(ui.lrclist,ktime.sel,1,buf);
						/*for(int i=0;i<ktime.syls;i++){
							printf("(%d,%d)",ktime.s[i]/10,ktime.e[i]/10);
						}
						printf("\n\n");*/
						//ListView_SetHotItem(ui.lrclist,ktime.sel+1);
						//ת����һ��
						SendMessage(ui.lrclist,WM_KEYDOWN,VK_DOWN,0);
					}
				}
			}
			break;
			/*printf("%d>>",ListView_GetNextItem(ui.lrclist,-1,LVIS_SELECTED));
			//ListView_GetNextItem()
			mci.StatusParms.dwItem=MCI_STATUS_MODE;
			mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
			if(mci.StatusParms.dwReturn==MCI_MODE_PLAY){
				mci.StatusParms.dwItem=MCI_STATUS_POSITION;
				mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&(mci.StatusParms));
				printf("%02d:%02d.%03d\n",sec/60,sec%60,mci.StatusParms.dwReturn%1000);
			}*/
			break;
	}
	return CallWindowProc(ui.listproc,hwndDlg,uMsg,wParam,lParam);
}

void ListInit(HWND hlist){
	LV_COLUMN lvc;
	lvc.mask=LVCF_TEXT|LVCF_WIDTH;
	#define _col(s,w,n) lvc.pszText=s;lvc.cx=w;SendMessage(hlist,LVM_INSERTCOLUMN,n,(LPARAM)&lvc);
	_col("��ʼʱ��",72,0);
	_col("����ʱ��",72,1);
	_col("����"    ,420,2);
}

void ListInsert(char *lrc,int n){
	LV_ITEM lvi;
	lvi.mask=LVIF_TEXT;
	lvi.cchTextMax=256;
	lvi.iItem=n;
	lvi.iSubItem=0;
	lvi.pszText="00:00.00";
	SendMessage(ui.lrclist,LVM_INSERTITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=1;
	SendMessage(ui.lrclist,LVM_SETITEM,0,(LPARAM)&lvi);
	lvi.iSubItem=2;
	lvi.pszText=lrc;
	SendMessage(ui.lrclist,LVM_SETITEM,0,(LPARAM)&lvi);
}

/*void ImportLRC(HWND hwndDlg,char *file){
	FILE *fp;
	char buf[256],*p;
	int n=0;
	fp=fopen(file,"r");
	if(!fp){
		MessageBox(hwndDlg,"���ļ�ʧ�ܣ�","����",MB_ICONSTOP);
		return;
	}
	while(!feof(fp)){
		fgets(buf,256,fp);
		//Ԥ����
		for(p=buf;*p;p++){
			if(*p=='\n')*p=0;
		}
		ListInsert(buf,n++);
	}
}*/

void SaveLRC(HWND hwndDlg){
	int i,n;
	FILE *fp;
	OPENFILENAME ofn;
	char filename[MAX_PATH],time1[16],time2[16],lrc[256];
    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;//ȫ·��
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = TEXT("�ı��ļ�(*.txt)\0*.txt\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_EXPLORER;
    if(GetSaveFileName(&ofn)&&filename[0]){
    	fp=fopen(filename,"w");
    	n=ListView_GetItemCount(ui.lrclist);
    	for(i=0;i<n;i++){
			ListView_GetItemText(ui.lrclist,i,0,time1,16);
			ListView_GetItemText(ui.lrclist,i,1,time2,16);
			ListView_GetItemText(ui.lrclist,i,2,lrc,256);
			fprintf(fp,"Dialogue: 0,%s,%s,Default,,0,0,0,,%s\n",time1,time2,lrc);
		}
		fclose(fp);
    }
	
}

void _ListInsert(HWND hlist){
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
	lvi.pszText="����";
	SendMessage(hlist,LVM_SETITEM,0,(LPARAM)&lvi);
	
	lvi.iItem=1;
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
    	ui.lrc     =GetDlgItem(hwndDlg,IDC_LRC);
    	ui.lrcfont =CreateFont(24,0,0,0,0,0,0,0,GB2312_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"΢���ź�");
		ui.lrcdc   =GetDC(ui.lrc);
		//ui.rich    =GetDlgItem(hwndDlg,IDC_RICHEDIT1);
		SelectObject(ui.lrcdc,ui.lrcfont);
		SetTextColor(ui.lrcdc,RGB(255,95,42));
		SetBkMode   (ui.lrcdc,TRANSPARENT);
    	ui.listproc=(WNDPROC)GetWindowLong(ui.lrclist,GWL_WNDPROC);
    	ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg,IDC_LIST1),LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    	SetWindowLong(ui.lrclist,GWL_WNDPROC,(LONG)DlgList);
    	SendMessage(ui.lrc,WM_SETFONT,(WPARAM)ui.lrcfont,0);
    	ListInit(ui.lrclist);
    	//ListInsert(GetDlgItem(hwndDlg,IDC_LIST1));
    	SetTimer(hwndDlg,114,500,TimerProc);
    	//SetTextColor(GetDC(ui.lrc2),RGB(0,255,255));
    }
    return TRUE;

    case WM_CLOSE:
    {
    	mciSendCommand(mci.open.wDeviceID,MCI_CLOSE,0,0);
        EndDialog(hwndDlg, 0);
    }
    return TRUE;
    
	case WM_HSCROLL:
	{
		switch(LOWORD(wParam)){
		case SB_ENDSCROLL:
			mci.play.dwFrom=SendMessage(ui.timeline,TBM_GETPOS,0,0)*1000;
			printf("%d\n",mci.play.dwFrom);
			mciSendCommand(mci.open.wDeviceID,MCI_PLAY,MCI_FROM,(DWORD)&mci.play);
			break;
		}
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
			case IDM_SAVE:
				SaveLRC(hwndDlg);
				break;
			case IDM_INFO:
				DialogBox(hInst,MAKEINTRESOURCE(IDD_DIALOG1),hwndDlg,(DLGPROC)DlgAbout);
				break;
			/*case BTN_ADD:{
				char buf[256];
				ListView_GetItemText(ui.lrclist,0,2,buf,256);
				puts(buf);
			}
			break;*/
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
				mciSendCommand(mci.open.wDeviceID,MCI_STOP,0,0);
				break;
			default:
				break;
        }
    }
    return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK DlgAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
    	
    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		default:;
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
