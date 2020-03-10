#include "FileSystem.h"
using namespace std;

//����ʵ��
void Ready()	//��¼ϵͳǰ��׼������,������ʼ��+ע��+��װ
{
    //��ʼ������
    nextUID = 0;
    nextGID = 0;
    isLogin = false;
    strcpy(Cur_User_Name,"root");
    strcpy(Cur_Group_Name,"root");

    //��ȡ������
    memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));
    DWORD k= 100;
    GetComputerName(Cur_Host_Name,&k);

    //��Ŀ¼inode��ַ ����ǰĿ¼��ַ������
    Root_Dir_Addr = Inode_StartAddr;	//��һ��inode��ַ
    Cur_Dir_Addr = Root_Dir_Addr;
    strcpy(Cur_Dir_Name,"/");


    char c;
    printf("�Ƿ��ʽ��?[y/n]");
    while(c = getch()){
        fflush(stdin);//���Ͻ������steam������ˢ�µ�ָ�����
        if(c=='y'){
            printf("\n");
            printf("�ļ�ϵͳ���ڸ�ʽ������\n");
            if(!Format()){
                printf("�ļ�ϵͳ��ʽ��ʧ��\n");
                return ;
            }
            printf("��ʽ�����\n");
            break;
        }
        else if(c=='n'){
            printf("\n");
            break;
        }
    }

    printf("�����ļ�ϵͳ����\n");
    if(!Install()){
        printf("��װ�ļ�ϵͳʧ��\n");
        return ;
    }
    printf("�������\n");
}

bool Format()	//��ʽ��һ����������ļ�
{
    int i,j;

    //��ʼ��������
    superblock->s_INODE_NUM = INODE_NUM;                            //640
    superblock->s_BLOCK_NUM = BLOCK_NUM;                            //10240
    superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);             //���㳬����Ĵ�С
    superblock->s_INODE_SIZE = INODE_SIZE;                          //128KB
    superblock->s_BLOCK_SIZE = BLOCK_SIZE;                          //512KB
    superblock->s_free_INODE_NUM = INODE_NUM;                       //����inode 640
    superblock->s_free_BLOCK_NUM = BLOCK_NUM;                       //���д���block 10240
    superblock->s_blocks_per_group = BLOCKS_PER_GROUP;              //���п��ջ��С 64
    superblock->s_free_addr = Block_StartAddr;	                    //���п��ջָ��Ϊ��һ��block��ʼ��ַ
    superblock->s_Superblock_StartAddr = Superblock_StartAddr;      //��������ʼ��ַ
    superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;    //����block��λͼ��ʼ��ַ todo
    superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;    //inode��λͼ��ʼ��ַ
    superblock->s_Block_StartAddr = Block_StartAddr;                //����block����ʼ��ַ
    superblock->s_Inode_StartAddr = Inode_StartAddr;                //inode����ʼ��ַ
    //���п��ջ�ں��渳ֵ

    //��ʼ��inodeλͼ
    memset(inode_bitmap,0,sizeof(inode_bitmap));
    fseek(fw,InodeBitmap_StartAddr,SEEK_SET);                       // ���������ļ�ָ��fw��λ�á�stream��ָ����seek_setΪ��׼��ƫ��InodeBitMap_StartAddr���ֽڵ�λ�á�
    fwrite(inode_bitmap,sizeof(inode_bitmap),1,fw);                 //��inodeλͼд��inodeλͼ��ʼλ�ÿ�ʼ�����������

    //��ʼ��blockλͼ
    memset(block_bitmap,0,sizeof(block_bitmap));                    //һ���ķ�ʽ��ʼ��block���λͼ
    fseek(fw,BlockBitmap_StartAddr,SEEK_SET);
    fwrite(block_bitmap,sizeof(block_bitmap),1,fw);

    //��ʼ�����̿��������ݳ������ӷ���֯
    //10240/64=160 ��160��
    for(i=BLOCK_NUM/BLOCKS_PER_GROUP-1;i>=0;i--){	//һ��INODE_NUM/BLOCKS_PER_GROUP�飬todo һ��FREESTACKNUM��128�� 64? �����̿� ����һ�����̿���Ϊ����
        if(i==BLOCK_NUM/BLOCKS_PER_GROUP-1)
            superblock->s_free[0] = -1;	//û����һ�����п���
        else
            superblock->s_free[0] = Block_StartAddr + (i+1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
        for(j=1;j<BLOCKS_PER_GROUP;j++){
            superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
        }
        fseek(fw,Block_StartAddr+i*BLOCKS_PER_GROUP*BLOCK_SIZE,SEEK_SET);
        fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�
    }
    //������д�뵽��������ļ�
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    fflush(fw);

    //��ȡinodeλͼ
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //��ȡblockλͼ
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    fflush(fr);

    //������Ŀ¼ "/"
    Inode cur;

    //����inode
    int inoAddr = ialloc();

    //�����inode������̿�
    int blockAddr = balloc();

    //��������̿������һ����Ŀ "."
    DirItem dirlist[16] = {0};                //32�ֽڣ�һ�����̿��ܴ� 512/32=16 ��Ŀ¼��
    strcpy(dirlist[0].itemName,".");
    dirlist[0].inodeAddr = inoAddr;

    //д�ش��̿�
    fseek(fw,blockAddr,SEEK_SET);
    fwrite(dirlist,sizeof(dirlist),1,fw);

    //��inode��ֵ
    cur.i_ino = 0;
    cur.i_atime = time(NULL);
    cur.i_ctime = time(NULL);
    cur.i_mtime = time(NULL);
    strcpy(cur.i_uname,Cur_User_Name);
    strcpy(cur.i_gname,Cur_Group_Name);
    cur.i_cnt = 1;	//һ�����ǰĿ¼,"."  //���������ж����ļ���ָ�����inode
    cur.i_dirBlock[0] = blockAddr;
    for(i=1;i<10;i++){
        cur.i_dirBlock[i] = -1;
    }
    cur.i_size = superblock->s_BLOCK_SIZE;
    cur.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
    cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;


    //д��inode
    fseek(fw,inoAddr,SEEK_SET);
    fwrite(&cur,sizeof(Inode),1,fw);
    fflush(fw);

    //����Ŀ¼�������ļ�
    mkdir(Root_Dir_Addr,"home");	//�û�Ŀ¼
    cd(Root_Dir_Addr,"home");
    mkdir(Cur_Dir_Addr,"root");

    cd(Cur_Dir_Addr,"..");
    mkdir(Cur_Dir_Addr,"etc");	//�����ļ�Ŀ¼
    cd(Cur_Dir_Addr,"etc");

    char buf[1000] = {0};

    sprintf(buf,"root:x:%d:%d\n",nextUID++,nextGID++);	//������Ŀ���û������������룺�û�ID���û���ID
    create(Cur_Dir_Addr,"passwd",buf);	//�����û���Ϣ�ļ�

    sprintf(buf,"root:root\n");	//������Ŀ���û���������
    create(Cur_Dir_Addr,"shadow",buf);	//�����û������ļ�
    chmod(Cur_Dir_Addr,"shadow",0660);	//�޸�Ȩ�ޣ���ֹ�����û���ȡ���ļ�

    sprintf(buf,"root::0:root\n");	//���ӹ���Ա�û��飬�û����������һ��Ϊ�գ�����û��ʹ�ã������ʶ�ţ������û��б����ã��ָ���
    sprintf(buf+strlen(buf),"user::1:\n");	//������ͨ�û��飬�����û��б�Ϊ��
    create(Cur_Dir_Addr,"group",buf);	//�����û�����Ϣ�ļ�

    cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼

    return true;
}

bool Install()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
    //��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ
    //��ȡ��Ŀ¼����ȡetcĿ¼����ȡ����ԱadminĿ¼����ȡ�û�Ŀ¼����ȡ�û�passwd�ļ���

    //��ȡ������
    fseek(fr,Superblock_StartAddr,SEEK_SET);
    fread(superblock,sizeof(SuperBlock),1,fr);

    //��ȡinodeλͼ
    fseek(fr,InodeBitmap_StartAddr,SEEK_SET);
    fread(inode_bitmap,sizeof(inode_bitmap),1,fr);

    //��ȡblockλͼ
    fseek(fr,BlockBitmap_StartAddr,SEEK_SET);
    fread(block_bitmap,sizeof(block_bitmap),1,fr);

    return true;
}

