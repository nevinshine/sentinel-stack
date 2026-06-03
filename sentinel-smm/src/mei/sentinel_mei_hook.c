// SPDX-License-Identifier: GPL-2.0-only
/*
 * sentinel_mei_hook.c — Sentinel Ring -3 MEI Telemetry & Interdiction Module
 *
 * A Loadable Kernel Module that dynamically hooks the Linux mei_cl_bus
 * subsystem using Kprobes and Kretprobes to:
 *
 *   1. Intercept all outbound HECI messages (host -> ME) via mei_cldev_send()
 *   2. Extract the target ME Client GUID via raw pointer chase
 *   3. Evaluate the GUID against a restricted deny-list
 *   4. Block unauthorized commands by overwriting the return register
 *      with -EPERM via kretprobe
 *   5. Log all inbound ME responses via mei_cldev_recv() for forensic
 *      correlation
 *
 * This module is the telemetric foundation for the Sentinel Stack's
 * Ring -3 containment architecture, providing deterministic visibility
 * into the Host Embedded Controller Interface (HECI) before hardware
 * lockdowns are applied in Phase 3.
 *
 * Build: make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
 * Load:  insmod sentinel_mei_hook.ko [strict_mkhi=1]
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#define pr_fmt(fmt) "SENTINEL_MEI: " fmt

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/mei_cl_bus.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/uuid.h>
#include <linux/version.h>
#include <net/genetlink.h>

#include "sentinel_mei_hook.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nevin Shine <nevinshine05@outlook.com>");
MODULE_DESCRIPTION("Sentinel Ring -3 MEI Telemetry & HECI Interdiction");
MODULE_VERSION("1.0");

/* -------------------------------------------------------------------------
 * Module Parameters
 * -------------------------------------------------------------------------
 * strict_mkhi: When set to 1, escalates MKHI policy from LOG_ONLY to DENY.
 *              This blocks ALL management commands to the ME, including
 *              those needed for the Phase 1 soft-disable. Only enable this
 *              after the soft-disable has been successfully executed.
 *
 * log_allowed: When set to 1, logs all ALLOWED transactions (verbose).
 *              Default is 0 (only log DENY and LOG_ONLY events).
 */
static int strict_mkhi;
module_param(strict_mkhi, int, 0644);
MODULE_PARM_DESC(strict_mkhi, "Escalate MKHI policy to DENY (default: 0)");

static int log_allowed;
module_param(log_allowed, int, 0644);
MODULE_PARM_DESC(log_allowed, "Log ALLOWED HECI transactions (default: 0)");

/* -------------------------------------------------------------------------
 * Internal State
 * -------------------------------------------------------------------------
 * The kretprobe entry-handler stores the policy decision in per-instance
 * data so the kretprobe return-handler can act on it without races.
 */
struct sentinel_mei_kretprobe_data {
  enum sentinel_mei_policy policy;
  guid_t guid;
  const char *client_name;
};

/* Counters for telemetry */
static atomic64_t mei_send_total = ATOMIC64_INIT(0);
static atomic64_t mei_send_blocked = ATOMIC64_INIT(0);
static atomic64_t mei_send_logged = ATOMIC64_INIT(0);
static atomic64_t mei_recv_total = ATOMIC64_INIT(0);

/* -------------------------------------------------------------------------
 * Generic Netlink Boilerplate
 * ------------------------------------------------------------------------- */
static const struct genl_multicast_group sentinel_genl_mcgrps[] = {
    {.name = SENTINEL_GENL_MCGRP},
};

static struct genl_family sentinel_genl_family = {
    .name = SENTINEL_GENL_NAME,
    .version = 1,
    .maxattr = SENTINEL_ATTR_MAX,
    .mcgrps = sentinel_genl_mcgrps,
    .n_mcgrps = ARRAY_SIZE(sentinel_genl_mcgrps),
};

static void sentinel_mei_netlink_broadcast(struct pt_regs *regs,
                                           const guid_t *guid, u32 action) {
  struct sk_buff *skb;
  void *msg_head;
  struct sentinel_heci_event evt;

  /* Must use GFP_ATOMIC because this is called from kprobe context */
  skb = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
  if (!skb) {
    pr_err("Failed to allocate netlink skb\n");
    return;
  }

  msg_head =
      genlmsg_put(skb, 0, 0, &sentinel_genl_family, 0, SENTINEL_CMD_EVENT);
  if (!msg_head) {
    pr_err("genlmsg_put failed\n");
    nlmsg_free(skb);
    return;
  }

  memset(&evt, 0, sizeof(evt));
  evt.pid = task_tgid_vnr(current);
  evt.uid = from_kuid(&init_user_ns, current_uid());
  evt.action = action;
  guid_copy((guid_t *)&evt.guid, guid);

  if (nla_put(skb, SENTINEL_ATTR_EVENT, sizeof(evt), &evt)) {
    pr_err("nla_put failed\n");
    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    return;
  }

  genlmsg_end(skb, msg_head);
  genlmsg_multicast(&sentinel_genl_family, skb, 0, 0, GFP_ATOMIC);
}

