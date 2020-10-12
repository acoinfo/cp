/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: cp.c
**
** ��   ��   ��: Hui.Kai (�ݿ�)
**
** �ļ���������: 2020 �� 06 �� 12 ��
**
** ��        ��: �ݹ鸴�ƹ���
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <stdio.h>
/*********************************************************************************************************
** ��������: __buildDstName
** ��������: ����Ŀ���ļ���
** �䡡��  : pcSrc         Դ�ļ�
**           pcDstDir      Ŀ��Ŀ¼
**           pcBuffer      �������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __buildDstName (PCHAR  pcSrc, PCHAR  pcDstDir, PCHAR  pcBuffer)
{
    size_t  stSrcLen    = lib_strlen(pcSrc);
    size_t  stDstDirLen = lib_strlen(pcDstDir);
    PCHAR   pcSrcName   = LW_NULL;

    if (pcSrc[stSrcLen - 1] == PX_DIVIDER) {
        pcSrc[stSrcLen - 1]  = PX_EOS;
    }

    pcSrcName = lib_rindex(pcSrc, PX_DIVIDER);

    if (pcSrcName == LW_NULL) {
        pcSrcName =  (PCHAR)pcSrc;
    } else {
        pcSrcName++;
    }

    lib_strlcpy(pcBuffer, pcDstDir, MAX_FILENAME_LENGTH - 1);

    if (pcBuffer[stDstDirLen - 1] != PX_DIVIDER) {
        pcBuffer[stDstDirLen]      = PX_DIVIDER;
        pcBuffer[stDstDirLen + 1]  = PX_EOS;
    }

    lib_strlcat(pcBuffer, pcSrcName, MAX_FILENAME_LENGTH);
}
/*********************************************************************************************************
** ��������: __tshellFsCmdCpFile
** ��������: �����ļ�
** �䡡��  : pcSrc        Դ�ļ�
**           pcDest       Ŀ���ļ�
**           bForce       ǿ�ƿ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellFsCmdCpFile (PCHAR  pcSrc, PCHAR  pcDest, BOOL  bForce)
{
#define __LW_CP_BUF_SZ  (128 * LW_CFG_KB_SIZE)

             INT        iError = PX_ERROR;
    REGISTER INT        iFdSrc;
    REGISTER INT        iFdDst = PX_ERROR;

             CHAR       cDstFile[MAX_FILENAME_LENGTH] = "\0";

    REGISTER ssize_t    sstRdNum;
    REGISTER ssize_t    sstWrNum;

             CHAR       cTemp[128];
             PCHAR      pcBuffer = cTemp;
             size_t     stBuffer = sizeof(cTemp);
             size_t     stOptim;

    struct   stat       statFile;
    struct   stat       statDst;

    iFdSrc = open(pcSrc, O_RDONLY, 0);                                  /*  ��Դ�ļ�                  */
    if (iFdSrc < 0) {
        fprintf(stderr, "can not open source file!\n");
        return  (PX_ERROR);
    }

    iError = fstat(iFdSrc, &statFile);                                  /*  ���Դ�ļ�����              */
    if (iError < 0) {
        goto    __error_handle;
    }

    iError = stat(pcDest, &statDst);
    if (iError == ERROR_NONE) {
        if (S_ISDIR(statDst.st_mode)) {
            __buildDstName(pcSrc, pcDest, cDstFile);                    /*  ����Ŀ���ļ�·��            */
        }
    }
    if (cDstFile[0] == PX_EOS) {
        lib_strlcpy(cDstFile, pcDest, MAX_FILENAME_LENGTH);
    }

    if (!bForce) {
        iError = access(cDstFile, 0);                                   /*  ���Ŀ���ļ��Ƿ����        */
        if (iError == ERROR_NONE) {
__re_select:
            printf("destination file is exist, overwrite? (Y/N)\n");
            read(0, cTemp, 128);
            if ((cTemp[0] == 'N') || (cTemp[0] == 'n')) {               /*  ������                      */
                iError = PX_ERROR;
                goto    __error_handle;

            } else if ((cTemp[0] != 'Y') && (cTemp[0] != 'y')) {        /*  ѡ�����                    */
                goto    __re_select;

            } else {                                                    /*  ѡ�񸲸�                    */
                unlink(cDstFile);                                       /*  ɾ��Ŀ���ļ�                */
            }
        }
    } else {
        iError = access(cDstFile, 0);                                   /*  ���Ŀ���ļ��Ƿ����        */
        if (iError == ERROR_NONE) {
            unlink(cDstFile);                                           /*  ɾ��Ŀ���ļ�                */
        }
    }
                                                                        /*  ����Ŀ���ļ�                */
    iFdDst = open(cDstFile, (O_WRONLY | O_CREAT | O_TRUNC), DEFFILEMODE);
    if (iFdDst < 0) {
        close(iFdSrc);
        fprintf(stderr, "can not open destination file!\n");
        return  (PX_ERROR);
    }

    stOptim = (UINT)__MIN(__LW_CP_BUF_SZ, statFile.st_size);            /*  ���㻺����                  */
    if (stOptim > 128) {
        pcBuffer = (PCHAR)__SHEAP_ALLOC(stOptim);                       /*  ���仺����                  */
        if (pcBuffer == LW_NULL) {
            pcBuffer =  cTemp;                                          /*  ʹ�þֲ���������            */

        } else {
            stBuffer =  stOptim;
        }
    }

    for (;;) {                                                          /*  ��ʼ�����ļ�                */
        sstRdNum = read(iFdSrc, pcBuffer, stBuffer);
        if (sstRdNum > 0) {
            sstWrNum = write(iFdDst, pcBuffer, (size_t)sstRdNum);
            if (sstWrNum != sstRdNum) {                                 /*  д���ļ�����                */
                fprintf(stderr, "can not write destination file! error: %s\n", lib_strerror(errno));
                iError = PX_ERROR;
                break;
            }
        } else if (sstRdNum == 0) {                                     /*  �������                    */
            iError = ERROR_NONE;
            break;

        } else {
            iError = PX_ERROR;                                          /*  ��ȡ���ݴ���                */
            break;
        }
    }