void printSuperBlock()		//��ӡ��������Ϣ
{
    printf("\n");
    printf("����inode�� / ��inode�� ��%d / %d\n",superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
    printf("����block�� / ��block�� ��%d / %d\n",superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
    printf("��ϵͳ block��С��%d �ֽڣ�ÿ��inodeռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_INODE_SIZE,sizeof(Inode));
    printf("\tÿ���̿��飨���ж�ջ��������block������%d\n",superblock->s_blocks_per_group);
    printf("\t������ռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_SUPERBLOCK_SIZE);
    printf("���̷ֲ���\n");
    printf("\t�����鿪ʼλ�ã�%d B\n",superblock->s_Superblock_StartAddr);
    printf("\tinodeλͼ��ʼλ�ã�%d B\n",superblock->s_InodeBitmap_StartAddr);
    printf("\tblockλͼ��ʼλ�ã�%d B\n",superblock->s_BlockBitmap_StartAddr);
    printf("\tinode����ʼλ�ã�%d B\n",superblock->s_Inode_StartAddr);
    printf("\tblock����ʼλ�ã�%d B\n",superblock->s_Block_StartAddr);
    printf("\n");

    return ;
}

void printInodeBitmap()	//��ӡinodeʹ�����
{
    printf("\n");
    printf("inodeʹ�ñ���[uesd:%d %d/%d]\n",superblock->s_INODE_NUM-superblock->s_free_INODE_NUM,superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);
    int i;
    i = 0;
    printf("0 ");
    while(i<superblock->s_INODE_NUM){
        if(inode_bitmap[i])
            printf("*");    //inode_bitmap[i]==1��������inode�Ѿ�����
        else
            printf(".");    //inode_bitmap[i]==0��������inode��δ����
        i++;
        if(i!=0 && i%32==0){
            printf("\n");
            if(i!=superblock->s_INODE_NUM)
                printf("%d ",i/32);
        }
    }
    printf("\n");
    printf("\n");
    return ;
}

void printBlockBitmap(int num)	//��ӡblockʹ����������Ը��ݸ���������ʾ��������
{
    printf("\n");
    printf("block�����̿飩ʹ�ñ���[used:%d %d/%d]\n",superblock->s_BLOCK_NUM-superblock->s_free_BLOCK_NUM,superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);
    int i;
    i = 0;
    printf("0 ");
    while(i<num){
        if(block_bitmap[i])
            printf("*");    //block_bitmap[i]==1��������block�Ѿ�����
        else
            printf(".");    //block_bitmap[i]==0��������block��δ����
        i++;
        if(i!=0 && i%32==0){
            printf("\n");
            if(num==superblock->s_BLOCK_NUM)
                getchar();
            if(i!=superblock->s_BLOCK_NUM)
                printf("%d ",i/32);
        }
    }
    printf("\n");
    printf("\n");
    return ;
}

int balloc()	//���̿���亯�����������ӷ�����block��
{
    //ʹ�ó������еĿ��п��ջ
    //���㵱ǰջ��
    int top;	//ջ��ָ��
    if(superblock->s_free_BLOCK_NUM==0){	//ʣ����п���Ϊ0
        printf("û�п��п���Է���\n");
        return -1;	//û�пɷ���Ŀ��п飬����-1
    }
    else{	//����ʣ���
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;
    }
    //��ջ��ȡ��
    //�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
    int retAddr;

    if(top==0){
        retAddr = superblock->s_free_addr;
        superblock->s_free_addr = superblock->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��

        //ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ

        //ȡ����һ�����п��ջ������ԭ����
        fseek(fr,superblock->s_free_addr,SEEK_SET);     //��superblock->s_free[0]ÿ��������Ϊ����
        fread(superblock->s_free,sizeof(superblock->s_free),1,fr);
        fflush(fr);

        superblock->s_free_BLOCK_NUM--;

    }
    else{	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
        retAddr = superblock->s_free[top];	//���淵�ص�ַ
        superblock->s_free[top] = -1;	//��ջ��
        top--;		//ջ��ָ��-1
        superblock->s_free_BLOCK_NUM--;	//���п���-1

    }

    //���³�����
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);
    fflush(fw);

    //����blockλͼ
    block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE] = 1;
    fseek(fw,(retAddr-Block_StartAddr)/BLOCK_SIZE+BlockBitmap_StartAddr,SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
    fwrite(&block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE],sizeof(bool),1,fw);
    fflush(fw);

    return retAddr;

}

bool bfree(int addr)	//���̿��ͷź���
{
    //�ж�
    //�õ�ַ���Ǵ��̿����ʼ��ַ
    if( (addr-Block_StartAddr) % superblock->s_BLOCK_SIZE != 0 ){
        printf("��ַ����,��λ�ò���block�����̿飩��ʼλ��\n");
        return false;
    }
    unsigned int bno = (addr-Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode�ڵ��
    //�õ�ַ��δʹ�ã������ͷſռ�
    if(block_bitmap[bno]==0){
        printf("��block�����̿飩��δʹ�ã��޷��ͷ�\n");
        return false;
    }

    //�����ͷ�
    //���㵱ǰջ��
    int top;	//ջ��ָ��
    if(superblock->s_free_BLOCK_NUM==superblock->s_BLOCK_NUM){	//û�зǿ��еĴ��̿�
        printf("û�зǿ��еĴ��̿飬�޷��ͷ�\n");
        return false;	//û�пɷ���Ŀ��п飬����-1
    }
    else{	//����
        top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;

        //���block����
        char tmp[BLOCK_SIZE] = {0};
        fseek(fw,addr,SEEK_SET);
        fwrite(tmp,sizeof(tmp),1,fw);

        if(top == superblock->s_blocks_per_group-1){	//��ջ����

            //�ÿ��п���Ϊ�µĿ��п��ջ
            superblock->s_free[0] = superblock->s_free_addr;	//�µĿ��п��ջ��һ����ַָ��ɵĿ��п��ջָ��
            int i;
            for(i=1;i<superblock->s_blocks_per_group;i++){
                superblock->s_free[i] = -1;	//���ջԪ�ص�������ַ
            }
            fseek(fw,addr,SEEK_SET);
            fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�

        }
        else{	//ջ��δ��
            top++;	//ջ��ָ��+1
            superblock->s_free[top] = addr;	//ջ���������Ҫ�ͷŵĵ�ַ����Ϊ�µĿ��п�
        }
    }


    //���³�����
    superblock->s_free_BLOCK_NUM++;	//���п���+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //����blockλͼ
    block_bitmap[bno] = 0;
    fseek(fw,bno+BlockBitmap_StartAddr,SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
    fwrite(&block_bitmap[bno],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}

int ialloc()	//����i�ڵ�������������inode��ַ
{
    //��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
    if(superblock->s_free_INODE_NUM==0){
        printf("û�п���inode���Է���\n");
        return -1;
    }
    else{

        //˳����ҿ��е�inode
        int i;
        for(i=0;i<superblock->s_INODE_NUM;i++){
            if(inode_bitmap[i]==0)	//�ҵ�����inode
                break;
        }


        //���³�����
        superblock->s_free_INODE_NUM--;	//����inode��-1
        fseek(fw,Superblock_StartAddr,SEEK_SET);
        fwrite(superblock,sizeof(SuperBlock),1,fw);

        //����inodeλͼ
        inode_bitmap[i] = 1;
        fseek(fw,InodeBitmap_StartAddr+i,SEEK_SET);
        fwrite(&inode_bitmap[i],sizeof(bool),1,fw);
        fflush(fw);

        return Inode_StartAddr + i*superblock->s_INODE_SIZE;
    }
}

bool ifree(int addr)	//�ͷ�i���������
{
    //�ж�
    if( (addr-Inode_StartAddr) % superblock->s_INODE_SIZE != 0 ){
        printf("��ַ����,��λ�ò���i�ڵ���ʼλ��\n");
        return false;
    }
    unsigned short ino = (addr-Inode_StartAddr) / superblock->s_INODE_SIZE;	//inode�ڵ��
    if(inode_bitmap[ino]==0){
        printf("��inode��δʹ�ã��޷��ͷ�\n");
        return false;
    }

    //���inode����
    Inode tmp = {0};
    fseek(fw,addr,SEEK_SET);
    fwrite(&tmp,sizeof(tmp),1,fw);

    //���³�����
    superblock->s_free_INODE_NUM++;
    //����inode��+1
    fseek(fw,Superblock_StartAddr,SEEK_SET);
    fwrite(superblock,sizeof(SuperBlock),1,fw);

    //����inodeλͼ
    inode_bitmap[ino] = 0;
    fseek(fw,InodeBitmap_StartAddr+ino,SEEK_SET);
    fwrite(&inode_bitmap[ino],sizeof(bool),1,fw);
    fflush(fw);

    return true;
}

bool mkdir(int parinoAddr,char name[])	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    DirItem dirlist[16];	//��ʱĿ¼�嵥��32*16=512B��һ��block��

    //�������ַȡ��inode
    Inode cur; //��ǰ�ļ���inode
    fseek(fr,parinoAddr,SEEK_SET);//��ָ��ƫ�Ƶ���ǰ��Ŀ¼��inode��ַ
    fread(&cur,sizeof(Inode),1,fr);//ȡ����һ��Ŀ¼��inode�е�block���

    int i = 0;
    int cnt = cur.i_cnt+1;	//Ŀ¼����
    int posi = -1,posj = -1;
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        int dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        //ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++){

            if( strcmp(dirlist[j].itemName,name)==0 ){   //������ͬʱ
                Inode tmp;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);
                if( ((tmp.i_mode>>9)&1)==1 ){	//����Ŀ¼
                    printf("Ŀ¼�Ѵ���\n");
                    return false;
                }
            }
            else if( strcmp(dirlist[j].itemName,"")==0 ){
                //�ҵ�һ�����м�¼������Ŀ¼���������λ��
                //��¼���λ��
                if(posi==-1){
                    posi = dno;
                    posj = j;
                }

            }
            i++;
        }

    }

    if(posi!=-1){	//�ҵ��������λ��

        //ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //�������Ŀ¼��
        strcpy(dirlist[posj].itemName,name);	//Ŀ¼��
        //д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
        int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ
        if(chiinoAddr==-1){
            printf("inode����ʧ��\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

        //��������Ŀ��inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 2;	//�������ǰĿ¼,"."��".."

        //�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
        int curblockAddr = balloc();
        if(curblockAddr==-1){
            printf("block����ʧ��\n");
            return false;
        }
        DirItem dirlist2[16] = {0};	//��ʱĿ¼���б� - 2
        strcpy(dirlist2[0].itemName,".");
        strcpy(dirlist2[1].itemName,"..");
        dirlist2[0].inodeAddr = chiinoAddr;	//��ǰĿ¼inode��ַ chiinoAddr for child inode address
        dirlist2[1].inodeAddr = parinoAddr;	//��Ŀ¼inode��ַ   parinoAddr for parent inode address

        //д�뵽��ǰĿ¼�Ĵ��̿�
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(dirlist2,sizeof(dirlist2),1,fw);

        p.i_dirBlock[0] = curblockAddr;
        int k;
        for(k=1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        p.i_size = superblock->s_BLOCK_SIZE;
        p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
        p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

        //��inodeд�뵽�����inode��ַ
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //����ǰĿ¼�Ĵ��̿�д��
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //д��inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        return true;
    }
    else{
        printf("û�ҵ�����Ŀ¼��,Ŀ¼����ʧ��");
        return false;
    }
}

void rmall(int parinoAddr)	//ɾ���ýڵ��������ļ���Ŀ¼
{
    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼�����������Ŀ¼�ļ���û�������ļ�����Ϊ�յĻ���ֻ��Ҫ����Ŀ¼�ļ��ͷ�
    int cnt = cur.i_cnt;
    if(cnt<=2){
        bfree(cur.i_dirBlock[0]);
        ifree(parinoAddr);
        return ;
    }

    //����ȡ�����̿飬Ŀ¼�ļ���Ϊ��
    int i = 0;
    while(i<160){	//С��160��10��block��һ��block���Դ�16��Ŀ¼�32B��
        DirItem dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){   //�����blockδʹ��
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16]; //��Ŀ¼�ļ���inode�ж�ȡblock���λ��
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�Ӵ��̿�������ȡ��Ŀ¼��ݹ�ɾ��
        int j;
        bool f = false;
        for(j=0;j<16;j++){
            //Inode tmp;

            if( ! (strcmp(dirlist[j].itemName,".")==0 ||
                   strcmp(dirlist[j].itemName,"..")==0 ||
                   strcmp(dirlist[j].itemName,"")==0 ) ){
                f = true;
                rmall(dirlist[j].inodeAddr);	//�ݹ�ɾ��
            }

            cnt = cur.i_cnt;
            i++;
        }

        //�ô��̿��ѿգ�����
        if(f)
            bfree(parblockAddr);

    }
    //��inode�ѿգ�����
    ifree(parinoAddr);
    return ;

}

bool rmdir(int parinoAddr,char name[])	//Ŀ¼ɾ������
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    if(strcmp(name,".")==0 || strcmp(name,"..")==0){
        printf("�������\n");
        return 0;
    }

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    if( (((cur.i_mode>>filemode>>1)&1)==0) && (strcmp(Cur_User_Name,"root")!=0) ){
        //û��д��Ȩ��
        printf("Ȩ�޲��㣺��д��Ȩ��\n");
        return false;
    }


    //����ȡ�����̿�
    int i = 0;
    while(i<160){	//С��160
        DirItem dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�ҵ�Ҫɾ����Ŀ¼
        int j;
        for(j=0;j<16;j++){
            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[j].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
                    //��Ŀ¼

                    rmall(dirlist[j].inodeAddr);

                    //ɾ����Ŀ¼��Ŀ��д�ش���
                    strcpy(dirlist[j].itemName,"");
                    dirlist[j].inodeAddr = -1;
                    fseek(fw,parblockAddr,SEEK_SET);
                    fwrite(&dirlist,sizeof(dirlist),1,fw);
                    cur.i_cnt--;
                    fseek(fw,parinoAddr,SEEK_SET);
                    fwrite(&cur,sizeof(Inode),1,fw);

                    fflush(fw);
                    return true;
                }
                else{
                    //����Ŀ¼������
                }
            }
            i++;
        }

    }

    printf("û���ҵ���Ŀ¼\n");
    return false;
}

bool create(int parinoAddr,char name[],char buf[])	//�����ļ��������ڸ�Ŀ¼�´����ļ����ļ����ݴ���buf
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("��������ļ�������\n");
        return false;
    }

    DirItem dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    int i = 0;
    int posi = -1,posj = -1;	//�ҵ���Ŀ¼λ��
    int dno;
    int cnt = cur.i_cnt+1;	//Ŀ¼����
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++){

            if( posi==-1 && strcmp(dirlist[j].itemName,"")==0 ){
                //�ҵ�һ�����м�¼�������ļ����������λ��
                posi = dno;
                posj = j;
            }
            else if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                Inode cur2;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&cur2,sizeof(Inode),1,fr);
                if( ((cur2.i_mode>>9)&1)==0 ){	//���ļ������������ܴ����ļ�
                    printf("�ļ��Ѵ���\n");
                    buf[0] = '\0';
                    return false;
                }
            }
            i++;
        }

    }
    if(posi!=-1){	//֮ǰ�ҵ�һ��Ŀ¼����
        //ȡ��֮ǰ�Ǹ�����Ŀ¼���Ӧ�Ĵ��̿�
        fseek(fr,cur.i_dirBlock[posi],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //�������Ŀ¼��
        strcpy(dirlist[posj].itemName,name);	//�ļ���
        int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ
        if(chiinoAddr==-1){
            printf("inode����ʧ��\n");
            return false;
        }
        dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

        //��������Ŀ��inode
        Inode p;
        p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
        p.i_atime = time(NULL);
        p.i_ctime = time(NULL);
        p.i_mtime = time(NULL);
        strcpy(p.i_uname,Cur_User_Name);
        strcpy(p.i_gname,Cur_Group_Name);
        p.i_cnt = 1;	//ֻ��һ���ļ�ָ��


        //��buf���ݴ浽���̿�
        int k;
        int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
        for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K
            //�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //д�뵽��ǰĿ¼�Ĵ��̿�
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
        }


        for(k=len/superblock->s_BLOCK_SIZE+1;k<10;k++){
            p.i_dirBlock[k] = -1;
        }
        if(len==0){	//����Ϊ0�Ļ�Ҳ�ָ���һ��block
            int curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return false;
            }
            p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
            //д�뵽��ǰĿ¼�Ĵ��̿�
            fseek(fw,curblockAddr,SEEK_SET);
            fwrite(buf,superblock->s_BLOCK_SIZE,1,fw);

        }
        p.i_size = len;
        p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
        p.i_mode = 0;
        p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;

        //��inodeд�뵽�����inode��ַ
        fseek(fw,chiinoAddr,SEEK_SET);
        fwrite(&p,sizeof(Inode),1,fw);

        //����ǰĿ¼�Ĵ��̿�д��
        fseek(fw,cur.i_dirBlock[posi],SEEK_SET);
        fwrite(dirlist,sizeof(dirlist),1,fw);

        //д��inode
        cur.i_cnt++;
        fseek(fw,parinoAddr,SEEK_SET);
        fwrite(&cur,sizeof(Inode),1,fw);
        fflush(fw);

        return true;
    }
    else
        return false;
}

