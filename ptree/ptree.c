#include <linux/kernel.h>
#include <linux/prinfo.h>

int ptree(struct prinfo* buf, int* nr) {
    printk(KERN_INFO "RPI TEST MSG\n");
    return 0;
}