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

// TODO: Change exec ls(1) to exec any shell command or binary
int exec(void)
{
    struct file *file_out = NULL;
    struct file *file_err = NULL;
    char *command = "ls";
    char *target = "/tmp/";
    char *output_file = "/tmp/remotek_stdout";
    char *errput_file = "/tmp/remotek_stderr";
    char *bufout = NULL;
    char *buferr = NULL;

    struct subprocess_info *sub_info = NULL;
    char *cmd = NULL;
    char *envp[] = { "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    int status = 0;

    loff_t pos = 0;
    loff_t filesize_out = 0;
    loff_t filesize_err = 0;
    int len = 0;

    pr_info("remotek: running %s on: %s\n", command, target);
    cmd = kmalloc(4096, GFP_KERNEL);
    if (!cmd)
    {
        pr_err("remotek: failed to allocate memory for cmd\n");
        return 1;
    }
    sprintf(cmd, "%s %s >%s 2>%s", command, target, output_file, errput_file);
    char *argv[] = { "/bin/sh", "-c", cmd, NULL };


    sub_info = call_usermodehelper_setup(argv[0], argv, envp, GFP_KERNEL, NULL, NULL, NULL);
    if (sub_info == NULL)
    {
        pr_err("remotek: failed to setup usermodehelper\n");
        kfree(cmd);
        return 1;
    }

    status = call_usermodehelper_exec(sub_info, UMH_WAIT_PROC);
    status = status >> 8;
    pr_info("remotek: finished with exit status: %d\n", status);

    filesize_out = file_size(output_file);
    filesize_err = file_size(errput_file);
    if (filesize_out == -1 || filesize_err == -1)
    {
        pr_err("remotek: cannot stat '%s' or '%s'\n", output_file, errput_file);
        kfree(cmd);
        return 1;
    }

    bufout = kmalloc(filesize_out + 1, GFP_KERNEL);
    buferr = kmalloc(filesize_err + 1, GFP_KERNEL);
    if (bufout == NULL || buferr == NULL)
    {
        pr_err("remotek: failed to allocate memory for bufout or buferr\n");
        kfree(cmd);
        return 1;
    }
    bufout[filesize_out] = '\0';
    buferr[filesize_err] = '\0';

    file_out = filp_open(output_file, O_RDONLY, 0);
    file_err = filp_open(errput_file, O_RDONLY, 0);
    if (IS_ERR(file_out) || IS_ERR(file_err))
    {
        pr_err("remotek: failed to open %s(%ld) or %s(%ld)\n",
               output_file,
               PTR_ERR(file_out),
               errput_file,
               PTR_ERR(file_err));
        kfree(bufout);
        kfree(buferr);
        kfree(cmd);
        return 1;
    }

    len = kernel_read(file_out, bufout, filesize_out, &pos);
    if (len < 0 || len < filesize_out)
    {
        pr_err("remotek: failed to read %s\n", output_file);
        filp_close(file_out, NULL);
        filp_close(file_err, NULL);
        kfree(bufout);
        kfree(cmd);
        return 1;
    }
    len = kernel_read(file_err, buferr, filesize_err, &pos);
    if (len < 0 || len < filesize_err)
    {
        pr_err("remotek: failed to read %s\n", errput_file);
        filp_close(file_out, NULL);
        filp_close(file_err, NULL);
        kfree(bufout);
        kfree(cmd);
        return 1;
    }


    pr_info("remotek: output:\n%s\n\n", bufout);
    pr_info("remotek: stderr:\n%s\n", buferr);
    pr_info("\n");
    filp_close(file_out, NULL);
    filp_close(file_err, NULL);
    kfree(bufout);
    kfree(buferr);
    kfree(cmd);
    return 0;
}