bool del(int parinoAddr,char name[])		//ɾ���ļ��������ڵ�ǰĿ¼��ɾ���ļ�
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return false;
    }

    //�������ַȡ���ļ��ĸ�Ŀ¼��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ���ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )//�ļ���ӵ����
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)//�ļ���ͬ��ӵ����
        filemode = 3;
    else
        filemode = 0;
    //todo change �����жϵ����ļ�Ŀ¼��Ȩ�ޣ�Ϊ�˺��潫Ŀ¼������ã�дȨ�ޣ�
    if( ((cur.i_mode>>filemode>>1)&1)==0 ){
        //û��д��Ȩ�ޣ�ɾ���ļ���Ҫ�ļ���Ŀ¼���д��Ȩ��
        printf("Ȩ�޲��㣺��д��Ȩ��\n");
        return false;
    }
    //todo change

    //����ȡ�����̿�

    int i = 0;
    while(i<160){	//С��160
        DirItem dirlist[16] = {0};

        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //�ҵ�Ҫɾ����Ŀ¼
        int pos;
        for(pos=0;pos<16;pos++){
            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[pos].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);

            if( strcmp(dirlist[pos].itemName,name)==0){
                if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
                    //��Ŀ¼������
                }
                else{
                    //���ļ�
                    //todo change judge file mode
                    //Inode file;
                    //fseek(fr,dirlist[pos].inodeAddr,SEEK_SET);
                    //fread(&file,sizeof(Inode),1,fr);
                    int delfilemode;
                    if(strcmp(Cur_User_Name,tmp.i_uname)==0 )//�ļ���ӵ����
                        delfilemode = 6;
                    else if(strcmp(Cur_User_Name,tmp.i_gname)==0)//�ļ���ͬ��ӵ����
                        delfilemode = 3;
                    else
                        delfilemode = 0;

                    if(((tmp.i_mode>>delfilemode>>1)&1)==0)

                    {
                        printf("Ȩ�޲��㣺��д��Ȩ��\n");
                        return false;
                    }
                    //todo change judge file mode
                    //�ͷ�block
                    int k;
                    for(k=0;k<10;k++)
                        if(tmp.i_dirBlock[k]!=-1)
                            bfree(tmp.i_dirBlock[k]);

                    //�ͷ�inode
                    ifree(dirlist[pos].inodeAddr);

                    //ɾ����Ŀ¼��Ŀ��д�ش���
                    strcpy(dirlist[pos].itemName,"");
                    dirlist[pos].inodeAddr = -1;
                    fseek(fw,parblockAddr,SEEK_SET);
                    fwrite(&dirlist,sizeof(dirlist),1,fw);
                    cur.i_cnt--;
                    fseek(fw,parinoAddr,SEEK_SET);
                    fwrite(&cur,sizeof(Inode),1,fw);

                    fflush(fw);
                    return true;
                }
            }
            i++;
        }

    }

    printf("û���ҵ����ļ�!\n");
    return false;
}


void ls(int parinoAddr)		//��ʾ��ǰĿ¼�µ������ļ����ļ��С���������ǰĿ¼��inode�ڵ��ַ
{
    Inode cur;
    //ȡ�����inode
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);
    fflush(fr);

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    if( ((cur.i_mode>>filemode>>2)&1)==0 ){
        //û�ж�ȡȨ��
        printf("Ȩ�޲��㣺�޶�ȡȨ��\n");
        return ;
    }

    //����ȡ�����̿�
    int i = 0;
    while(i<cnt && i<160){
        DirItem dirlist[16] = {0};
        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16 && i<cnt;j++){
            Inode tmp;
            //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
            fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
            fread(&tmp,sizeof(Inode),1,fr);
            fflush(fr);

            if( strcmp(dirlist[j].itemName,"")==0 ){
                continue;
            }

            //�����Ϣ
            if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
                //����Ŀ¼�ļ�
                printf("d");
            }
            else{
                //������ͨ�ļ�
                printf("-");
            }

            tm *ptr;	//�洢ʱ��
            ptr=gmtime(&tmp.i_mtime);

            //���Ȩ����Ϣ
            int t = 8;
            while(t>=0){
                if( ((tmp.i_mode>>t)&1)==1){
                    if(t%3==2)	printf("r");
                    if(t%3==1)	printf("w");
                    if(t%3==0)	printf("x");
                }
                else{
                    printf("-");
                }
                t--;
            }
            printf("\t");

            //����
            printf("%d\t",tmp.i_cnt);	//����
            printf("%s\t",tmp.i_uname);	//�ļ������û���
            printf("%s\t",tmp.i_gname);	//�ļ������û���
            printf("%d B\t",tmp.i_size);	//�ļ���С
            printf("%d.%d.%d %02d:%02d:%02d  ",1900+ptr->tm_year,ptr->tm_mon+1,ptr->tm_mday,(8+ptr->tm_hour)%24,ptr->tm_min,ptr->tm_sec);	//��һ���޸ĵ�ʱ��
            printf("%s",dirlist[j].itemName);	//�ļ���
            printf("\n");
            i++;
        }

    }
    /*  δд�꣬û��ls foldername */

}

void cd(int parinoAddr,char name[])	//���뵱ǰĿ¼�µ�nameĿ¼
{
    char *name1,*name2;
    if(strlen(name)==1 && strcmp(name,"/")==0)
    {
        Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        strcpy(Cur_Dir_Name,"/");
        return;
    }

    name2=strstr(name,"/");
    if(name2!=NULL && name[0]!='/')
    {
        name2++;
        name1=strtok(name,"/");
    }
    else if(name[0]=='/')
    {
        Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        strcpy(Cur_Dir_Name,"/");
        parinoAddr=Cur_Dir_Addr;
        name++;
        name2=strstr(name,"/");
        if(name2!=NULL)
        {
            name2++;
            name1=strtok(name,"/");
        }
        else
            name1=name;
    }
    else
        name1=name;

    //ȡ����ǰĿ¼��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
    int i = 0;

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    //i_mode����filemodeλ�����λ�������ڵ�ǰ�û����ݵĶ��ڸ��ļ��Ĵ�ȡȨ��
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    while(i<160)
    {
        DirItem dirlist[16] = {0};
        if(cur.i_dirBlock[i/16]==-1)
        {
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++)
        {
            if(strcmp(dirlist[j].itemName,name1)==0)
            {
                Inode tmp;
                //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ( (tmp.i_mode>>9) & 1 ) == 1 )
                {
                    //�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
                    //Ŀ¼�Ľ���Ȩ�޾��Ǹ�Ŀ¼�ļ���ִ��Ȩ��
                    if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 )
                    {	//root�û�����Ŀ¼�����Բ鿴
                        //û��ִ��Ȩ��
                        printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
                        return ;
                    }

                    //�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼

                    Cur_Dir_Addr = dirlist[j].inodeAddr;
                    if( strcmp(dirlist[j].itemName,".")==0)
                    {
                        //��Ŀ¼������
                    }
                    else if(strcmp(dirlist[j].itemName,"..")==0)
                    {
                        //��һ��Ŀ¼
                        int k;
                        for(k=strlen(Cur_Dir_Name);k>=0;k--)
                            if(Cur_Dir_Name[k]=='/')
                                break;
                        Cur_Dir_Name[k]='\0';
                        if(strlen(Cur_Dir_Name)==0)//���ڸ�Ŀ¼��cd ..
                            Cur_Dir_Name[0]='/',Cur_Dir_Name[1]='\0';
                    }
                    else
                    {
                        if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')
                            //��ǰ�Ĳ���Ŀ¼�Ǹ�Ŀ¼
                            strcat(Cur_Dir_Name,"/");//��Cur_Dir_Name��׷���ַ���
                        strcat(Cur_Dir_Name,dirlist[j].itemName);
                        if(name2!=NULL)
                            cd(Cur_Dir_Addr,name2);
                        else
                            return;
                    }

                    return ;
                }
                else
                {
                    //�ҵ���Ŀ¼��������Ŀ¼��������
                }
            }
            i++;
        }
    }
    //û�ҵ�
    printf("û�и�Ŀ¼\n");
    return ;

}


void gotoxy(HANDLE hOut, int x, int y)	//�ƶ���굽ָ��λ��
{
    COORD pos;
    pos.X = x;             //������
    pos.Y = y;            //������
    SetConsoleCursorPosition(hOut, pos);
}

void vi(int parinoAddr,char name[],char buf[])	//ģ��һ����vi�������ı���nameΪ�ļ���
{
    //���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("��������ļ�������\n");
        return ;
    }

    //��ջ�����
    memset(buf,0,sizeof(buf));
    int maxlen = 0;	//���������󳤶�

    //��������ͬ���ļ����еĻ�����༭ģʽ��û�н��봴���ļ�ģʽ
    DirItem dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    int i = 0,j;
    int dno;
    int fileInodeAddr = -1;	//�ļ���inode��ַ
    bool isExist = false;	//�ļ��Ƿ��Ѵ���
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ���������������ļ������༭
                    fileInodeAddr = dirlist[j].inodeAddr;
                    isExist = true;
                    goto label;
                }
            }
            i++;
        }
    }
    label:

    //��ʼ��vi
    int cnt = 0;    //cnt�����ı��ļ��Ĵ�С
    system("cls");	//����

    int winx,winy,curx,cury;

    HANDLE handle_out;                              //����һ�����
    CONSOLE_SCREEN_BUFFER_INFO screen_info;         //���崰�ڻ�������Ϣ�ṹ��
    COORD pos = {0, 0};                             //����һ������ṹ��

    if(isExist){	//�ļ��Ѵ��ڣ�����༭ģʽ�������֮ǰ���ļ�����
        int vifilemode;
        if(strcmp(Cur_User_Name,fileInode.i_uname)==0 )
            vifilemode = 6;
        else if(strcmp(Cur_User_Name,fileInode.i_gname)==0)
            vifilemode = 3;
        else
            vifilemode = 0;
        //Ȩ���жϡ��ж��ļ��Ƿ�ɶ�
        if( ((fileInode.i_mode>>vifilemode>>2)&1)==0){
            //���ɶ�
            printf("Ȩ�޲��㣺û�ж�Ȩ��\n");
            return ;
        }

        //���ļ����ݶ�ȡ��������ʾ�ڣ�������
        i = 0;
        int sumlen = fileInode.i_size;	//�ļ�����
        int getlen = 0;	//ȡ�����ĳ���
        for(i=0;i<10;i++){
            char fileContent[1000] = {0};
            if(fileInode.i_dirBlock[i]==-1){
                continue;
            }
            //����ȡ�����̿������
            fseek(fr,fileInode.i_dirBlock[i],SEEK_SET);
            fread(fileContent,superblock->s_BLOCK_SIZE,1,fr);	//��ȡ��һ�����̿��С������
            fflush(fr);
            //����ַ���
            int curlen = 0;	//��ǰָ��
            while(curlen<superblock->s_BLOCK_SIZE){
                if(getlen>=sumlen)	//ȫ��������
                    break;
                printf("%c",fileContent[curlen]);	//�������Ļ
                buf[cnt++] = fileContent[curlen];	//�����buf
                curlen++;
                getlen++;
            }
            if(getlen>=sumlen)
                break;
        }
        maxlen = sumlen;
    }

    //������֮��Ĺ��λ��
    handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //��ñ�׼����豸���
    GetConsoleScreenBufferInfo(handle_out, &screen_info);   //��ȡ������Ϣ
    winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
    winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
    curx = screen_info.dwCursorPosition.X;
    cury = screen_info.dwCursorPosition.Y;


    //����vi
    //����vi��ȡ�ļ�����

    int mode = 0;	//viģʽ��һ��ʼ������ģʽ
    unsigned char c;
    while(1){
        if(mode==0){	//������ģʽ
            c=getch();

            if(c=='i' || c=='a'){	//����ģʽ
                if(c=='a'){
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        /*
                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //������һ�������·�ҳ
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - ����ģʽ - ");
                            gotoxy(handle_out,0,cury);
                        }
                        */
                    }
                }

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }
                else{
                    //��ʾ "����ģʽ"
                    gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,winy-1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,curx,cury);
                }
                //todo
                gotoxy(handle_out,curx,cury);
                mode = 1;


            }
            //todo: 1333-1408,1356-1357������ɫ�Ժ����⣬������
            else if(c==':'){
                //system("color 09");//�����ı�Ϊ��ɫ
                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);
                _COORD pos;
                if(cury-winy+2>0)
                    pos.X = 0,pos.Y = cury+1;
                else
                    pos.X = 0,pos.Y = winy-1;
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");

                if(cury-winy+2>0)
                    gotoxy(handle_out,0,cury+1);
                else
                    gotoxy(handle_out,0,winy-1);
                WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // �ı�����
                //todo :���´���������,�������ͻ
