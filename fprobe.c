#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/gro.h>

#include <linux/fprobe.h>
#include <asm/ptrace.h>

#define UDP_GRO_CNT_MAX 64

static struct fprobe fp = {
};

static void entry_handler(struct fprobe *fp, unsigned long entry_ip, struct pt_regs *regs) {
    struct list_head *head = (struct list_head *)regs_get_kernel_argument(regs, 0);
    struct sk_buff *skb = (struct sk_buff *)regs_get_kernel_argument(regs, 1);

    struct udphdr *uh = udp_gro_udphdr(skb);
    struct udphdr *uh2;
    struct sk_buff *p;
    unsigned int ulen;
    int ret = 0;

    /* requires non zero csum, for symmetry with GSO */
    if (!uh->check) {
        return;
    }

    /* Do not deal with padded or malicious packets, sorry ! */
    ulen = ntohs(uh->len);
    if (ulen <= sizeof(*uh) || ulen != skb_gro_len(skb)) {
        return;
    }

    list_for_each_entry(p, head, list) {
        pr_info("0x%p: 0x%p\n", skb, p);

        if (!NAPI_GRO_CB(p)->same_flow) {
            pr_info("0x%p: 0x%p !same_flow\n", skb, p);
            continue;
        }

        uh2 = udp_hdr(p);

        /* Match ports only, as csum is always non zero */
        if ((*(u32 *)&uh->source != *(u32 *)&uh2->source)) {
            pr_info("0x%p: 0x%p !source\n", skb, p);
            continue;
        }

        if (NAPI_GRO_CB(skb)->is_flist != NAPI_GRO_CB(p)->is_flist) {
            pr_info("0x%p: 0x%p !is_flist\n", skb, p);
            return;
        }

        /* Terminate the flow on len mismatch or if it grow "too much".
         * Under small packet flood GRO count could elsewhere grow a lot
         * leading to excessive truesize values.
         * On len mismatch merge the first packet shorter than gso_size,
         * otherwise complete the GRO packet.
         */
        if (ulen > ntohs(uh2->len)) {
            pr_info("0x%p: 0x%p ulen > uh2->len\n", skb, p);
        } else {
            if (NAPI_GRO_CB(skb)->is_flist) {
                if ((skb->ip_summed != p->ip_summed) ||
                    (skb->csum_level != p->csum_level)) {
                    pr_info("0x%p: 0x%p !csum\n", skb, p);
                    return;
                }
                pr_info("0x%p: 0x%p skb_gro_receive_list\n", skb, p);
            } else {
                pr_info("0x%p: 0x%p skb_gro_receive\n", skb, p);
            }
        }

        if (ret || ulen != ntohs(uh2->len) || NAPI_GRO_CB(p)->count >= UDP_GRO_CNT_MAX) {
            pr_info("0x%p: 0x%p ulen: %d, uh2->len: %d\n", skb, p, ulen, ntohs(uh2->len));
        }

        return;
    }
}

static void exit_handler(struct fprobe *fp, unsigned long entry_ip, struct pt_regs *regs) {

}

static int __init fprobe_init(void) {
    int ret;

    fp.entry_handler = entry_handler;
    fp.exit_handler = exit_handler;

    ret = register_fprobe(&fp, "udp_gro_receive_segment", NULL);
    if (ret < 0) {
        pr_err("register_fprobe failed, returned %d\n", ret);
        return ret;
    }

    pr_info("Planted fprobe\n");
    return 0;
}

static void __exit fprobe_exit(void) {
    unregister_fprobe(&fp);
    pr_info("fprobe unregistered\n");
}

module_init(fprobe_init)
module_exit(fprobe_exit)

MODULE_LICENSE("GPL");