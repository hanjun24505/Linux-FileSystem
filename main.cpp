#include "FileSystem.h"
using namespace std;

//全局变量定义,各区域起始地址
const int Superblock_StartAddr = 0;																//超级块 偏移地址,占一个磁盘块
const int InodeBitmap_StartAddr = 1*BLOCK_SIZE;													//inode位图 偏移地址，占两个磁盘块，最多监控 1024 个inode的状态
const int BlockBitmap_StartAddr = InodeBitmap_StartAddr + 2*BLOCK_SIZE;							//block位图 偏移地址，占二十个磁盘块，最多监控 10240 个磁盘块（5120KB）的状态
const int Inode_StartAddr = BlockBitmap_StartAddr + 20*BLOCK_SIZE;								//inode节点区 偏移地址，占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块，640/(512/128)=160
const int Block_StartAddr = Inode_StartAddr + INODE_NUM/(BLOCK_SIZE/INODE_SIZE) * BLOCK_SIZE;	//block数据区 偏移地址 ，占 BLOCK_NUM 个磁盘块,10240*512B=5120KB=5MB

const int Sum_Size = Block_StartAddr + BLOCK_NUM * BLOCK_SIZE;									//虚拟磁盘文件总大小

//单个文件最大大小
const int File_Max_Size =	10*BLOCK_SIZE +														//10个直接块
                             BLOCK_SIZE/sizeof(int) * BLOCK_SIZE +								//一级间接块
                             (BLOCK_SIZE/sizeof(int))*(BLOCK_SIZE/sizeof(int)) * BLOCK_SIZE;		//二级间接块

int Root_Dir_Addr;							//根目录inode地址
int Cur_Dir_Addr;							//当前目录
int Tree_Dir_Addr;
int Cur_Dir_Addr1;
char Cur_Dir_Name[310];						//当前目录名
char Cur_Host_Name[110];					//当前主机名
char Cur_User_Name[110];					//当前登陆用户名
char Cur_Group_Name[110];					//当前登陆用户组名
char Cur_User_Dir_Name[310];				//当前登陆用户目录名

int nextUID;								//下一个要分配的用户标识号
int nextGID;								//下一个要分配的用户组标识号

bool isLogin;								//是否有用户登陆标识量

FILE* fw;									//虚拟磁盘文件 写文件指针
FILE* fr;									//虚拟磁盘文件 读文件指针
SuperBlock *superblock = new SuperBlock;	//超级块指针
bool inode_bitmap[INODE_NUM];				//inode位图
bool block_bitmap[BLOCK_NUM];				//磁盘block块位图

char buffer[10000000] = {0};				//10M，缓存整个虚拟磁盘文件，保证Sum_Size<10M


int main()
{
    system("title 文件系统");
    system("color F0");
    //打开虚拟磁盘文件
    if( (fr = fopen(FILESYSNAME,"rb"))==NULL){	//只读打开虚拟磁盘文件（二进制文件）
        //虚拟磁盘文件不存在，创建一个
        fw = fopen(FILESYSNAME,"wb");	//只写打开虚拟磁盘文件（二进制文件）
        if(fw==NULL){
            printf("虚拟磁盘文件打开失败\n");
            return 0;	//打开文件失败
        }
        fr = fopen(FILESYSNAME,"rb");	//创建好，现在就可以打开了

        //初始化变量
        nextUID = 0;
        nextGID = 0;
        isLogin = false;
        strcpy(Cur_User_Name,"root");//todo: 这里会不会有安全隐患
        strcpy(Cur_Group_Name,"root");

        //获取主机名
        memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
        DWORD k= 100;
        GetComputerName(Cur_Host_Name,&k);

        //根目录inode地址 ，当前目录地址和名字
        Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址，inode区起始地址
        Cur_Dir_Addr = Root_Dir_Addr;
        strcpy(Cur_Dir_Name,"/");           //根目录从“/”开始

        printf("文件系统正在格式化……\n");
        if(!Format()){
            printf("文件系统格式化失败\n");
            return 0;
        }
        printf("格式化完成\n");
        printf("按任意键进行第一次登陆\n");
        system("pause");
        system("cls");


        if(!Install()){
            printf("安装文件系统失败\n");
            return 0;
        }
    }
    else{	//虚拟磁盘文件已存在的情况下，要读取.sys文件，加载文件系统
        fread(buffer,Sum_Size,1,fr);//fr = fopen(FILESYSNAME,"rb")时得到的fr指针

        //取出文件内容暂存到缓冲区中，以写方式打开文件之后再写回文件（写方式打开会清空文件）
        fw = fopen(FILESYSNAME,"wb");	//只写打开虚拟磁盘文件（二进制文件）
        if(fw==NULL){
            printf("虚拟磁盘文件打开失败\n");
            return false;	//打开文件失败
        }
        fwrite(buffer,Sum_Size,1,fw);
        //todo: 格式化后面处理
        /* 提示是否要格式化
         * 因为不是第一次登陆，先略去这一步
         * 下面需要手动设置变量
        Ready();
        system("pause");
        system("cls");
        */

        //初始化变量
        nextUID = 0;
        nextGID = 0;
        isLogin = false;
        strcpy(Cur_User_Name,"root");
        strcpy(Cur_Group_Name,"root");

        //获取主机名
        memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
        DWORD k= 100;
        GetComputerName(Cur_Host_Name,&k);

        //根目录inode地址 ，当前目录地址和名字
        Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址
        Cur_Dir_Addr = Root_Dir_Addr;
        strcpy(Cur_Dir_Name,"/");

        if(!Install()){
            printf("安装文件系统失败\n");
            return 0;
        }

    }


    //testPrint();

    //登录
    while(1){
        if(isLogin){	//登陆成功，才能进入shell
            char str[100];
            char *p;
            if( (p = strstr(Cur_Dir_Name,Cur_User_Dir_Name))==NULL)	//没找到
                printf("[%s@%s %s]# ",Cur_Host_Name,Cur_User_Name,Cur_Dir_Name);
            else
                printf("[%s@%s ~%s]# ",Cur_Host_Name,Cur_User_Name,Cur_Dir_Name+strlen(Cur_User_Dir_Name));
            gets(str);
            cmd(str);
        }
        else{
            system("title 文件系统");//设置cmd窗口标题
            system("mode con cols=800 lines=200");
            FILE *FP;
            FP=fopen("dog.txt","r");
            char c;
            while((c=fgetc(FP))!=EOF)
                putchar(c);

            //getchar();
            printf("欢迎来到FileSystem，请先登录\n");
            while(!login());	//登陆
            printf("登陆成功！\n");
            //system("pause");
            system("cls");
        }
    }

    fclose(fw);		//释放文件指针
    fclose(fr);		//释放文件指针

    return 0;
}


