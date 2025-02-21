/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <log.h>
#include <compiler.h>
#include <kpmodule.h>
#include <hook.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <asm/current.h>
#include <linux/compiler.h>
#include "hide_mount.h"

KPM_NAME("hide_mount");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("test");
KPM_DESCRIPTION("test");


struct mm_struct *(*get_task_mm)(struct task_struct *task) = 0;
void *show_vfsmnt_addr;
void *show_mountinfo_addr;
void *show_vfsstat_addr;
char target_path[TARGET_PATH_NUM][MAX_PATH_LEN];



//static int show_vfsmnt(struct seq_file *m, struct vfsmount *mnt)
//static int show_mountinfo(struct seq_file *m, struct vfsmount *mnt)
//static int show_vfsstat(struct seq_file *m, struct vfsmount *mnt)
void before_show_vfsmnt(hook_fargs2_t *args, void *udata){
    if(likely(get_task_mm)){
        if(current&&!(get_task_mm(current))){
            return;
        }
    }

    struct seq_file* b_seq_file;
    b_seq_file = args->arg0;
    args->local.data0 = b_seq_file->count;
}

void after_show_vfsmnt(hook_fargs2_t *args, void *udata){
    if(args->local.data0){
        struct seq_file* o_seq_file;
        o_seq_file = args->arg0;
        size_t bcount = (size_t)args->local.data0;
        int llen = o_seq_file->count - bcount;
        if(unlikely(llen==0)){
            return;
        }
        char lstr[llen];
        memcpy(lstr,o_seq_file->buf+bcount,llen);
        for(int i=0;i<TARGET_PATH_NUM;i++){
            if(unlikely(strstr(lstr,target_path[i]))){
                logkd("test_log:%d %d %s\n",bcount,o_seq_file->count,lstr);
                o_seq_file->count = bcount;
                break;
            }
        }
    }

}

static long hide_mount_init(const char *args, const char *event, void *__user reserved)
{
    strcpy(target_path[0],"/data/adb/modules");
    strcpy(target_path[1],"/debug_ramdisk");
    get_task_mm = (typeof(get_task_mm))kallsyms_lookup_name("get_task_mm");
    show_vfsmnt_addr = kallsyms_lookup_name("show_vfsmnt");
    show_mountinfo_addr = kallsyms_lookup_name("show_mountinfo");
    show_vfsstat_addr = kallsyms_lookup_name("show_vfsstat");
    logkd("show_vfsmnt_addr:%llx,show_mountinfo_addr:llx,show_vfsstat_addr:llx\n",show_vfsmnt_addr,show_mountinfo_addr,show_vfsstat_addr);
    if(show_vfsmnt_addr){
        hook_wrap2((void *)show_vfsmnt_addr,before_show_vfsmnt,after_show_vfsmnt,0);
    }
    if(show_mountinfo_addr){
        hook_wrap2((void *)show_mountinfo_addr,before_show_vfsmnt,after_show_vfsmnt,0);
    }
    if(show_vfsstat_addr){
        hook_wrap2((void *)show_vfsstat_addr,before_show_vfsmnt,after_show_vfsmnt,0);
    }
    logkd("kpm hide_mount init\n");
    return 0;
}

static long hide_mount_control0(const char *args, char *__user out_msg, int outlen)
{
    return 0;
}

static long hide_mount_exit(void *__user reserved)
{
    if(show_vfsmnt_addr){
        unhook((void *)show_vfsmnt_addr);
    }
    if(show_mountinfo_addr){
        unhook((void *)show_mountinfo_addr);
    }
    if(show_vfsstat_addr){
        unhook((void *)show_vfsstat_addr);
    }
    logkd("kpm hide_mount  exit\n");
}

KPM_INIT(hide_mount_init);
KPM_CTL0(hide_mount_control0);
KPM_EXIT(hide_mount_exit);