__error_handle:
    close(iFdSrc);
    if (iFdDst >= 0) {
        fchmod(iFdDst, statFile.st_mode);                               /*  ����Ϊ��Դ�ļ���ͬ�� mode   */
        close(iFdDst);
    }

    if (iError == ERROR_NONE) {                                         /*  �������                    */
        struct utimbuf  utimbDst;

        utimbDst.actime  = statFile.st_atime;                           /*  �޸ĸ��ƺ��ļ���ʱ����Ϣ    */
        utimbDst.modtime = statFile.st_mtime;

        utime(cDstFile, &utimbDst);                                     /*  �����ļ�ʱ��                */
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __tshellFsCmdCpDir
** ��������: ����Ŀ¼
** �䡡��  : pcSrc        ԴĿ¼
**           pcDest       Ŀ��Ŀ¼
**           bForce       ǿ�Ƹ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellFsCmdCpDir (PCHAR  pcSrc, PCHAR  pcDest, BOOL  bForce)
{
             INT        iError = PX_ERROR;
             CHAR       cSrcFile[MAX_FILENAME_LENGTH] = "\0";
             CHAR       cDstFile[MAX_FILENAME_LENGTH] = "\0";
    struct   stat       statFile;
    struct   stat       statSrcDir;
    struct dirent      *pdirent;
             DIR       *pdir;

    iError = stat(pcSrc, &statSrcDir);                                  /*  ���ԴĿ¼����              */
    if (iError < 0) {
        fprintf(stderr, "can not get source file status!\n");
        return  (PX_ERROR);
    }

    /*
     *  �ݹ鸴��Ŀ¼������
     */
    pdir = opendir(pcSrc);                                              /*  �޷���Ŀ¼                */
    if (pdir == LW_NULL) {
        fprintf(stderr, "can not open directory %s, error: %s\n", pcSrc, lib_strerror(errno));
        return  (PX_ERROR);
    }

    do {
        pdirent = readdir(pdir);
        if (pdirent) {
            if (!lib_strcmp(pdirent->d_name, "..") || !lib_strcmp(pdirent->d_name, ".")) {
                continue;
            }

            __buildDstName(pdirent->d_name, pcSrc, cSrcFile);           /*  ���������ļ�·��            */
            __buildDstName(pdirent->d_name, pcDest, cDstFile);          /*  ���������ļ�·��            */

            iError = stat(cSrcFile, &statFile);
            if (iError == ERROR_NONE) {
                if (S_ISREG(statFile.st_mode)) {
                    iError = __tshellFsCmdCpFile(cSrcFile, cDstFile, bForce);
                } else if (S_ISDIR(statFile.st_mode)) {
                    iError = mkdir(cDstFile, statFile.st_mode);
                    if (iError) {
                        if (errno == EACCES) {
                            fprintf(stderr, "insufficient permissions.\n");
                        } else {
                            fprintf(stderr, "can not make directory %s, error: %s\n",
                                            cDstFile, lib_strerror(errno));
                        }

                        break;
                    }
                    iError = __tshellFsCmdCpDir(cSrcFile, cDstFile, bForce);
                }
            }
        }
    } while (pdirent);

    closedir(pdir);
    chmod(pcDest, statSrcDir.st_mode);

    if (iError == ERROR_NONE) {                                         /*  �������                    */
        struct utimbuf  utimbDst;

        utimbDst.actime  = statSrcDir.st_atime;                         /*  �޸ĸ��ƺ�Ŀ¼��ʱ����Ϣ    */
        utimbDst.modtime = statSrcDir.st_mtime;

        utime(pcDest, &utimbDst);                                       /*  ����Ŀ¼ʱ��                */
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: main
** ��������: ϵͳ���� "cp"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  main (INT  iArgC, PCHAR  ppcArgV[])
{
         PCHAR      pcDest;
         PCHAR      pcSrc;
         INT        iC;
         BOOL       bRecursive = LW_FALSE;
         BOOL       bForce     = LW_FALSE;
         INT        iError     = PX_ERROR;
         CHAR       cDstFile[MAX_FILENAME_LENGTH] = "\0";

struct   stat       statFile;
struct   stat       statDst;


    while ((iC = getopt(iArgC, ppcArgV, "rf")) != EOF) {
        switch (iC) {

        case 'r':
            bRecursive = LW_TRUE;
            break;

        case 'f':
            bForce = LW_TRUE;
            break;
        }
    }

    if (iArgC - optind < 2) {
        fprintf(stderr, "parameter error!\n");
        return  (PX_ERROR);
    }

    pcSrc   = ppcArgV[optind];
    pcDest  = ppcArgV[optind + 1];

    getopt_free();

    if (lib_strcmp(pcSrc, pcDest) == 0) {                               /*  �ļ��ظ�                    */
        fprintf(stderr, "parameter error!\n");
        return  (PX_ERROR);
    }

    iError = stat(pcSrc, &statFile);                                    /*  ���Դ�ļ�����              */
    if (iError < 0) {
        fprintf(stderr, "can not get source file status!\n");
        return  (PX_ERROR);
    }

    if (S_ISREG(statFile.st_mode)) {
        iError = __tshellFsCmdCpFile(pcSrc, pcDest, bForce);
    } else if (S_ISDIR(statFile.st_mode)) {
        if (!bRecursive) {
            fprintf(stderr, "cp: -r not specified!\n");
            return  (PX_ERROR);
        }

        iError = stat(pcDest, &statDst);
        if (iError == ERROR_NONE) {
            if(!S_ISDIR(statDst.st_mode)) {                             /*  Ŀ���ļ�����Ŀ¼            */
                return  (iError);
            }

            __buildDstName(pcSrc, pcDest, cDstFile);                    /*  ԴĿ¼��ΪĿ�����Ŀ¼      */
            iError = mkdir(cDstFile, statFile.st_mode);
            if (iError) {
                if (errno == EACCES) {
                    fprintf(stderr, "insufficient permissions.\n");
                } else {
                    fprintf(stderr, "can not make directory %s, error: %s\n",
                                    cDstFile, lib_strerror(errno));
                }
                return  (iError);
            }
        } else {
            if (API_GetLastError() == ENOENT) {                         /*  Ŀ��Ŀ¼������              */
                iError = mkdir(pcDest, statFile.st_mode);
                if (iError) {
                    if (errno == EACCES) {
                        fprintf(stderr, "insufficient permissions.\n");
                    } else {
                        fprintf(stderr, "can not make directory %s, error: %s\n",
                                         pcDest, lib_strerror(errno));
                    }
                    return  (iError);
                }
            } else {
                return  (iError);
            }
        }

        if (cDstFile[0] != PX_EOS) {
            lib_strlcpy(pcDest, cDstFile, MAX_FILENAME_LENGTH);
        }

        iError = __tshellFsCmdCpDir(pcSrc, pcDest, bForce);
    } else {
        fprintf(stderr, "can not copy this file type!\n");
        errno  = EINVAL;
        iError = PX_ERROR;
    }

    return  (iError);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
