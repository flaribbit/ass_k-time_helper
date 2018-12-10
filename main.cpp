#define WINVER 0x0600
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "resource.h"

//20181012 1.5小时 界面完成
//20181013 2小时 音乐播放部分完成
//20181013 2小时 导入歌词部分完成 k时间完成
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
	int s[100];//开始时间
	int e[100];//结束时间
	int index[100];//索引
	char lrc[256];
	char dst[256];
	int si,ei,syls,sel;//当前位置
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
    ofn.lpstrFile = ui.lrcfile;//全路径
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.lrcfile);
    ofn.lpstrFilter = TEXT("歌词文件(*.txt)\0*.txt\0");
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
			MessageBox(hwndDlg,"打开文件失败！","错误",MB_ICONSTOP);
			return;
		}
		while(!feof(fp)){
			fgets(buf,256,fp);
			//预处理
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
    ofn.lpstrFile = ui.file;//全路径
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(ui.file);
    ofn.lpstrFilter = TEXT("音频文件(*.mp3;*.aac;*.wav)\0*.mp3;*.aac;*.wav\0所有文件\0*.*\0");
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
	//获取音乐时间
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
	//如果是字母 记为开始
	while(*p){
		ktime.index[i++]=p-ktime.lrc;//记录位置
		if(*p>0){//碰到英文找结尾
			while(*p>0){
				p++;
				//if(*p==' '||*p==','||*p=='.'){//此类结尾再+1
				if(*p==' '){//此类结尾再+1
					p++;
					break;
				}
			}
		}else{//中文+2
			p+=2;
		}
		if(*p=='"'||*p==' '||*p=='('||*p==')')p++;//忽略的符号
	}
	ktime.index[i]=p-ktime.lrc;
	ktime.syls=i;
}

BOOL CALLBACK DlgList(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
		case 0x10C1:
			//获取当前选择项
			ktime.sel=ListView_GetNextItem(ui.lrclist,-1,LVIS_SELECTED);
			if(ktime.sel>=0&&ktime.doindex){
				//初始化 索引歌词
				ktime.si=ktime.ei=0;
				ListView_GetItemText(ui.lrclist,ktime.sel,2,ktime.lrc,256);
				IndexLRC();
				SetWindowText(ui.lrc,ktime.lrc);
				//printf("长度：%d\n",ktime.syls);
			}
			//
			ktime.doindex^=1;
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			//检查是否正在播放
			mci.StatusParms.dwItem=MCI_STATUS_MODE;
			mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
			//如果确实正在播放 屏蔽连按 检查是否已经k值
			if(mci.StatusParms.dwReturn==MCI_MODE_PLAY&&lParam<0x1000000&&ktime.lrc[0]!='{'){
				//获取位置
				mci.StatusParms.dwItem=MCI_STATUS_POSITION;
				mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&(mci.StatusParms));
				//如果是按键按下 记录为开始时间
				if(uMsg==WM_KEYDOWN){
					ktime.s[ktime.si++]=mci.StatusParms.dwReturn;
					//strncpy(buf,ktime.lrc,ktime.index[ktime.si]);
					//buf[ktime.index[ktime.si]]=0;
					//SetWindowText(ui.lrc,buf);
					//提示文字
					TextOut(ui.lrcdc,0,0,ktime.lrc,ktime.index[ktime.si]);
				}
				//否则记录为结束时间
				else{
					ktime.e[ktime.ei++]=mci.StatusParms.dwReturn;
					//如果一句话完成了
					if(ktime.ei>=ktime.syls){
						//写入数据
						char *pd=ktime.dst;
						int lend=mci.StatusParms.dwReturn;//时间转储方便调用
						ktime.s[ktime.si]=lend;
						for(int i=0,j=0;i<ktime.syls;i++){
							//写k时间
							sprintf(buf,"{\\k%d}",(ktime.s[i+1]-ktime.s[i])/10);
							//puts(buf);
							for(j=0;buf[j];j++){
								*pd++=buf[j];
							}
							//写歌词
							for(j=ktime.index[i];j<ktime.index[i+1];j++){
								*pd++=ktime.lrc[j];
							}
						}
						*pd=0;
						//puts(ktime.dst);
						//数据写入列表
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
						//转到下一行
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
	_col("开始时间",72,0);
	_col("结束时间",72,1);
	_col("内容"    ,420,2);
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
		MessageBox(hwndDlg,"打开文件失败！","错误",MB_ICONSTOP);
		return;
	}
	while(!feof(fp)){
		fgets(buf,256,fp);
		//预处理
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
    ofn.lpstrFile = filename;//全路径
    ofn.lpstrFile[0] = 0;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = TEXT("文本文件(*.txt)\0*.txt\0");
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
    	//获取控件
    	ui.lrclist =GetDlgItem(hwndDlg,IDC_LIST1);
    	ui.timeline=GetDlgItem(hwndDlg,SLIDER_MAIN);
    	ui.time    =GetDlgItem(hwndDlg,IDC_TIME);
    	ui.lrc     =GetDlgItem(hwndDlg,IDC_LRC);
    	ui.lrcfont =CreateFont(24,0,0,0,0,0,0,0,GB2312_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"微软雅黑");
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
        		//选择文件
        		OpenMusic(hwndDlg);
        		//打开音乐
        		MCIOpenMusic(&mci);
        		//获取音乐长度
        		mci.StatusParms.dwItem=MCI_STATUS_LENGTH;
        		mciSendCommand(mci.open.wDeviceID,MCI_STATUS,MCI_WAIT|MCI_STATUS_ITEM,(DWORD)&mci.StatusParms);
        		ui.duration=mci.StatusParms.dwReturn;
        		//设置滑块
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
				//查询当前状态
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
