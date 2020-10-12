/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: cp.c
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2020 年 06 月 12 日
**
** 描        述: 递归复制工具
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <stdio.h>
/*********************************************************************************************************
** 函数名称: __buildDstName
** 功能描述: 创建目标文件名
** 输　入  : pcSrc         源文件
**           pcDstDir      目标目录
**           pcBuffer      输出缓冲
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: __tshellFsCmdCpFile
** 功能描述: 复制文件
** 输　入  : pcSrc        源文件
**           pcDest       目的文件
**           bForce       强制拷贝
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    iFdSrc = open(pcSrc, O_RDONLY, 0);                                  /*  打开源文件                  */
    if (iFdSrc < 0) {
        fprintf(stderr, "can not open source file!\n");
        return  (PX_ERROR);
    }

    iError = fstat(iFdSrc, &statFile);                                  /*  获得源文件属性              */
    if (iError < 0) {
        goto    __error_handle;
    }

    iError = stat(pcDest, &statDst);
    if (iError == ERROR_NONE) {
        if (S_ISDIR(statDst.st_mode)) {
            __buildDstName(pcSrc, pcDest, cDstFile);                    /*  生成目标文件路径            */
        }
    }
    if (cDstFile[0] == PX_EOS) {
        lib_strlcpy(cDstFile, pcDest, MAX_FILENAME_LENGTH);
    }

    if (!bForce) {
        iError = access(cDstFile, 0);                                   /*  检测目标文件是否存在        */
        if (iError == ERROR_NONE) {
__re_select:
            printf("destination file is exist, overwrite? (Y/N)\n");
            read(0, cTemp, 128);
            if ((cTemp[0] == 'N') || (cTemp[0] == 'n')) {               /*  不覆盖                      */
                iError = PX_ERROR;
                goto    __error_handle;

            } else if ((cTemp[0] != 'Y') && (cTemp[0] != 'y')) {        /*  选择错误                    */
                goto    __re_select;

            } else {                                                    /*  选择覆盖                    */
                unlink(cDstFile);                                       /*  删除目标文件                */
            }
        }
    } else {
        iError = access(cDstFile, 0);                                   /*  检测目标文件是否存在        */
        if (iError == ERROR_NONE) {
            unlink(cDstFile);                                           /*  删除目标文件                */
        }
    }
                                                                        /*  创建目标文件                */
    iFdDst = open(cDstFile, (O_WRONLY | O_CREAT | O_TRUNC), DEFFILEMODE);
    if (iFdDst < 0) {
        close(iFdSrc);
        fprintf(stderr, "can not open destination file!\n");
        return  (PX_ERROR);
    }

    stOptim = (UINT)__MIN(__LW_CP_BUF_SZ, statFile.st_size);            /*  计算缓冲区                  */
    if (stOptim > 128) {
        pcBuffer = (PCHAR)__SHEAP_ALLOC(stOptim);                       /*  分配缓冲区                  */
        if (pcBuffer == LW_NULL) {
            pcBuffer =  cTemp;                                          /*  使用局部变量缓冲            */

        } else {
            stBuffer =  stOptim;
        }
    }

    for (;;) {                                                          /*  开始拷贝文件                */
        sstRdNum = read(iFdSrc, pcBuffer, stBuffer);
        if (sstRdNum > 0) {
            sstWrNum = write(iFdDst, pcBuffer, (size_t)sstRdNum);
            if (sstWrNum != sstRdNum) {                                 /*  写入文件错误                */
                fprintf(stderr, "can not write destination file! error: %s\n", lib_strerror(errno));
                iError = PX_ERROR;
                break;
            }
        } else if (sstRdNum == 0) {                                     /*  拷贝完毕                    */
            iError = ERROR_NONE;
            break;

        } else {
            iError = PX_ERROR;                                          /*  读取数据错误                */
            break;
        }
    }

__error_handle:
    close(iFdSrc);
    if (iFdDst >= 0) {
        fchmod(iFdDst, statFile.st_mode);                               /*  设置为与源文件相同的 mode   */
        close(iFdDst);
    }

    if (iError == ERROR_NONE) {                                         /*  拷贝完成                    */
        struct utimbuf  utimbDst;

        utimbDst.actime  = statFile.st_atime;                           /*  修改复制后文件的时间信息    */
        utimbDst.modtime = statFile.st_mtime;

        utime(cDstFile, &utimbDst);                                     /*  设置文件时间                */
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __tshellFsCmdCpDir
** 功能描述: 复制目录
** 输　入  : pcSrc        源目录
**           pcDest       目的目录
**           bForce       强制复制
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    iError = stat(pcSrc, &statSrcDir);                                  /*  获得源目录属性              */
    if (iError < 0) {
        fprintf(stderr, "can not get source file status!\n");
        return  (PX_ERROR);
    }

    /*
     *  递归复制目录中内容
     */
    pdir = opendir(pcSrc);                                              /*  无法打开目录                */
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

            __buildDstName(pdirent->d_name, pcSrc, cSrcFile);           /*  生成完整文件路径            */
            __buildDstName(pdirent->d_name, pcDest, cDstFile);          /*  生成完整文件路径            */

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

    if (iError == ERROR_NONE) {                                         /*  拷贝完成                    */
        struct utimbuf  utimbDst;

        utimbDst.actime  = statSrcDir.st_atime;                         /*  修改复制后目录的时间信息    */
        utimbDst.modtime = statSrcDir.st_mtime;

        utime(pcDest, &utimbDst);                                       /*  设置目录时间                */
    }

    return  (iError);
}
/*********************************************************************************************************
** 函数名称: main
** 功能描述: 系统命令 "cp"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    if (lib_strcmp(pcSrc, pcDest) == 0) {                               /*  文件重复                    */
        fprintf(stderr, "parameter error!\n");
        return  (PX_ERROR);
    }

    iError = stat(pcSrc, &statFile);                                    /*  获得源文件属性              */
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
            if(!S_ISDIR(statDst.st_mode)) {                             /*  目标文件不是目录            */
                return  (iError);
            }

            __buildDstName(pcSrc, pcDest, cDstFile);                    /*  源目录作为目标的子目录      */
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
            if (API_GetLastError() == ENOENT) {                         /*  目标目录不存在              */
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
