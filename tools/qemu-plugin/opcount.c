/* A QEMU TCG plugin that counts guest instructions executed.
 *
 * This is the repository's deterministic cross-target performance metric. It
 * answers "how many riscv32 instructions does a decode actually execute",
 * which is the number that matters and the one nothing else here provides:
 * static .text bounds a win but cannot rank changes, the host's Callgrind
 * counts x86-64 instructions for a decoder that ships to 32-bit targets, and
 * wall clock under qemu-user measures the JIT rather than the guest.
 *
 * It is exact and reproducible: the same binary on the same input returns the
 * same count every time, on any host, with no timing noise at all. It is not
 * a cycle count -- it does not know about the target's pipeline, its cache, or
 * its multiplier latency -- so it ranks candidates rather than predicting
 * hardware time. Real silicon stays the timing authority.
 *
 * Build with tools/opcount.py, which supplies the include path and the
 * QEMU-version-matched header.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <qemu-plugin.h>

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static struct qemu_plugin_scoreboard *counts;
static qemu_plugin_u64 insn_count;

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);

    /* One accumulate per translation block rather than per instruction: the
       block's instruction count is known at translation time, so the executed
       total is exact while costing a single add per block at run time. */
    qemu_plugin_register_vcpu_tb_exec_inline_per_vcpu(
        tb, QEMU_PLUGIN_INLINE_ADD_U64, insn_count, n);
}

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    uint64_t total = qemu_plugin_u64_sum(insn_count);
    /* stderr so it cannot be confused with the decoder's own stdout, which
       carries the checksum and PSNR the harness parses. */
    fprintf(stderr, "OPCOUNT %" PRIu64 "\n", total);
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info,
                                           int argc, char **argv)
{
    counts = qemu_plugin_scoreboard_new(sizeof(uint64_t));
    insn_count = qemu_plugin_scoreboard_u64(counts);
    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    return 0;
}