//                FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ
                SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
                printf(":");

                char pc;
                int tcnt = 1;	//������ģʽ������ַ�����
                while( c = getch() ){
                    if(c=='\r'){	//�س�
                        break;
                    }
                    else if(c=='\b'){	//�˸񣬴�������ɾ��һ���ַ�
                        //SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                        tcnt--;
                        if(tcnt==0)
                            break;
                        printf("\b");
                        printf(" ");
                        printf("\b");
                        continue;
                    }
                    pc = c;
                    printf("%c",pc);
                    tcnt++;
                }
                if(pc=='q'){
                    buf[cnt] = '\0';
                    //buf[maxlen] = '\0';
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    system("cls");
                    break;	//vi >>>>>>>>>>>>>> �˳�����
                }
                else{
                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");

                    if(cury-winy+2>0)
                        gotoxy(handle_out,0,cury+1);
                    else
                        gotoxy(handle_out,0,winy-1);
                    SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
                    //FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ
                    printf(" ��������");
                    //getch();
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    gotoxy(handle_out,curx,cury);
                }
            }
            else if(c==27){	//ESC��������ģʽ����״̬��
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                gotoxy(handle_out,curx,cury);

            }

        }
        else if(mode==1){	//����ģʽ

            gotoxy(handle_out,winx/4*3,winy-1);
            int i = winx/4*3;
            while(i<winx-1){
                printf(" ");
                i++;
            }
            if(cury>winy-2){
                gotoxy(handle_out,winx/4*3,cury+2);
                printf("\b\b\b\b\b\b\b\b\b\b");
                printf("                              ");
                printf("\b\b\b\b\b\b\b\b\b\b");
                gotoxy(handle_out,winx/4*3,cury+1);
            }

            else
                gotoxy(handle_out,winx/4*3,winy-1);
            printf("[��:%d,��:%d]",curx==-1?0:curx,cury);
            //todo

            //todo
            gotoxy(handle_out,curx,cury);

            c = getch();
            if(c==27){	// ESC����������ģʽ
                mode = 0;
                //��״̬��
                gotoxy(handle_out,0,winy-1);
                SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                int i;
                for(i=0;i<winx-1;i++)
                    printf(" ");
                continue;
            }
            else if(c=='\b'){	//�˸�ɾ��һ���ַ�
                if(cnt==0)	//�Ѿ��˵��ʼ
                    continue;
                printf("\b");
                printf(" ");
                printf("\b");
                curx--;
                cnt--;	//ɾ���ַ�
                if(buf[cnt]=='\n'){
                    //Ҫɾ��������ַ��ǻس������ص���һ��
                    if(cury!=0)
                        cury--;
                    int k;
                    curx = 0;//�ҵ���һ�е�ĩβλ��
                    for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
                        curx++;
                    gotoxy(handle_out,curx,cury);
                    printf(" ");
                    gotoxy(handle_out,curx,cury);
                    if(cury-winy+2>=0){	//��ҳʱ
                        gotoxy(handle_out,curx,0);
                        gotoxy(handle_out,curx,cury+1);
                        SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                        int i;
                        for(i=0;i<winx-1;i++)
                            printf(" ");
                        gotoxy(handle_out,0,cury+1);
                        printf(" - ����ģʽ - ");
                        gotoxy(handle_out,0,cury+2);
                        printf("\b\b\b\b\b\b\b\b\b\b");
                        printf("                              ");
                        printf("\b\b\b\b\b\b\b\b\b\b");

                    }
                    gotoxy(handle_out,curx,cury);

                }
                else
                    buf[cnt] = ' ';
                continue;
            }
            //todo to find out a way to solve this
            else if(c==224){	//�ж��Ƿ��Ǽ�ͷ
                c = getch();
                if(c==75){	//���ͷ
                    if(cnt!=0){
                        cnt--;
                        curx--;
                        if(buf[cnt]=='\n'){
                            //��һ���ַ��ǻس�
                            if(cury!=0)
                                cury--;
                            int k;
                            curx = 0;
                            for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
                                curx++;
                        }
                        gotoxy(handle_out,curx,cury);
                    }
                }
                else if(c==77){	//�Ҽ�ͷ
                    cnt++;
                    if(cnt>maxlen)
                        maxlen = cnt;
                    curx++;
                    if(curx==winx){
                        curx = 0;
                        cury++;

                        if(cury>winy-2 || cury%(winy-1)==winy-2){
                            //������һ�������·�ҳ
                            if(cury%(winy-1)==winy-2)
                                printf("\n");
                            SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                            int i;
                            for(i=0;i<winx-1;i++)
                                printf(" ");
                            gotoxy(handle_out,0,cury+1);
                            printf(" - ����ģʽ - ");
                            gotoxy(handle_out,0,cury);
                        }

                    }
                    gotoxy(handle_out,curx,cury);
                }
                continue;
            }
            if(c=='\r'){	//�����س�
                printf("\n");
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }

                buf[cnt++] = '\n';
                if(cnt>maxlen)
                    maxlen = cnt;
                continue;
            }
            else{
                printf("%c",c);
            }
            //�ƶ����
            curx++;
            if(curx==winx){
                curx = 0;
                cury++;

                if(cury>winy-2 || cury%(winy-1)==winy-2){
                    //������һ�������·�ҳ
                    if(cury%(winy-1)==winy-2)
                        printf("\n");
                    SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
                    int i;
                    for(i=0;i<winx-1;i++)
                        printf(" ");
                    gotoxy(handle_out,0,cury+1);
                    printf(" - ����ģʽ - ");
                    gotoxy(handle_out,0,cury);
                }

                buf[cnt++] = '\n';
                if(cnt>maxlen)
                    maxlen = cnt;
                if(cury==winy){
                    printf("\n");
                }
            }
            //��¼�ַ�
            buf[cnt++] = c;
            if(cnt>maxlen)
                maxlen = cnt;
        }
        else{	//����ģʽ
        }
    }
    if(isExist){	//����Ǳ༭ģʽ
        //��buf����д���ļ��Ĵ��̿�

        if( ((fileInode.i_mode>>filemode>>1)&1)==1 ){	//��д
            writefile(fileInode,fileInodeAddr,buf);
        }
        else{	//����д
            printf("Ȩ�޲��㣺��д��Ȩ��\n");
        }

    }
    else{	//�Ǵ����ļ�ģʽ
        if( ((cur.i_mode>>filemode>>1)&1)==1){
            //��д�����Դ����ļ�
            create(parinoAddr,name,buf);	//�����ļ�
        }
        else{
            printf("Ȩ�޲��㣺��д��Ȩ��\n");
            return ;
        }
    }
}

void writefile(Inode fileInode,int fileInodeAddr,char buf[])	//��buf����д���ļ��Ĵ��̿�
{
    //buf
    //fileInode
    //fileInodeAddr
    //��buf����д�ش��̿�
    int k;
    int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
    for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K
        //�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
        int curblockAddr;
        if(fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE]==-1){
            //ȱ�ٴ��̿飬����һ��
            curblockAddr = balloc();
            if(curblockAddr==-1){
                printf("block����ʧ��\n");
                return ;
            }
            fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
        }
        else{
            curblockAddr = fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE];
        }
        //д�뵽��ǰĿ¼�Ĵ��̿�
        fseek(fw,curblockAddr,SEEK_SET);
        fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);    //buf+k����ָʾ��ǰ��������ȡλ��
        fflush(fw);
    }
    //���¸��ļ���С
    fileInode.i_size = len;
    fileInode.i_mtime = time(NULL);
    fseek(fw,fileInodeAddr,SEEK_SET);
    fwrite(&fileInode,sizeof(Inode),1,fw);
    fflush(fw);
}

void inUsername(char username[])	//�����û���
{
    printf("username:");
    scanf("%s",username);	//�û���
}

void inPasswd(char passwd[])	//�������룬ʵ��������������
{
    int plen = 0;
    char c;
    fflush(stdin);	//��ջ�����
    printf("passwd:");
    while(c=getch()){
        if(c=='\r'){	//����س�������ȷ��
            passwd[plen] = '\0';
            fflush(stdin);	//�建����
            printf("\n");
            break;
        }
        else if(c=='\b'){	//�˸�ɾ��һ���ַ�
            if(plen!=0){	//û��ɾ��ͷ
                plen--;
            }
        }
        else{	//�����ַ�
            passwd[plen++] = c;
        }
    }
}

bool login()	//��½����
{
    char username[100] = {0};
    char passwd[100] = {0};
    inUsername(username);	//�����û���
    inPasswd(passwd);		//�����û�����
    if(check(username,passwd)){	//�˶��û���������
        isLogin = true;
        return true;
    }
    else{
        isLogin = false;
        return false;
    }
}

