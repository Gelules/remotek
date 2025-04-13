#include "exec.h"
#include "globals.h"
#include "network.h"

#include <linux/kmod.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/stat.h>

static loff_t file_size(const char *file)
{
    struct path p;
    struct kstat stat = { 0 };
    int ret = 0;

    ret = kern_path(file, LOOKUP_FOLLOW, &p);
    if (ret != 0)
    {
        pr_err("execls: cannot find file: %s\n", file);
        return -1;
    }

    ret = vfs_getattr(&p, &stat, STATX_SIZE, AT_STATX_SYNC_AS_STAT);
    path_put(&p);

    if (ret != 0)
    {
        pr_err("execls: vfs_getattr failed on %s\n", file);
    }

    return stat.size;
}

int exec(const char *command)
{
    struct subprocess_info *sub_info = NULL;
    char *output_file = "/tmp/remotek_stdout";
    char *errput_file = "/tmp/remotek_stderr";
    char *cmd = NULL;
    char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    int status = 0;

    pr_info("remotek: executing %s\n", command);
    cmd = kmalloc(4096, GFP_KERNEL);
    if (!cmd)
    {
        pr_err("remotek: failed to allocate memory for cmd\n");
        return -1;
    }
    sprintf(cmd, "%s >%s 2>%s", command, output_file, errput_file);
    char *argv[] = { "/bin/sh", "-c", cmd, NULL };


    sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
    if (sub_info == NULL)
    {
        pr_err("remotek: failed to setup usermodehelper\n");
        kfree(cmd);
        return -2;
    }

    status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
    status = status >> 8;
    pr_info("remotek: exec finished with exit status: %d\n", status);

    kfree(cmd);
    return status;
}

char *read_file(const char *path)
{
    struct file *f = NULL;
    char *buf = NULL;
    loff_t pos = 0;
    loff_t filesize = 0;
    int len = 0;

    filesize = file_size(path);
    if (filesize == -1)
    {
        pr_err("remotek: cannot stat '%s'\n", path);
        return NULL;
    }

    buf = kmalloc(filesize + 1, GFP_KERNEL);
    if (buf == NULL)
    {
        pr_err("remotek: failed to allocate memory for buf\n");
        return NULL;
    }
    buf[filesize] = '\0';

    f = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(f))
    {
        pr_err("remotek: failed to open %s(%ld)\n", path, PTR_ERR(f));
        kfree(buf);
        return NULL;
    }

    len = kernel_read(f, buf, filesize, &pos);
    if (len < 0 || len < filesize)
    {
        pr_err("remotek: failed to read %s\n", path);
        filp_close(f, NULL);
        kfree(buf);
        return NULL;
    }

    pr_info("remotek: output:\n%s", buf);
    pr_info("\n");
    filp_close(f, NULL);
    return buf;
}