/* -------------------------------------------------------------------------
 * GUID Policy Lookup
 * -------------------------------------------------------------------------
 * Evaluates a ME Client GUID against the restricted deny-list.
 * Returns the policy action and populates the client name for logging.
 */
static enum sentinel_mei_policy
sentinel_mei_lookup_guid(const guid_t *guid, const char **out_name) {
  int i;

  for (i = 0; i < SENTINEL_MEI_RESTRICTED_GUIDS_COUNT; i++) {
    if (guid_equal(guid, &sentinel_mei_restricted_guids[i].guid)) {
      *out_name = sentinel_mei_restricted_guids[i].name;

      /* Honor the strict_mkhi override */
      if (sentinel_mei_restricted_guids[i].policy == SMEI_LOG_ONLY &&
          strict_mkhi)
        return SMEI_DENY;

      return sentinel_mei_restricted_guids[i].policy;
    }
  }

  *out_name = "Unknown";
  return SMEI_ALLOW;
}

/* -------------------------------------------------------------------------
 * GUID Extraction from mei_cl_device Pointer
 * -------------------------------------------------------------------------
 * Safely extracts the ME Client GUID from the first argument (RDI on
 * x86_64 SysV ABI) of mei_cldev_send().
 *
 * The pointer chase is:
 *   mei_cl_device *cldev  (arg0, from pt_regs->di)
 *     -> struct mei_me_client *me_cl  (opaque, via offsetof)
 *       -> guid_t at MEI_ME_CLIENT_GUID_OFFSET (24 bytes into me_cl)
 *
 * Since mei_me_client is opaque in the public headers, we read the
 * me_cl pointer using offsetof(struct mei_cl_device, me_cl) on the
 * upstream-exposed struct, then raw-chase into the opaque struct
 * using copy_from_kernel_nofault() to avoid faults on invalid pointers.
 */
static bool sentinel_mei_extract_guid(struct pt_regs *regs, guid_t *out_guid) {
  struct mei_cl_device *cldev;
  void *me_cl_ptr;
  unsigned long me_cl_addr;

  /* arg0: struct mei_cl_device *cldev */
  cldev = (struct mei_cl_device *)regs_get_kernel_argument(regs, 0);
  if (!cldev)
    return false;

  /*
   * Read the opaque me_cl pointer from the upstream-exposed struct.
   * offsetof(struct mei_cl_device, me_cl) is safe because the field
   * is declared (as a forward-declared pointer) in <linux/mei_cl_bus.h>.
   */
  if (copy_from_kernel_nofault(
          &me_cl_addr, (void *)cldev + offsetof(struct mei_cl_device, me_cl),
          sizeof(unsigned long)))
    return false;

  me_cl_ptr = (void *)me_cl_addr;
  if (!me_cl_ptr || me_cl_addr < PAGE_OFFSET)
    return false;

  /*
   * Read the GUID from the opaque mei_me_client struct.
   * The guid_t (protocol_name) is the first field of
   * mei_client_properties, which sits at offset 24 within
   * mei_me_client (after list_head[16] + kref[4] + padding[4]).
   */
  if (copy_from_kernel_nofault(out_guid, me_cl_ptr + MEI_ME_CLIENT_GUID_OFFSET,
                               sizeof(guid_t)))
    return false;

  return true;
}

/* -------------------------------------------------------------------------
 * Kprobe: mei_cldev_send() — Outbound HECI Telemetry
 * -------------------------------------------------------------------------
 * This is the observability probe. It fires on every outbound HECI message
 * and logs the transaction metadata. The actual blocking is performed by
 * the kretprobe below.
 *
 * Function signature (upstream Linux 6.x):
 *   ssize_t mei_cldev_send(struct mei_cl_device *cldev,
 *                          const u8 *buf, size_t length)
 */
