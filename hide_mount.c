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
#include <linux/mutex.h>
#include <asm/current.h>
#include <linux/compiler.h>
#include "hide_mount.h"

KPM_NAME("hide_mount");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("test");
KPM_DESCRIPTION("内核隐藏挂载");
#define MAX_TARGET_PATHS 50

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
        struct seq_file* b_seq_file;
        b_seq_file = args->arg0;
        args->local.data0 = b_seq_file->count;
    }
}
static bool path_exists(const char *path)
{
    for (int i = 0; i < current_num; ++i) {
        if (strncmp(target_path[i], path, MAX_PATH_LEN) == 0) {
            return true;
        }
    }
    return false;
}

void after_show_vfsmnt(hook_fargs2_t *args, void *udata)
{
    if (args->local.data0) {
        struct seq_file* o_seq_file = args->arg0;
        size_t bcount = (size_t)args->local.data0;
        int llen = o_seq_file->count - bcount;

        if (unlikely(llen <= 0)) return;

        char *lstr = kmalloc(llen + 1, GFP_KERNEL);
        if (!lstr) return;

        memcpy(lstr, o_seq_file->buf + bcount, llen);
        lstr[llen] = '\0';

        mutex_lock(&path_mutex);
        for (int i = 0; i < current_num; i++) {
            if (strstr(lstr, target_path[i])) {
                logkd("Hiding mount: %s\n", lstr);
                o_seq_file->count = bcount;
                break;
            }
        }
        mutex_unlock(&path_mutex);
        
        kfree(lstr);
    }
}
static long hide_mount_init(const char *args, const char *event, void *__user reserved)
{
    mutex_init(&path_mutex);
    
    mutex_lock(&path_mutex);
    // 初始化默认路径
    strncpy(target_path[0], "/debug_ramdisk", MAX_PATH_LEN);
    strncpy(target_path[1], "/apex/com.android.art/bin/dex2oat64", MAX_PATH_LEN);
    strncpy(target_path[2], "/apex/com.android.art/bin/dex2oat32", MAX_PATH_LEN);
    current_num = 3;
    mutex_unlock(&path_mutex);

    get_task_mm = (typeof(get_task_mm))kallsyms_lookup_name("get_task_mm");
    show_vfsmnt_addr = kallsyms_lookup_name("show_vfsmnt");
    show_mountinfo_addr = kallsyms_lookup_name("show_mountinfo");
    show_vfsstat_addr = kallsyms_lookup_name("show_vfsstat");
    logkd("show_vfsmnt_addr:%llx,show_mountinfo_addr:%llx,show_vfsstat_addr:%llx\n",show_vfsmnt_addr,show_mountinfo_addr,show_vfsstat_addr);
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
    char cmd[MAX_PATH_LEN] = {0};
    strncpy(cmd, args, MAX_PATH_LEN-1);

    mutex_lock(&path_mutex);
    
    if (strcmp(cmd, "delete") == 0) {
        if (current_num > 0) {
            current_num--;
            logkd("Removed path: %s\n", target_path[current_num]);
            memset(target_path[current_num], 0, MAX_PATH_LEN);
        }
    } else {
        if (current_num >= MAX_TARGET_PATHS) {
            mutex_unlock(&path_mutex);
            return -ENOSPC;
        }
        
        if (strnlen(cmd, MAX_PATH_LEN) >= MAX_PATH_LEN) {
            mutex_unlock(&path_mutex);
            return -EINVAL;
        }
        
        if (path_exists(cmd)) {
            mutex_unlock(&path_mutex);
            return -EEXIST;
        }

        strncpy(target_path[current_num], cmd, MAX_PATH_LEN-1);
        target_path[current_num][MAX_PATH_LEN-1] = '\0';
        current_num++;
        logkd("Added new path: %s\n", cmd);
    }
    
    mutex_unlock(&path_mutex);
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
