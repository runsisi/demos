#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

#define MAX_SYMBOL_LEN    64
static char symbol[MAX_SYMBOL_LEN] = "udp_gro_receive";
module_param_string(symbol, symbol, sizeof(symbol), 0644);

static struct kprobe kp = {
    .symbol_name = symbol,
};

static int pre_handler(struct kprobe *p, struct pt_regs *regs) {

    return 0;
}

static void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {

}

static int __init kprobe_init(void) {
    int ret;

    kp.pre_handler = pre_handler;
    kp.post_handler = post_handler;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("register_kprobe failed, returned %d\n", ret);
        return ret;
    }

    pr_info("Planted kprobe at %pF\n", kp.addr);
    return 0;
}

static void __exit kprobe_exit(void) {
    unregister_kprobe(&kp);
    pr_info("kprobe at %pF unregistered\n", kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)

MODULE_LICENSE("GPL");