static int sentinel_mei_send_pre(struct kprobe *p, struct pt_regs *regs) {
  guid_t guid;
  const char *client_name = "Unknown";
  enum sentinel_mei_policy policy;
  size_t length;

  atomic64_inc(&mei_send_total);

  if (!sentinel_mei_extract_guid(regs, &guid))
    return 0; /* Cannot extract GUID; allow and move on */

  /* arg2: size_t length */
  length = (size_t)regs_get_kernel_argument(regs, 2);

  policy = sentinel_mei_lookup_guid(&guid, &client_name);

  switch (policy) {
  case SMEI_DENY:
    pr_warn("[DENY] -> ME Client: %-24s GUID: %pUl  len=%zu  pid=%d  comm=%s\n",
            client_name, &guid, length, current->pid, current->comm);
    atomic64_inc(&mei_send_blocked);
    break;

  case SMEI_LOG_ONLY:
    pr_info("[LOG]  -> ME Client: %-24s GUID: %pUl  len=%zu  pid=%d  comm=%s\n",
            client_name, &guid, length, current->pid, current->comm);
    atomic64_inc(&mei_send_logged);
    break;

  case SMEI_ALLOW:
    if (log_allowed)
      pr_debug("[PASS] -> ME Client: %-24s GUID: %pUl  len=%zu\n", client_name,
               &guid, length);
    break;
  }

  return 0;
}

static struct kprobe kp_mei_send = {
    .symbol_name = "mei_cldev_send",
    .pre_handler = sentinel_mei_send_pre,
};

/* -------------------------------------------------------------------------
 * Kretprobe: mei_cldev_send() — HECI Interdiction
 * -------------------------------------------------------------------------
 * The entry handler captures the GUID and policy decision into per-instance
 * data. The return handler checks if the policy is DENY and, if so,
 * overwrites the return value with -EPERM.
 *
 * This two-stage approach is necessary because kretprobes fire on function
 * RETURN, at which point the original pt_regs from the entry are no longer
 * accessible. The kretprobe_instance->data bridge carries state across.
 */
static int sentinel_mei_send_ret_entry(struct kretprobe_instance *ri,
                                       struct pt_regs *regs) {
  struct sentinel_mei_kretprobe_data *data;
  guid_t guid;
  const char *client_name;

  data = (struct sentinel_mei_kretprobe_data *)ri->data;
  data->policy = SMEI_ALLOW;
  data->client_name = "Unknown";

  if (!sentinel_mei_extract_guid(regs, &guid)) {
    memset(&data->guid, 0, sizeof(guid_t));
    return 0;
  }

  guid_copy(&data->guid, &guid);
  data->policy = sentinel_mei_lookup_guid(&guid, &client_name);
  data->client_name = client_name;

  return 0;
}

static int sentinel_mei_send_ret_handler(struct kretprobe_instance *ri,
                                         struct pt_regs *regs) {
  struct sentinel_mei_kretprobe_data *data;

  data = (struct sentinel_mei_kretprobe_data *)ri->data;

  if (data->policy == SMEI_DENY) {
    /*
     * Overwrite the return register (RAX on x86_64) with -EPERM.
     * This causes the calling process's write() / ioctl() to
     * receive an EPERM error, effectively blocking the HECI
     * transaction before the payload reaches the ME circular queue.
     */
    regs_set_return_value(regs, (unsigned long)(-EPERM));

    pr_warn("[BLOCK] Interdicted HECI send to ME Client: %s  "
            "GUID: %pUl  pid=%d  comm=%s  (return -> -EPERM)\n",
            data->client_name, &data->guid, current->pid, current->comm);

    /* Broadcast telemetry over Netlink */
    sentinel_mei_netlink_broadcast(regs, &data->guid, 1);
  }

  return 0;
}

static struct kretprobe krp_mei_send = {
    .handler = sentinel_mei_send_ret_handler,
    .entry_handler = sentinel_mei_send_ret_entry,
    .data_size = sizeof(struct sentinel_mei_kretprobe_data),
    .maxactive = 16, /* Max concurrent probed instances */
    .kp =
        {
            .symbol_name = "mei_cldev_send",
        },
};

/* -------------------------------------------------------------------------
 * Kprobe: mei_cldev_recv() — Inbound ME Response Telemetry
 * -------------------------------------------------------------------------
 * Observes all inbound responses from the ME to the host. This is purely
 * telemetric — we do not interdict inbound traffic, as blocking ME
 * responses could destabilize the HECI state machine.
 *
 * Function signature:
 *   ssize_t mei_cldev_recv(struct mei_cl_device *cldev,
 *                          u8 *buf, size_t length)
 */
static int sentinel_mei_recv_pre(struct kprobe *p, struct pt_regs *regs) {
  guid_t guid;
  const char *client_name = "Unknown";
  size_t length;

  atomic64_inc(&mei_recv_total);

  if (!sentinel_mei_extract_guid(regs, &guid))
    return 0;

  length = (size_t)regs_get_kernel_argument(regs, 2);

  (void)sentinel_mei_lookup_guid(&guid, &client_name);

  pr_info("[RECV] <- ME Client: %-24s GUID: %pUl  len=%zu  pid=%d  comm=%s\n",
          client_name, &guid, length, current->pid, current->comm);

  return 0;
}