bool check(char username[],char passwd[])	//�˶��û���������
{
    int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
    int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
    Inode passwd_Inode;		//�û��ļ���inode
    Inode shadow_Inode;		//�û������ļ���inode

    Inode cur_dir_inode;	//��ǰĿ¼��inode
    int i,j;
    DirItem dirlist[16];	//��ʱĿ¼

    cd(Cur_Dir_Addr,"etc");	//���������ļ�Ŀ¼

    //�ҵ�passwd��shadow�ļ�inode��ַ����ȡ��
    //ȡ����ǰĿ¼��inode
    fseek(fr,Cur_Dir_Addr,SEEK_SET);
    fread(&cur_dir_inode,sizeof(Inode),1,fr);
    //����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
    for(i=0;i<10;i++){
        if(cur_dir_inode.i_dirBlock[i]==-1){
            continue;
        }
        //����ȡ�����̿�
        fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        for(j=0;j<16;j++){	//����Ŀ¼��
            if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
                strcmp(dirlist[j].itemName,"shadow")==0  ){
                Inode tmp;	//��ʱinode
                //ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ((tmp.i_mode>>9)&1)==0 ){
                    //���ļ�
                    //�б���passwd�ļ�����shadow�ļ�
                    if( strcmp(dirlist[j].itemName,"passwd")==0 ){
                        passwd_Inode_Addr = dirlist[j].inodeAddr;
                        passwd_Inode = tmp;
                    }
                    else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
                        shadow_Inode_Addr = dirlist[j].inodeAddr;
                        shadow_Inode = tmp;
                    }
                }
            }
        }
        if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1)	//���ҵ���
            break;
    }

    //����passwd�ļ������Ƿ�����û�username
    char buf[1000000];	//���1M���ݴ�passwd���ļ�����
    char buf2[600];		//�ݴ���̿�����
    j = 0;	//���̿�ָ��
    //ȡ��passwd�ļ�����
    for(i=0;i<passwd_Inode.i_size;i++){
        if(i%superblock->s_BLOCK_SIZE==0){	//������
            //���µĴ��̿�
            fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char buf3[600];
    buf3[0]='\n';
    j=1;
    if(strcmp(username,"root")!=0)
    {
        for(i=0;i<strlen(username);i++)
        {
            buf3[j]=username[i];
            j++;
        }

        buf3[j++]=':';
        buf3[j]='\0';

        if(strstr(buf,buf3)==NULL)
        {
            //û�ҵ����û�
            printf("�û�������\n");
            cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
            return false;
        }
    }
    /*
    else
    {
        if(strstr(buf,username)==NULL){
        //û�ҵ����û�
        printf("�û�������\n");
        cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
        return false;
    }
    }
    */




    //������ڣ��鿴shadow�ļ���ȡ�����룬�˶�passwd�Ƿ���ȷ
    //ȡ��shadow�ļ�����
    j = 0;
    for(i=0;i<shadow_Inode.i_size;i++){
        if(i%superblock->s_BLOCK_SIZE==0)
        {	//������������̿�
            //���µĴ��̿�
            fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char *p;	//�ַ�ָ��
    if( (p = strstr(buf,username))==NULL){
        //û�ҵ����û�
        printf("shadow�ļ��в����ڸ��û�\n");
        cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
        return false;
    }
    //�ҵ����û���ȡ������
    while((*p)!=':'){
        p++;
    }
    p++;
    j = 0;
    while((*p)!='\n'){
        buf2[j++] = *p;
        p++;
    }
    buf2[j] = '\0';

    //�˶�����
    if(strcmp(buf2,passwd)==0){	//������ȷ����½
        strcpy(Cur_User_Name,username);
        if(strcmp(username,"root")==0)
            strcpy(Cur_Group_Name,"root");	//��ǰ��½�û�����
        else
            strcpy(Cur_Group_Name,"user");	//��ǰ��½�û�����
        cd(Cur_Dir_Addr,"..");
        cd(Cur_Dir_Addr,"home");\
		cd(Cur_Dir_Addr,username);	//���뵽�û�Ŀ¼
        strcpy(Cur_User_Dir_Name,Cur_Dir_Name);	//���Ƶ�ǰ��½�û�Ŀ¼��
        return true;
    }
    else{
        printf("�������\n");
        cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
        return false;
    }
}

void gotoRoot()	//�ص���Ŀ¼
{
    memset(Cur_User_Name,0,sizeof(Cur_User_Name));		//��յ�ǰ�û���
    memset(Cur_User_Dir_Name,0,sizeof(Cur_User_Dir_Name));	//��յ�ǰ�û�Ŀ¼
    Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
    strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
}

void logout()	//�û�ע��
{
    //�ص���Ŀ¼
    gotoRoot();

    isLogin = false;
    printf("�û�ע��\n");
    system("pause");
    system("cls");
}

bool useradd(char username[])	//�û�ע��
{
    if(strcmp(Cur_User_Name,"root")!=0){
        printf("Ȩ�޲���\n");
        return false;
    }
    int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
    int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
    int group_Inode_Addr = -1;	//�û����ļ�inode��ַ
    Inode passwd_Inode;		//�û��ļ���inode
    Inode shadow_Inode;		//�û������ļ���inode
    Inode group_Inode;		//�û����ļ�inode
    //ԭ����Ŀ¼
    char bak_Cur_User_Name[110];
    char bak_Cur_User_Name_2[110];
    char bak_Cur_User_Dir_Name[310];
    int bak_Cur_Dir_Addr;
    char bak_Cur_Dir_Name[310];
    char bak_Cur_Group_Name[310];

    Inode cur_dir_inode;	//��ǰĿ¼��inode
    int i,j;
    DirItem dirlist[16];	//��ʱĿ¼

    //�����ֳ����ص���Ŀ¼
    strcpy(bak_Cur_User_Name,Cur_User_Name);
    strcpy(bak_Cur_User_Dir_Name,Cur_User_Dir_Name);
    bak_Cur_Dir_Addr = Cur_Dir_Addr;
    strcpy(bak_Cur_Dir_Name,Cur_Dir_Name);

    //�����û�Ŀ¼
    gotoRoot();
    cd(Cur_Dir_Addr,"home");
    //�����ֳ�
    strcpy( bak_Cur_User_Name_2 , Cur_User_Name);
    strcpy( bak_Cur_Group_Name , Cur_Group_Name);
    //����
    strcpy(Cur_User_Name,username);
    strcpy(Cur_Group_Name,"user");
    if(!mkdir(Cur_Dir_Addr,username)){
        strcpy( Cur_User_Name,bak_Cur_User_Name_2);
        strcpy( Cur_Group_Name,bak_Cur_Group_Name);
        //�ָ��ֳ����ص�ԭ����Ŀ¼
        strcpy(Cur_User_Name,bak_Cur_User_Name);
        strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
        Cur_Dir_Addr = bak_Cur_Dir_Addr;
        strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

        printf("�û�ע��ʧ��!\n");
        return false;
    }
    //�ָ��ֳ�
    strcpy( Cur_User_Name,bak_Cur_User_Name_2);
    strcpy( Cur_Group_Name,bak_Cur_Group_Name);

    //�ص���Ŀ¼
    gotoRoot();

    //�����û�Ŀ¼
    cd(Cur_Dir_Addr,"etc");

    //�����û�����
    char passwd[100] = {0};
    inPasswd(passwd);	//��������

    //�ҵ�passwd��shadow�ļ�inode��ַ����ȡ����׼��������Ŀ

    //ȡ����ǰĿ¼��inode
    fseek(fr,Cur_Dir_Addr,SEEK_SET);
    fread(&cur_dir_inode,sizeof(Inode),1,fr);

    //����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
    for(i=0;i<10;i++){
        if(cur_dir_inode.i_dirBlock[i]==-1){
            continue;
        }
        //����ȡ�����̿�
        fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        for(j=0;j<16;j++){	//����Ŀ¼��
            if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
                strcmp(dirlist[j].itemName,"shadow")==0 ||
                strcmp(dirlist[j].itemName,"group")==0){
                Inode tmp;	//��ʱinode
                //ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ((tmp.i_mode>>9)&1)==0 ){
                    //���ļ�
                    //�б���passwd�ļ�����shadow�ļ�
                    if( strcmp(dirlist[j].itemName,"passwd")==0 ){
                        passwd_Inode_Addr = dirlist[j].inodeAddr;
                        passwd_Inode = tmp;
                    }
                    else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
                        shadow_Inode_Addr = dirlist[j].inodeAddr;
                        shadow_Inode = tmp;
                    }
                    else if(strcmp(dirlist[j].itemName,"group")==0 ){
                        group_Inode_Addr = dirlist[j].inodeAddr;
                        group_Inode = tmp;
                    }
                }
            }
        }
        if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1&&group_Inode_Addr!=-1)	//���ҵ���
            break;
    }

    //����passwd�ļ������Ƿ�����û�username
    char buf[100000];	//���100K���ݴ�passwd���ļ�����
    char buf2[600];		//�ݴ���̿�����
    j = 0;	//���̿�ָ��
    //ȡ��passwd�ļ�����
    for(i=0;i<passwd_Inode.i_size;i++){
        if(i%superblock->s_BLOCK_SIZE==0){	//������
            //���µĴ��̿�
            fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char buf3[100];
    buf3[0]='\n';
    if(strcmp(buf,"root")!=0)
    {   j=1;
        for(i=0;i<strlen(username);i++)
        {
            buf3[j++]=username[i];

        }
        buf3[j++]=':';
        buf3[j]='\0';

        if(strstr(buf,buf3)!=NULL)
        {
            printf("�û��Ѵ���******\n");

            //�ָ��ֳ����ص�ԭ����Ŀ¼
            strcpy(Cur_User_Name,bak_Cur_User_Name);
            strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
            Cur_Dir_Addr = bak_Cur_Dir_Addr;
            strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
            return false;

        }

    }


    else{
        //���ڸ��û�

        printf("�û��Ѵ���******\n");

        //�ָ��ֳ����ص�ԭ����Ŀ¼
        strcpy(Cur_User_Name,bak_Cur_User_Name);
        strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
        Cur_Dir_Addr = bak_Cur_Dir_Addr;
        strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
        return false;

    }


    //��������ڣ���passwd�д������û���Ŀ,�޸�group�ļ�

    sprintf(buf+strlen(buf),"%s:x:%d:%d\n",username,nextUID++,1);	//������Ŀ���û������������룺�û�ID���û���ID���û���Ϊ��ͨ�û��飬ֵΪ1
    passwd_Inode.i_size = strlen(buf);
    writefile(passwd_Inode,passwd_Inode_Addr,buf);	//���޸ĺ��passwdд���ļ���

    //ȡ��shadow�ļ�����
    j = 0;
    for(i=0;i<shadow_Inode.i_size;i++){
        if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
            //���µĴ��̿�
            fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    //����shadow��Ŀ
    sprintf(buf+strlen(buf),"%s:%s\n",username,passwd);	//������Ŀ���û���������
    shadow_Inode.i_size = strlen(buf);
    writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���


    //ȡ��group�ļ�����
    j = 0;
    for(i=0;i<group_Inode.i_size;i++){
        if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
            //���µĴ��̿�
            fseek(fr,group_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    //����group����ͨ�û��б�
    if(buf[strlen(buf)-2]==':')
        sprintf(buf+strlen(buf)-1,"%s\n",username);	//���������û�
    else
        sprintf(buf+strlen(buf)-1,",%s\n",username);	//���������û�
    group_Inode.i_size = strlen(buf);
    writefile(group_Inode,group_Inode_Addr,buf);	//���޸ĺ������д���ļ���

    //�ָ��ֳ����ص�ԭ����Ŀ¼
    strcpy(Cur_User_Name,bak_Cur_User_Name);
    strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
    Cur_Dir_Addr = bak_Cur_Dir_Addr;
    strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

    printf("�û�ע��ɹ�\n");
    return true;
}



bool userdel(char username[])	//�û�ɾ��
{
    if(strcmp(Cur_User_Name,"root")!=0){
        printf("Ȩ�޲���:����ҪrootȨ��\n");
        return false;
    }
    if(strcmp(username,"root")==0)
    {
        printf("�޷�ɾ��root�û�\n");
        return false;
    }
    int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
    int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
    int group_Inode_Addr = -1;	//�û����ļ�inode��ַ
    Inode passwd_Inode;		  //�û��ļ���inode
    Inode shadow_Inode;		  //�û������ļ���inode
    Inode group_Inode;		  //�û����ļ�inode
    //ԭ����Ŀ¼������
    char bak_Cur_User_Name[110];
    char bak_Cur_User_Dir_Name[310];
    int bak_Cur_Dir_Addr;
    char bak_Cur_Dir_Name[310];

    Inode cur_dir_inode;	//��ǰĿ¼��inode
    int i,j;
    DirItem dirlist[16];	//��ʱĿ¼

    //�����ֳ����ص���Ŀ¼

    strcpy(bak_Cur_User_Name,Cur_User_Name);
    strcpy(bak_Cur_User_Dir_Name,Cur_User_Dir_Name);
    bak_Cur_Dir_Addr = Cur_Dir_Addr;
    strcpy(bak_Cur_Dir_Name,Cur_Dir_Name);

    //�ص���Ŀ¼
    gotoRoot();

    //�����û�Ŀ¼
    cd(Cur_Dir_Addr,"etc");

    //�����û�����
    //char passwd[100] = {0};
    //inPasswd(passwd);	//��������

    //�ҵ�passwd��shadow�ļ�inode��ַ����ȡ����׼��������Ŀ

    //ȡ����ǰĿ¼��inode
    fseek(fr,Cur_Dir_Addr,SEEK_SET);
    fread(&cur_dir_inode,sizeof(Inode),1,fr);

    //����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
    for(i=0;i<10;i++){
        if(cur_dir_inode.i_dirBlock[i]==-1)
        {
            continue;
        }
        //����ȡ�����̿�
        fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        for(j=0;j<16;j++){	//����Ŀ¼��
            if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
                strcmp(dirlist[j].itemName,"shadow")==0 ||
                strcmp(dirlist[j].itemName,"group")==0){
                Inode tmp;	//��ʱinode
                //ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ((tmp.i_mode>>9)&1)==0 ){
                    //���ļ�
                    //�б���passwd�ļ�����shadow�ļ�
                    if( strcmp(dirlist[j].itemName,"passwd")==0 ){
                        passwd_Inode_Addr = dirlist[j].inodeAddr;
                        passwd_Inode = tmp;
                    }
                    else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
                        shadow_Inode_Addr = dirlist[j].inodeAddr;
                        shadow_Inode = tmp;
                    }
                    else if(strcmp(dirlist[j].itemName,"group")==0 ){
                        group_Inode_Addr = dirlist[j].inodeAddr;
                        group_Inode = tmp;
                    }
                }
            }
        }
        if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1&&group_Inode_Addr!=-1)	//���ҵ���
            break;
    }

    //����passwd�ļ������Ƿ�����û�username
    char buf[100000];	//���100K���ݴ�passwd���ļ�����
    char buf2[600];		//�ݴ���̿�����
    j = 0;	//���̿�ָ��
    //ȡ��passwd�ļ�����
    for(i=0;i<passwd_Inode.i_size;i++)

    {
        if(i%superblock->s_BLOCK_SIZE==0)
        {	//������
            //���µĴ��̿�
            fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char buf3[600];
    buf3[0]='\n';
    j=1;
    if(strcmp(username,"root")!=0)
    {
        for(i=0;i<strlen(username);i++)
        {
            buf3[j]=username[i];
            j++;
        }

        buf3[j++]=':';
        buf3[j]='\0';

        if(strstr(buf,buf3)==NULL)
        {
            //û�ҵ����û�
            printf("�û�������\n");

            //�ָ��ֳ����ص�ԭ����Ŀ¼
            strcpy(Cur_User_Name,bak_Cur_User_Name);
            strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
            Cur_Dir_Addr = bak_Cur_Dir_Addr;
            strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
            return false;
        }
    }





    //������ڣ���passwd��shadow��group��ɾ�����û�����Ŀ
    //ɾ��passwd��Ŀ
    char *p = strstr(buf,buf3);
    p++;
    *p = '\0';
    while((*p)!='\n')	//�ճ��м�Ĳ���
        p++;
    p++;
    strcat(buf,p);
    passwd_Inode.i_size = strlen(buf);	//�����ļ���С
    writefile(passwd_Inode,passwd_Inode_Addr,buf);	//���޸ĺ��passwdд���ļ���

    //ȡ��shadow�ļ�����
    j = 0;
    for(i=0;i<shadow_Inode.i_size;i++)
    {
        if(i%superblock->s_BLOCK_SIZE==0)
        {	//������������̿�
            //���µĴ��̿�
            fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char buf4[600];
    buf4[0]='\n';
    j=1;
    if(strcmp(username,"root")!=0)
    {
        for(i=0;i<strlen(username);i++)
        {
            buf4[j]=username[i];
            j++;
        }
        buf4[j++]=':';
        buf4[j]='\0';
    }


    if(strstr(buf,buf4)!=NULL)
    {
        //ɾ��shadow��Ŀ
        p = strstr(buf,buf4);
        p++;
        *p = '\0';
        while((*p)!='\n')	//�ճ��м�Ĳ���
            p++;
        p++;
        strcat(buf,p);
        shadow_Inode.i_size = strlen(buf);	//�����ļ���С
        writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���

    }








    //ȡ��group�ļ�����
    j = 0;
    for(i=0;i<group_Inode.i_size;i++)
    {
        if(i%superblock->s_BLOCK_SIZE==0)
        {	//������������̿�
            //���µĴ��̿�
            fseek(fr,group_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';
    //printf("%s",buf);

    //ɾ��group����ͨ�û��б�
    char buf5[100];
    j=0;
    if(strcmp(username,"root")!=0)
    {
        p=strstr(buf,username);
        p--;
        if(*p==':')
            buf5[j++]=':';

        else
            buf5[j++]=',';
        for(i=0;i<strlen(username);i++)
            buf5[j++]=username[i];
        printf("%s",buf5);
        char *q=strstr(buf,username);
        int k=0;
        for(k=0;k<strlen(username);k++)
            q++;

        if((*q)=='\n')
            buf5[j++]='\n';
        else
            buf5[j++]=',';

        buf5[j]='\0';

    }


    if(strstr(buf,buf5)!=NULL)
    {int flag=0;
        p = strstr(buf,buf5);//����username��buf�е���ʼ��ַ

        if((*p)==':')
        {
            p++;
            flag=1;
        }
        *p = '\0';
        while((*p)!='\n' && (*p)!=',')	//�ճ��м�Ĳ���
            p++;
        if(flag==1)
            p++;

        strcat(buf,p);
        group_Inode.i_size = strlen(buf);	//�����ļ���С
        writefile(group_Inode,group_Inode_Addr,buf);	//���޸ĺ������д���ļ���
    }
    //�ָ��ֳ����ص�ԭ����Ŀ¼
    strcpy(Cur_User_Name,bak_Cur_User_Name);
    strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
    Cur_Dir_Addr = bak_Cur_Dir_Addr;
    strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

    //ɾ���û�Ŀ¼
    Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
    strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
    cd(Cur_Dir_Addr,"home");
    rmdir(Cur_Dir_Addr,username);

    //�ָ��ֳ����ص�ԭ����Ŀ¼
    strcpy(Cur_User_Name,bak_Cur_User_Name);
    strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
    Cur_Dir_Addr = bak_Cur_Dir_Addr;
    strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

    printf("�û���ɾ��\n");
    return true;

}

bool chpwd( char username[])
//root �û����Ը������û������� ���Լ�ֻ�ܸ��Լ������룬��������֮��ص���ǰĿ¼
{
    char passwd1[100],passwd2[100];
    int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
    Inode shadow_Inode;		  //�û������ļ���inode
    //ԭ����Ŀ¼������

    char bak_Cur_User_Name[110];
    char bak_Cur_User_Dir_Name[310];
    int bak_Cur_Dir_Addr;
    char bak_Cur_Dir_Name[310];

    Inode cur_dir_inode;	//��ǰĿ¼��inode
    int i,j;
    DirItem dirlist[16];	//��ʱĿ¼

    //�����ֳ����ص���Ŀ¼������õ�ǰ���û���Ϣ

    strcpy(bak_Cur_User_Name,Cur_User_Name);
    strcpy(bak_Cur_User_Dir_Name,Cur_User_Dir_Name);
    bak_Cur_Dir_Addr = Cur_Dir_Addr;
    strcpy(bak_Cur_Dir_Name,Cur_Dir_Name);
    //�ص���Ŀ¼
    gotoRoot();

    //�����û�Ŀ¼
    cd(Cur_Dir_Addr,"etc");

    //�ҵ�shadow�ļ�inode��ַ����ȡ����׼��������Ŀ

    //ȡ����ǰĿ¼��inode
    fseek(fr,Cur_Dir_Addr,SEEK_SET);
    fread(&cur_dir_inode,sizeof(Inode),1,fr);

    //����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
    for(i=0;i<10;i++)
    {
        if(cur_dir_inode.i_dirBlock[i]==-1)
        {
            continue;
        }
        //����ȡ�����̿�
        fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        for(j=0;j<16;j++)
        {	//����Ŀ¼��
            if( strcmp(dirlist[j].itemName,"shadow")==0)
            {//�ҵ�shadow��Ŀ
                Inode tmp;	//��ʱinode
                //ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ((tmp.i_mode>>9)&1)==0 )
                {//���ļ�

                    if(strcmp(dirlist[j].itemName,"shadow")==0 )
                    {
                        shadow_Inode_Addr = dirlist[j].inodeAddr;
                        shadow_Inode = tmp;
                        break;
                    }

                }

            }

        }

    }
    char buf[100000];	//���100K���ݴ�passwd���ļ�����
    char buf2[600];		//�ݴ���̿�����
    j = 0;
    for(i=0;i<shadow_Inode.i_size;i++)
    {
        if(i%superblock->s_BLOCK_SIZE==0)
        {	//������������̿�
            //���µĴ��̿�
            fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);
            fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
            j = 0;
        }
        buf[i] = buf2[j++];
    }
    buf[i] = '\0';

    char buf4[600];
    buf4[0]='\n';
    j=1;
    for(i=0;i<strlen(username);i++)
    {
        buf4[j]=username[i];
        j++;
    }
    buf4[j++]=':';
    buf4[j]='\0';

    if(strcmp(bak_Cur_User_Name,"root")!=0&&strcmp(bak_Cur_User_Name,username)!=0)
    {
        printf("Ȩ�޲���:����ҪrootȨ��\n");
        return false;
    }
    if(strcmp(bak_Cur_User_Name,"root")==0)
    {
        //�����ǰ�û���root�û��Ļ� �����޸������û�������


        if(strstr(buf,buf4)!=NULL)
        {

            printf("����root�û��������޸��Լ��������û�������\n");
            printf("������Ҫ�޸ĵ����룺\n");
            inPasswd( passwd1);
            //if(scanf("%s",passwd1)!=0)

            //ɾ��shadow��Ŀ
            char *p = strstr(buf,buf4);
            while((*p)!=':')
                p++;
            p++;
            *p='\0';
            while((*p)!='\n')
            {

                p++;
            }
            strcat(passwd1,p);
            strcat(buf,passwd1);

            shadow_Inode.i_size = strlen(buf);	//�����ļ���С
            writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���
        }

        else
        {
            printf("�û������ڣ��޷��޸�����\n");
            return false ;
        }

    }
    if(strcmp(bak_Cur_User_Name,username)==0)
    {

        printf("����������ǰ������:\n");
        inPasswd(passwd2);


        if(strstr(buf,buf4)!=NULL)
        {
            //�ҵ��û���������ʼλ��
            char *p = strstr(buf,buf4);
            while((*p)!=':')
                p++;
            p++;

            char *q;
            q=p;

            i=0;
            char buf3[100];//��ʱ�洢���ڵ�����
            while((*q)!='\n')
            {

                buf3[i++]=*q;
                q++;
            }
            buf3[i]='\0';
            *p='\0';
            p++;
            printf("@@@@@@@@%s",buf3);
            if(strcmp(buf3,passwd2)==0)
            {
                printf("��ǰ�����������ȷ���������µ�����:\n");
                int k=0;
                while(k<5)
                {
                    inPasswd(passwd2);
                    k++;
                    if(strlen(passwd2)<5)
                        printf("����̫�򵥣����������룺\n");
                    else

                    { strcat(passwd2,p);
                        strcat(buf,passwd2);

                        shadow_Inode.i_size = strlen(buf);	//�����ļ���С
                        writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���
                        break;
                    }


                }
            }
            else

            {
                printf("��ǰ�������������������µ�����:\n");
                return false;

            }

        }



        //�ж�����������Ƿ���ȷ
        //�������ȷ�������� 5�λ���
        //���������ȷ�������µ�����
        //�޸�shadow �ļ���д�ش���
    }
    strcpy(Cur_User_Name,bak_Cur_User_Name);
    strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
    Cur_Dir_Addr = bak_Cur_Dir_Addr;
    strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
    return true;

}

void chmod(int parinoAddr,char name[],int pmode)	//�޸��ļ���Ŀ¼Ȩ��
{
    if(strlen(name)>=MAX_NAME_SIZE){
        printf("�������Ŀ¼������\n");
        return ;
    }
    if(strcmp(name,".")==0 || strcmp(name,"..")==0){
        printf("�������\n");
        return ;
    }
    //ȡ�����ļ���Ŀ¼inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //����ȡ�����̿�
    int i = 0,j;
    DirItem dirlist[16] = {0};
    while(i<160){
        if(cur.i_dirBlock[i/16]==-1){
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++){
            if( strcmp(dirlist[j].itemName,name)==0 ){	//�ҵ���Ŀ¼�����ļ�
                //ȡ����Ӧ��inode
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                fflush(fr);
                goto label;
            }
        }
        i++;
    }
    label:
    if(i>=160){
        printf("�ļ�������\n");
        return ;
    }

    //�ж��Ƿ��Ǳ��û�
    if(strcmp(Cur_User_Name,fileInode.i_uname)!=0 && strcmp(Cur_User_Name,"root")!=0){
        printf("Ȩ�޲���\n");
        return ;
    }

    //��inode��mode���Խ����޸�
    fileInode.i_mode  = (fileInode.i_mode>>9<<9) | pmode;	//�޸�Ȩ��

    //��inodeд�ش���
    fseek(fw,dirlist[j].inodeAddr,SEEK_SET);
    fwrite(&fileInode,sizeof(Inode),1,fw);
    fflush(fw);
}

void touch(int parinoAddr,char name[],char buf[])	//touch������ļ��������ַ�
{
    //���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
    if(strlen(name)>=MAX_NAME_SIZE)
    {
        printf("��������ļ�������\n");
        return ;
    }
    //��������ͬ���ļ����еĻ���ʾ�����˳�����û�еĻ�������һ�����ļ�
    DirItem dirlist[16];	//��ʱĿ¼�嵥

    //�������ַȡ��inode
    Inode cur,fileInode;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    int i = 0,j;
    int dno;
    int fileInodeAddr = -1;	//�ļ���inode��ַ
    while(i<160){
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i/16;	//�ڵڼ���ֱ�ӿ���

        if(cur.i_dirBlock[dno]==-1){
            i+=16;
            continue;
        }
        fseek(fr,cur.i_dirBlock[dno],SEEK_SET);
        fread(dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++){
            if(strcmp(dirlist[j].itemName,name)==0 ){
                //������ȡ��inode���ж��Ƿ����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileInode,sizeof(Inode),1,fr);
                if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ�����������ʾ�����˳�����
                    printf("�ļ��Ѵ���\n");
                    return ;
                }
            }
            i++;
        }
    }

    //�ļ������ڣ�����һ�����ļ�
    if( ((cur.i_mode>>filemode>>1)&1)==1 || strcmp(Cur_Host_Name,"root")==0){
        //��д�����Դ����ļ�
        buf[0] = '\0';
        create(parinoAddr,name,buf);	//�����ļ�
    }
    else
    {
        printf("Ȩ�޲��㣺��д��Ȩ��\n");
        return ;
    }

}

void help()	//��ʾ���������嵥
{
    printf("ls - ��ʾ��ǰĿ¼�嵥\n");
    printf("cd - ����Ŀ¼\n");
    printf("mkdir - ����Ŀ¼\n");
    printf("rmdir - ɾ��Ŀ¼\n");
    printf("super - �鿴������\n");
    printf("inode - �鿴inodeλͼ\n");
    printf("block - �鿴blockλͼ\n");
    printf("cat - ����ļ�\n");
    printf("mv - �ƶ��ļ�\n");
    printf("vi - vi�༭��\n");
    printf("tree - ����״�ṹ��ʾ���\n");
    printf("touch - ����һ�����ļ�\n");
    printf("rm - ɾ���ļ�\n");
    printf("cls - ����\n");
    printf("logout - �û�ע��\n");
    printf("useradd - �����û�\n");
    printf("userdel - ɾ���û�\n");
    printf("chpwd - ��������\n");
    printf("chmod - �޸��ļ���Ŀ¼Ȩ��\n");
    printf("help - ��ʾ�����嵥\n");
    printf("exit - �˳�ϵͳ\n");
    return ;
}

void cmd(char str[])	//�������������
{
    char p1[100];
    char p2[100];
    char p3[100];
    char p4[100];
    char buf[100000];	//���100K
    int tmp = 0;
    int i;
    sscanf(str,"%s",p1);

    if(strcmp(p1,"ls")==0){
        ls(Cur_Dir_Addr);	//��ʾ��ǰĿ¼
    }
    else if(strcmp(p1,"cd")==0){
        sscanf(str,"%s%s",p1,p2);
        cd(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"mkdir")==0){
        sscanf(str,"%s%s",p1,p2);
        mkdir(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"rmdir")==0){
        sscanf(str,"%s%s",p1,p2);
        rmdir(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"super")==0){
        printSuperBlock();
    }
    else if(strcmp(p1,"inode")==0){
        printInodeBitmap();
    }
    else if(strcmp(p1,"block")==0){
        sscanf(str,"%s%s",p1,p2);
        tmp = 0;
        if('0'<=p2[0] && p2[0]<='9'){
            for(i=0;p2[i];i++)
                tmp = tmp*10+p2[i]-'0';
            printBlockBitmap(tmp);
        }
        else
            printBlockBitmap();
    }
    else if(strcmp(p1,"cat")==0){
        sscanf(str,"%s%s%s",p1,p2,p3);
        cat(Cur_Dir_Addr,p2,p3);
        p3[0] = '\0';
    }
    else if(strcmp(p1,"mv")==0)
    {
        sscanf(str,"%s%s%s%s",p1,p2,p3,p4);
        mv(Cur_Dir_Addr,p2,p3,p4);
    }
    else if(strcmp(p1,"vi")==0){	//����һ���ļ�
        sscanf(str,"%s%s",p1,p2);
        vi(Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
    }
    else if(strcmp(p1,"tree")==0){
        p2[0]='\0';
        sscanf(str,"%s%s",p1,p2);
        if(strcmp(p2,"")==0)
        {
            PrintDirentStruct(Cur_Dir_Name,Cur_Dir_Addr,0);   //��״��ʾ
        } else
        {
            //findInodeAddr(Cur_Dir_Addr,p2);
            findfile(Cur_Dir_Addr,p2);
            //printf("Cur_DIr:%dTree_dir:%d\n",Cur_Dir_Addr1,Tree_Dir_Addr);
            PrintDirentStruct(p2,Cur_Dir_Addr1,0);//findInodeAddr(Cur_Dir_Addr,p2)
        }

    }
    else if(strcmp(p1,"touch")==0){
        sscanf(str,"%s%s%s",p1,buf,p2);
        touch (Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
    }
    else if(strcmp(p1,"rm")==0){	//ɾ��һ���ļ�
        sscanf(str,"%s%s",p1,p2);
        del(Cur_Dir_Addr,p2);
    }
    else if(strcmp(p1,"cls")==0){
        system("cls");
    }
    else if(strcmp(p1,"logout")==0){
        logout();
    }
    else if(strcmp(p1,"useradd")==0){
        p2[0] = '\0';
        sscanf(str,"%s%s",p1,p2);
        if(strlen(p2)==0){
            printf("��������\n");
        }
        else{
            useradd(p2);
        }
    }
    else if(strcmp(p1,"userdel")==0){
        p2[0] = '\0';
        sscanf(str,"%s%s",p1,p2);
        if(strlen(p2)==0){
            printf("��������\n");
        }
        else{
            userdel(p2);
        }
    }
    else if(strcmp(p1,"chpwd")==0)
    {    p2[0] = '\0';
        sscanf(str,"%s%s",p1,p2);
        if(strlen(p2)==0)
        {
            printf("��������\n");
        }
        else{
            chpwd(p2);
        }

    }
    else if(strcmp(p1,"chmod")==0){
        p2[0] = '\0';
        p3[0] = '\0';
        sscanf(str,"%s%s%s",p1,p2,p3);
        if(strlen(p2)==0 || strlen(p3)==0){
            printf("��������\n");
        }
        else{
            tmp = 0;
            for(i=0;p3[i];i++)
                tmp = tmp*8+p3[i]-'0';
            chmod(Cur_Dir_Addr,p2,tmp);
        }
    }
    else if(strcmp(p1,"help")==0){
        help();
    }
    else if(strcmp(p1,"format")==0){
        if(strcmp(Cur_User_Name,"root")!=0){
            printf("Ȩ�޲��㣺����ҪrootȨ��\n");
            return ;
        }
        Ready();
        logout();
    }
    else if(strcmp(p1,"exit")==0){
        printf("�˳�FileSystem\n");
        exit(0);
    }
    else{
        printf("��Ǹ��û�и�����\n");
    }
    return ;
}
//�Զ������������
void my_error(const char *strerr)
{
    perror(strerr);
    exit(1);
}
//���Ŀ¼�ṹ
void PrintDirentStruct(char Cur_Dir_Name_sub[],int Cur_Dir_Addr_sub, int level) {
    //��õ�ǰĿ¼��
    DirItem dirlist[16];
    char direntName[310];
    strcpy(direntName, Cur_Dir_Name_sub);
    //����һ��inode
    Inode cur, fileInode;
    fseek(fr, Cur_Dir_Addr_sub, SEEK_SET);
    fread(&cur, sizeof(Inode), 1, fr);
    //DIR *p_dir = NULL;
    int i = 0, j;
    int dno;
    //int fileInodeAddr = -1;    //�ļ���inode��ַ
    while (i < 160) {
        //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
        dno = i / 16;    //�ڵڼ���ֱ�ӿ���

        if (cur.i_dirBlock[dno] == -1) {
            i += 16;
            continue;
        }
        fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
        fread(dirlist, sizeof(dirlist), 1, fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for (j = 0; j < 16; j++) {//�ж��Ƿ����ļ�
            if(strcmp(dirlist[j].itemName,"") != 0){
                fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
                fread(&fileInode, sizeof(Inode), 1, fr);
                if ((((fileInode.i_mode >> 9) & 1) == 0))//�ж����ļ���ֱ�����
                {

                    int calc;
                    for (calc = 0; calc < level; calc++) {
                        printf("|");
                        printf("     ");
                    }
                    printf("|--- ");
                    printf("%s\n", dirlist[j].itemName);
                } else if ((((fileInode.i_mode >> 9) & 1) == 1) && (strcmp(dirlist[j].itemName,".")!=0) &&
                           ( strcmp(dirlist[j].itemName,"..")!=0))//�ж���Ŀ¼
                {
                    int count;
                    for (count = 0; count < level; count++) {
                        printf("|");
                        printf("     ");
                    }
                    printf("|--- ");
                    //printf("%d\n",level);
                    printf("%s\n", dirlist[j].itemName);
                    PrintDirentStruct(dirlist[j].itemName, dirlist[j].inodeAddr, level + 1);



                } else//�ǡ�.�����ߡ�..��
                {
                    //printf("hhhh\n");
                   // continue;//continue��������

                }
            }



            i++;
        }
    }

}
int findInodeAddr(int parinoAddr,char name[])	//�ҵ��ļ�������Ӧ��inode��ַ
{   //���봫����name=/home/root/rank1
    char *name1,*name2;
    int parinoAddr_sub;
    if(strlen(name)==1 && strcmp(name,"/")==0)
    {
        //���ظ�Ŀ¼inode��ַ
        return Root_Dir_Addr;
    }

    name2=strstr(name,"/");//name2=/home/root/rank1
    if(name2!=NULL && name[0]!='/')
    {
        name2++;
        name1=strtok(name,"/");
    }
    else if(name[0]=='/')
    {
        //Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        //strcpy(Cur_Dir_Name,"/");
        Tree_Dir_Addr = Root_Dir_Addr;
        parinoAddr_sub=Tree_Dir_Addr;
        name++;//name=home/root/rank1
        name2=strstr(name,"/");//name2=/root/rank1
        if(name2!=NULL)
        {
            name2++;//name2=root/rank1
            name1=strtok(name,"/");//name1=home
        }
        else
            name1=name;
    }
    else
        name1=name;

    //ȡ����ǰĿ¼��inode
    Inode cur;
    fseek(fr,parinoAddr_sub,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);//�ҵ��˸���inode

    //����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
    int i = 0;

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    //i_mode����filemodeλ�����λ�������ڵ�ǰ�û����ݵĶ��ڸ��ļ��Ĵ�ȡȨ��
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    while(i<160)
    {
        DirItem dirlist[16] = {0};
        if(cur.i_dirBlock[i/16]==-1)
        {
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++)
        {
            if(strcmp(dirlist[j].itemName,name1)==0)
            {
                Inode tmp;
                //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ( (tmp.i_mode>>9) & 1 ) == 1 )
                {
                    //�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
                    //Ŀ¼�Ľ���Ȩ�޾��Ǹ�Ŀ¼�ļ���ִ��Ȩ��
                    if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 )
                    {	//root�û�����Ŀ¼�����Բ鿴
                        //û��ִ��Ȩ��
                        printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
                        return -1;
                    }

                    //�ҵ���Ŀ¼������Ŀ¼��������ǰTreeĿ¼

                    Tree_Dir_Addr = dirlist[j].inodeAddr;
                    if( strcmp(dirlist[j].itemName,".")==0)
                    {
                        return Cur_Dir_Addr;
                    }
                    else if(strcmp(dirlist[j].itemName,"..")==0)
                    {
                        return dirlist[j].inodeAddr;

                    }
                    else
                    {
                        //if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')
                            //��ǰ�Ĳ���Ŀ¼�Ǹ�Ŀ¼
                            //     strcat(Cur_Dir_Name,"/");//��Cur_Dir_Name��׷���ַ���
                            // strcat(Cur_Dir_Name,dirlist[j].itemName);
                            if(name2!=NULL)
                                findInodeAddr(Tree_Dir_Addr,name2);
                            else
                                return Tree_Dir_Addr;
                    }


                }
                else
                {
                    //�ҵ���Ŀ¼��������Ŀ¼��������
                }

            }

            i++;
        }

    }

    //û�ҵ�
    printf("û�и�Ŀ¼\n");
    return -1;

}


void findfile(int parinoAddr,char name[])	//���뵱ǰĿ¼�µ�nameĿ¼
{
    char *name1,*name2;
    if(strlen(name)==1 && strcmp(name,"/")==0)
    {
        //Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        Cur_Dir_Addr1 = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        //strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
        //ls(Cur_Dir_Addr1);
        return;
    }

    name2=strstr(name,"/");
    if(name2!=NULL && name[0]!='/')
    {
        name2++;
        name1=strtok(name,"/");
    }
    else if(name[0]=='/')
    {
        Cur_Dir_Addr1 = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        //Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
        //strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
        parinoAddr=Cur_Dir_Addr1;
        name++;
        name2=strstr(name,"/");
        if(name2!=NULL)
        {
            name2++;
            name1=strtok(name,"/");
        }
        else
            name1=name;
    }
    else
        name1=name;

    //ȡ����ǰĿ¼��inode
    Inode cur;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);

    //����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
    int i = 0;

    //ȡ��Ŀ¼����
    int cnt = cur.i_cnt;

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    //i_mode����filemodeλ�����λ�������ڵ�ǰ�û����ݵĶ��ڸ��ļ��Ĵ�ȡȨ��
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    while(i<160)
    {
        DirItem dirlist[16] = {0};
        if(cur.i_dirBlock[i/16]==-1)
        {
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);

        //����ô��̿��е�����Ŀ¼��
        int j;
        for(j=0;j<16;j++)
        {
            if(strcmp(dirlist[j].itemName,name1)==0)
            {
                Inode tmp;
                //ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&tmp,sizeof(Inode),1,fr);

                if( ( (tmp.i_mode>>9) & 1 ) == 1 )
                {
                    //�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
                    //Ŀ¼�Ľ���Ȩ�޾��Ǹ�Ŀ¼�ļ���ִ��Ȩ��
                    if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 )
                    {	//root�û�����Ŀ¼�����Բ鿴
                        //û��ִ��Ȩ��
                        printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
                        return ;
                    }

                    //�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼
                    Cur_Dir_Addr1 = dirlist[j].inodeAddr;
                    //Cur_Dir_Addr = dirlist[j].inodeAddr;
                    if( strcmp(dirlist[j].itemName,".")==0)
                    {
                        //��Ŀ¼������
                    }
                    else if(strcmp(dirlist[j].itemName,"..")==0)
                    {
                        //��һ��Ŀ¼
                        /*
                        int k;
                        for(k=strlen(Cur_Dir_Name);k>=0;k--)
                            if(Cur_Dir_Name[k]=='/')
                                break;
                        Cur_Dir_Name[k]='\0';
                        if(strlen(Cur_Dir_Name)==0)//���ڸ�Ŀ¼��cd ..
                            Cur_Dir_Name[0]='/',Cur_Dir_Name[1]='\0';
                        */
                    }
                    else
                    {
                        /*
                        if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')
                        //��ǰ�Ĳ���Ŀ¼�Ǹ�Ŀ¼
                            strcat(Cur_Dir_Name,"/");//��Cur_Dir_Name��׷���ַ���
                        strcat(Cur_Dir_Name,dirlist[j].itemName);
                        */
                        if(name2!=NULL)
                            findfile(Cur_Dir_Addr1,name2);
                        else
                        {
                            //printf("!!!!!");
                            //ls(Cur_Dir_Addr1);
                            return;
                        }
                    }
                    //printf("!!!!!!!!!!!!!!!!!!!!!\n");
                    //ls(Cur_Dir_Addr1);
                    return ;
                }
                else
                {
                    //�ҵ���Ŀ¼��������Ŀ¼��������
                }

            }

            i++;
        }

    }

    //û�ҵ�
    printf("û�и�Ŀ¼\n");
    return ;

}

void mv(int parinoAddr,char name1[],char name2[],char name3[])
{
    //
    if(strlen(name1)>=MAX_NAME_SIZE || strlen(name2)>=MAX_NAME_SIZE)
    {
        printf("��������ļ�������\n");
        return ;
    }
    if(strcmp(name1,".")==0 || strcmp(name1,"..")==0)
    {
        printf("�������\n");
        return ;
    }
    if((strcmp(name1,name2)==0))//���ļ�����ͬ�������κ��޸�
        //����������������ļ���Ŀ¼�ļ�ͬ���������ڴ������ﲻ������
    {
        printf("�ļ�ͬ���������޸ģ�\n");
        return;
    }
    DirItem dirlist[16]={0};	//��ʱĿ¼�嵥
    Inode cur,fileinode1,fileinode2;

    int file1_dirlist,file2_dirlist;
    int file1_dirBlock,file2_dirBlock;//���ļ����ڵ�ֱ�ӿ��±�
    int file1Exist=0,file2Exist=0;

    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);
    fflush(fr);

    int i = 0,j;
    while(i<160)
    {
        if(cur.i_dirBlock[i/16]==-1)
        {
            i+=16;
            continue;
        }
        //ȡ�����̿�
        int parblockAddr = cur.i_dirBlock[i/16];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);

        //����ô��̿��е�����Ŀ¼��
        for(j=0;j<16;j++)
        {
            if( (strcmp(dirlist[j].itemName,name1)==0) )
            {
                //ȡ����Ӧ��inode
                file1_dirBlock=i/16;
                file1_dirlist=j;
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileinode1,sizeof(Inode),1,fr);
                fflush(fr);
                file1Exist=1;
            }
            if( (strcmp(dirlist[j].itemName,name2)==0) )
            {

                file2_dirBlock=i/16;
                file2_dirlist=j;
                //ȡ����Ӧ��inode
                fseek(fr,dirlist[j].inodeAddr,SEEK_SET);
                fread(&fileinode2,sizeof(Inode),1,fr);
                fflush(fr);
                file2Exist=1;
            }
            if (file1Exist==1 && file2Exist==1)
                goto label;
            i++;
        }
    }
    label:
    if(i>=160)
    {
        if(file1Exist==0)
        {
            printf("Դ�ļ������ڣ�\n");
            return ;
        }
    }

    //�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0 )
        filemode = 6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode = 3;
    else
        filemode = 0;

    if( (((fileinode1.i_mode>>filemode>>1)&1)==0) && (strcmp(Cur_User_Name,"root")!=0) )
    {
        //���ƶ��ļ���Ȩ�޶�λд��Ȩ��
        printf("Ȩ�޲��㣺���ƶ���д�룩Ȩ�ޣ�\n");
        return ;
    }

    int dir_flag1=0;//��ʶ�Ƿ���Ŀ¼�ļ�
    if((fileinode1.i_mode>>9)&1==1)
    {
        dir_flag1=1;
    }
    int dir_flag2=0;
    if((fileinode2.i_mode>>9)&1==1)
    {
        dir_flag2=1;
    }

    if(file2Exist==0)//���Ŀ���ļ������ڣ�����Դ�ļ���Ŀ¼�ļ������ļ���ֱ��������
    {
        int parblockAddr = cur.i_dirBlock[file1_dirBlock];
        rename(parblockAddr,file1_dirlist,name2);
        printf("����������\n");
        return ;
    }
    char c;
    if(dir_flag1==0 || dir_flag1==1)//Դ�ļ����ļ�
    {
        if(dir_flag2==0)//Ŀ���ļ�Ҳ���ļ�
        {
            if((strcmp(name3,"-i")==0))
            {
                printf("�Ƿ񸲸�Ŀ���ļ���y/n \n");
                scanf("%c",&c);
                getchar();
                if(c=='n')
                    return ;
            }
            //����Ŀ���ļ�����Դ�ļ�������
            del(parinoAddr,name2);
            int parblockAddr = cur.i_dirBlock[file1_dirBlock];
            rename(parblockAddr,file1_dirlist,name2);
            return ;
        }
        else//Ŀ���ļ���Ŀ¼�ļ�
        {
            cd(parinoAddr,name2);
            DirItem dirlist1[16]={0};
            Inode cur1;
            fseek(fr,Cur_Dir_Addr,SEEK_SET);//Cur_Dir_Addr��name2Ŀ¼�ļ�inode��ַ
            fread(&cur1,sizeof(Inode),1,fr);//cur1��inode�ṹ��
            fflush(fr);
            i = 0;
            int posi = -1,posj = -1;	//�ҵ���Ŀ¼λ��
            int dno;
            while(i<160)
            {
                //160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
                dno = i/16;	//�ڵڼ���ֱ�ӿ���

                if(cur1.i_dirBlock[dno]==-1)
                {
                    i+=16;
                    continue;
                }
                fseek(fr,cur1.i_dirBlock[dno],SEEK_SET);//cur1.i_dirBlock[dno]��ַ��ŵ���name2Ŀ¼�µ��ļ���Ŀ¼��
                fread(&dirlist1,sizeof(dirlist1),1,fr);
                fflush(fr);

                //����ô��̿��е�����Ŀ¼��
                int j;
                for(j=0;j<16;j++)
                {
                    if( posi==-1 && strcmp(dirlist1[j].itemName,"")==0 )
                    {
                        //�ҵ�һ�����м�¼�������ļ����������λ��
                        posi = dno;
                        posj = j;
                    }
                    else if(strcmp(dirlist1[j].itemName,name1)==0)
                        //strcmp������ȷ���0������Ϊ1
                    {
                        //������ȡ��inode���ж��Ƿ����ļ�
                        Inode cur2;
                        fseek(fr,dirlist1[j].inodeAddr,SEEK_SET);
                        fread(&cur2,sizeof(Inode),1,fr);
                        fflush(fr);
                        if( ((cur2.i_mode>>9)&1)==0 )
                        {	//���ļ������������ܴ����ļ�
                            printf("Ŀ��Ŀ¼���ļ��Ѵ���\n");
                            cd(Cur_Dir_Addr,"..");
                            return ;
                        }
                    }
                    i++;
                }
            }
            if(posi!=-1)//posi��Ϊ�ҵ��Ŀ���Ŀ¼�����ڵ�ֱ�ӿ��ַ�±�
            {
                //name2Ŀ¼�µ�.�ļ���inode�ṹ��(��cur1)��cntҪ��1
                cur1.i_cnt++;
                fseek(fw,Cur_Dir_Addr,SEEK_SET);//Cur_Dir_Addr��name2Ŀ¼�ļ���.�ļ���inode�ĵ�ַ
                fwrite(&cur1,sizeof(Inode),1,fw);//cur1��inode�ṹ��
                fflush(fw);
                //��dirlist1��������Ŀ¼�����ڵ�Ŀ¼�����д���һ��Ŀ¼��
                fseek(fr,cur1.i_dirBlock[posi],SEEK_SET);
                fread(&dirlist1,sizeof(dirlist1),1,fr);
                fflush(fr);

                int parblockAddr = cur.i_dirBlock[file1_dirBlock];
                fseek(fr,parblockAddr,SEEK_SET);
                fread(&dirlist,sizeof(dirlist),1,fr);
                fflush(fr);
                strcpy(dirlist1[posj].itemName,name1);
                dirlist1[posj].inodeAddr=dirlist[file1_dirlist].inodeAddr;//Ӧ��name1�ļ���Ӧ��Ŀ¼���inodeAddr

                fseek(fw,cur1.i_dirBlock[posi],SEEK_SET);
                fwrite(&dirlist1,sizeof(dirlist1),1,fw);
                fflush(fw);

                //�ٰ�Դ�ļ���Ӧ��Ŀ¼��ɾ��
                strcpy(dirlist[file1_dirlist].itemName,"");
                dirlist[file1_dirlist].inodeAddr = -1;

                fseek(fw,cur.i_dirBlock[file1_dirBlock],SEEK_SET);
                fwrite(&dirlist,sizeof(dirlist),1,fw);
                fflush(fw);
                //Դ�ļ�����Ŀ¼��inode��cntҪ��1

                cur.i_cnt--;
                fseek(fw,parinoAddr,SEEK_SET);
                fwrite(&cur,sizeof(Inode),1,fw);
                fflush(fw);
            }
            cd(Cur_Dir_Addr,"..");
            return;
        }
    }
}

int rename(int parblockAddr,int file_dirlist,char name[])
{
    DirItem dirlist[16]={0};
    fseek(fr,parblockAddr,SEEK_SET);
    fread(&dirlist,sizeof(dirlist),1,fr);
    fflush(fr);
    strcpy(dirlist[file_dirlist].itemName,name);//Դ�ļ�������
    fseek(fw,parblockAddr,SEEK_SET);
    fwrite(&dirlist,sizeof(dirlist),1,fw);
    fflush(fw);
}

int cat(int parinoAddr,char name[],char p3[])
{
    if(strlen(name)>=MAX_NAME_SIZE)
    {
        printf("�����ļ�������\n");
        return 0;
    }

    Inode cur,filecur;
    int isExist=0;
    fseek(fr,parinoAddr,SEEK_SET);
    fread(&cur,sizeof(Inode),1,fr);
    fflush(fr);

    int nflag=0,n=1;
    if(strcmp(p3,"-n")==0)
    {
        nflag=1;
    }
    p3[0]='\0';

    DirItem dirlist[16]={0};
    int fileinodeAddr;

    int filemode;
    if(strcmp(Cur_User_Name,cur.i_uname)==0)
        filemode=6;
    else if(strcmp(Cur_User_Name,cur.i_gname)==0)
        filemode=3;
    else
        filemode=0;

    int i=0,parblockAddr;
    while(i<10)
    {
        if(cur.i_dirBlock[i]==-1)
        {
            i++;
            continue;
        }
        parblockAddr=cur.i_dirBlock[i];
        fseek(fr,parblockAddr,SEEK_SET);
        fread(&dirlist,sizeof(dirlist),1,fr);
        fflush(fr);
        for(int j=0;j<16;j++)
        {
            if(strcmp(dirlist[j].itemName,name)==0)
            {
                fileinodeAddr=dirlist[j].inodeAddr;
                fseek(fr,fileinodeAddr,SEEK_SET);
                fread(&filecur,sizeof(Inode),1,fr);
                fflush(fr);
                if( (filecur.i_mode>>9)&1==1 )
                    continue;
                else
                {
                    isExist=1;
                    break;
                }
            }
        }
        i++;
        if(isExist==1)
            break;
    }
    if( (filecur.i_mode>>filemode>>2)&1==0 )
    {
        printf("û�ж��ļ�Ȩ�ޣ�\n");
        return 0;
    }

    int sumlen = filecur.i_size;	//�ļ�����
    int getlen = 0;	//ȡ�����ĳ���
    for(i=0;i<10;i++)
    {
        char fileContent[1000] = {0};
        if(filecur.i_dirBlock[i]==-1)
        {
            continue;
        }
        //����ȡ�����̿������
        fseek(fr,filecur.i_dirBlock[i],SEEK_SET);
        fread(fileContent,superblock->s_BLOCK_SIZE,1,fr);	//��ȡ��һ�����̿��С������
        fflush(fr);
        //����ַ���
        int curlen = 0;	//��ǰָ��
        if(nflag==1)
            printf("%d  ",n++);
        while(curlen<superblock->s_BLOCK_SIZE)
        {
            if(getlen>=sumlen)	//ȫ��������
                break;
            printf("%c",fileContent[curlen]);	//�������Ļ
            if(nflag==1&&fileContent[curlen]=='\n'&&curlen!=sumlen-1)
            {
                printf("%d  ",n++);
            }
            curlen++;
            getlen++;
        }
        if(getlen>=sumlen)
            break;
    }
    printf("\n");
}