static struct kprobe kp_mei_recv = {
    .symbol_name = "mei_cldev_recv",
    .pre_handler = sentinel_mei_recv_pre,
};

/* -------------------------------------------------------------------------
 * Module Initialization
 * -------------------------------------------------------------------------
 * Registers all kprobes and kretprobes. Gracefully handles the case where
 * the mei_me driver is not loaded (symbol not found) by logging a warning
 * and returning -ENODEV instead of panicking.
 */
static int __init sentinel_mei_init(void) {
  int ret;

  pr_info("=== Sentinel Ring -3 MEI Telemetry Module ===\n");
  pr_info("Loading... (strict_mkhi=%d, log_allowed=%d)\n", strict_mkhi,
          log_allowed);

  /* Register Generic Netlink Family */
  ret = genl_register_family(&sentinel_genl_family);
  if (ret) {
    pr_err("Failed to register Generic Netlink family: %d\n", ret);
    return ret;
  }
  pr_info("[+] Generic Netlink family '%s' registered\n", SENTINEL_GENL_NAME);

  /* Register the kretprobe first (it includes its own kprobe) */
  ret = register_kretprobe(&krp_mei_send);
  if (ret < 0) {
    if (ret == -ENOENT) {
      pr_warn("mei_cldev_send symbol not found. "
              "Is the mei_me driver loaded? "
              "(modprobe mei_me)\n");
      pr_warn("Module will not load without an active "
              "Intel ME interface.\n");
      return -ENODEV;
    }
    pr_err("Failed to register kretprobe on mei_cldev_send: %d\n", ret);
    return ret;
  }
  pr_info("[+] Kretprobe attached: mei_cldev_send (interdiction active)\n");

  /*
   * Register the observability kprobe on mei_cldev_send.
   * Note: Both the kprobe and kretprobe can coexist on the same symbol.
   * The kprobe fires on ENTRY for telemetry logging, while the
   * kretprobe fires on RETURN for interdiction.
   */
  ret = register_kprobe(&kp_mei_send);
  if (ret < 0) {
    pr_err("Failed to register kprobe on mei_cldev_send: %d\n", ret);
    goto unregister_kretprobe;
  }
  pr_info("[+] Kprobe attached: mei_cldev_send (telemetry active)\n");

  /* Register the recv kprobe for inbound telemetry */
  ret = register_kprobe(&kp_mei_recv);
  if (ret < 0) {
    pr_warn("Failed to register kprobe on mei_cldev_recv: %d "
            "(non-fatal, inbound telemetry disabled)\n",
            ret);
    /* Non-fatal: recv telemetry is optional */
  } else {
    pr_info("[+] Kprobe attached: mei_cldev_recv (inbound telemetry active)\n");
  }

  pr_info("=== Sentinel MEI Telemetry: OPERATIONAL ===\n");
  pr_info("Monitoring %d restricted ME Client GUIDs\n",
          SENTINEL_MEI_RESTRICTED_GUIDS_COUNT);

  return 0;

unregister_kretprobe:
  unregister_kretprobe(&krp_mei_send);
  genl_unregister_family(&sentinel_genl_family);
  return ret;
}

/* -------------------------------------------------------------------------
 * Module Teardown
 * -------------------------------------------------------------------------
 * Cleanly unregisters all probes and reports final telemetry counters.
 */
static void __exit sentinel_mei_exit(void) {
  pr_info("=== Sentinel MEI Telemetry: SHUTTING DOWN ===\n");

  /* Unregister in reverse order of registration */
  if (kp_mei_recv.addr)
    unregister_kprobe(&kp_mei_recv);

  unregister_kprobe(&kp_mei_send);
  unregister_kretprobe(&krp_mei_send);
  genl_unregister_family(&sentinel_genl_family);

  pr_info("Final Counters:\n");
  pr_info("  HECI Send Total:   %lld\n", atomic64_read(&mei_send_total));
  pr_info("  HECI Send Blocked: %lld\n", atomic64_read(&mei_send_blocked));
  pr_info("  HECI Send Logged:  %lld\n", atomic64_read(&mei_send_logged));
  pr_info("  HECI Recv Total:   %lld\n", atomic64_read(&mei_recv_total));

  /* Report kretprobe missed count (indicates maxactive was too low) */
  if (krp_mei_send.nmissed)
    pr_warn("Kretprobe missed %d events (increase maxactive)\n",
            krp_mei_send.nmissed);

  pr_info("=== Sentinel MEI Telemetry: OFFLINE ===\n");
}

module_init(sentinel_mei_init);
module_exit(sentinel_mei_exit);
