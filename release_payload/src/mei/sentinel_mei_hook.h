/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * sentinel_mei_hook.h — Sentinel MEI Kprobes Telemetry & Interdiction
 *
 * Minimal struct overlays and policy definitions for out-of-tree hooking
 * of the Linux mei_cl_bus subsystem. These definitions mirror the upstream
 * kernel structures at known offsets to allow safe GUID extraction from
 * kprobe pt_regs context without depending on unexported headers.
 *
 * WARNING: These offsets are validated against Linux 6.19 (Fedora 43).
 * Kernel ABI changes may require re-validation via `pahole` or BTF inspection.
 *
 * Copyright (C) 2026 Nevin Shine <nevinshine05@outlook.com>
 */

#ifndef _SENTINEL_MEI_HOOK_H
#define _SENTINEL_MEI_HOOK_H

#include <linux/types.h>
#include <linux/uuid.h>

/* -------------------------------------------------------------------------
 * Policy Configuration
 * -------------------------------------------------------------------------
 * Defines the enforcement posture for intercepted HECI messages.
 */
enum sentinel_mei_policy {
	SMEI_ALLOW,     /* Permit the HECI transaction silently                */
	SMEI_LOG_ONLY,  /* Permit but emit a dmesg telemetry event             */
	SMEI_DENY,      /* Block by overwriting return register with -EPERM    */
};

/* -------------------------------------------------------------------------
 * GUID Extraction Strategy
 * -------------------------------------------------------------------------
 * The upstream <linux/mei_cl_bus.h> exposes struct mei_cl_device but
 * forward-declares struct mei_me_client as OPAQUE. We cannot dereference
 * cldev->me_cl->props.protocol_name via the public header.
 *
 * Instead, we use a two-step raw pointer chase:
 *
 *   1. Read the `me_cl` pointer from mei_cl_device at a known offset.
 *      From the upstream struct layout (kernel 6.19):
 *
 *        struct mei_cl_device {
 *            struct list_head bus_list;       // +0   (16 bytes)
 *            struct mei_device *bus;          // +16  (8 bytes)
 *            struct device dev;              // +24  (varies, ~1024 bytes)
 *            struct mei_me_client *me_cl;    // +24 + sizeof(struct device)
 *            ...
 *        };
 *
 *   2. Read the GUID (uuid_le / guid_t) from mei_me_client at a known
 *      offset. The mei_me_client layout:
 *
 *        struct mei_me_client {
 *            struct list_head list;               // +0   (16 bytes)
 *            struct kref refcnt;                   // +16  (4 bytes + 4 pad)
 *            struct mei_client_properties props;   // +24
 *              -> uuid_le protocol_name;           // +24 (first field)
 *            ...
 *        };
 *
 * IMPORTANT: sizeof(struct device) varies by kernel config (LOCKDEP,
 * KASAN, etc). We compute the me_cl offset dynamically at module init
 * using offsetof() on the struct exposed by <linux/mei_cl_bus.h>.
 */

#include <linux/mei_cl_bus.h>

/*
 * Offset of the GUID within struct mei_me_client.
 * The uuid_le protocol_name is the first field of mei_client_properties,
 * which follows list_head (16) + kref (4 + 4 padding) = 24.
 */
#define MEI_ME_CLIENT_GUID_OFFSET  24

/* -------------------------------------------------------------------------
 * Restricted ME Client GUIDs (Default Deny List)
 * -------------------------------------------------------------------------
 * These GUIDs identify ME firmware services that pose an unacceptable
 * security risk if accessible from Ring 0. Any HECI message targeting
 * these clients will be interdicted by the kretprobe handler.
 *
 * Reference: Intel MEI Client Bus Documentation & ME firmware analysis.
 */
struct sentinel_mei_guid_entry {
	guid_t guid;
	const char *name;
	enum sentinel_mei_policy policy;
};

/*
 * Well-known Intel ME Client GUIDs.
 *
 * AMT Remote Control:
 *   Allows remote power-on, power-off, and reset of the platform.
 *   An attacker with ME access can remotely reboot into a compromised
 *   boot chain.
 *
 * AMT Serial-Over-LAN (SOL):
 *   Provides a remote serial console that bypasses the OS entirely.
 *   An attacker can interact with BIOS/UEFI setup, GRUB, or single-user
 *   mode without any OS-level authentication.
 *
 * ICC (Integrated Clock Controller):
 *   Controls CPU/PCH clock frequencies. Malicious clock manipulation
 *   can destabilize the platform or enable side-channel attacks.
 *
 * MKHI (ME Kernel Host Interface):
 *   The master management interface. While we need it for the soft-disable
 *   in Phase 1, by default we LOG all traffic to it. It is only set to
 *   DENY if the operator explicitly enables strict mode.
 */
#define SENTINEL_MEI_RESTRICTED_GUIDS_COUNT 4

static const struct sentinel_mei_guid_entry
sentinel_mei_restricted_guids[SENTINEL_MEI_RESTRICTED_GUIDS_COUNT] = {
	{
		/* AMT Remote Control */
		.guid   = GUID_INIT(0x12F80028, 0xB4B7, 0x4B2D,
				    0xAC, 0xA8, 0x46, 0xE0, 0xFF, 0x65, 0x81, 0x4C),
		.name   = "AMT Remote Control",
		.policy = SMEI_DENY,
	},
	{
		/* AMT Serial-Over-LAN (SOL) */
		.guid   = GUID_INIT(0xFB3B192E, 0xE714, 0x44A4,
				    0x8B, 0x22, 0x55, 0x82, 0x23, 0xDE, 0x6F, 0xF4),
		.name   = "AMT SOL",
		.policy = SMEI_DENY,
	},
	{
		/* ICC (Integrated Clock Controller) */
		.guid   = GUID_INIT(0xF934D0F2, 0x3E42, 0x402E,
				    0x8C, 0xDF, 0x73, 0x01, 0x02, 0xAA, 0x13, 0xD1),
		.name   = "ICC Clock Control",
		.policy = SMEI_DENY,
	},
	{
		/* MKHI (ME Kernel Host Interface) — LOG by default */
		.guid   = GUID_INIT(0x8E6A6715, 0x9ABC, 0x4043,
				    0x88, 0xEF, 0x9E, 0x39, 0xC6, 0xF6, 0x3E, 0x0F),
		.name   = "MKHI",
		.policy = SMEI_LOG_ONLY,
	},
};

/* -------------------------------------------------------------------------
 * Telemetry Event Types (for structured dmesg logging)
 * ------------------------------------------------------------------------- */
enum sentinel_mei_event {
	SMEI_EVT_SEND_ALLOWED,
	SMEI_EVT_SEND_BLOCKED,
	SMEI_EVT_SEND_LOGGED,
	SMEI_EVT_RECV_OBSERVED,
	SMEI_EVT_PROBE_ATTACHED,
	SMEI_EVT_PROBE_DETACHED,
};

/* -------------------------------------------------------------------------
 * Generic Netlink Telemetry Bridge
 * ------------------------------------------------------------------------- */
#define SENTINEL_GENL_NAME "SENTINEL_MEI"
#define SENTINEL_GENL_MCGRP "heci_events"

enum sentinel_genl_attrs {
	SENTINEL_ATTR_UNSPEC,
	SENTINEL_ATTR_EVENT,
	__SENTINEL_ATTR_MAX,
};
#define SENTINEL_ATTR_MAX (__SENTINEL_ATTR_MAX - 1)

enum sentinel_genl_cmds {
	SENTINEL_CMD_UNSPEC,
	SENTINEL_CMD_EVENT,
	__SENTINEL_CMD_MAX,
};
#define SENTINEL_CMD_MAX (__SENTINEL_CMD_MAX - 1)

/* Packed binary struct for zero-allocation userspace unpacking */
struct sentinel_heci_event {
	u32 pid;
	u32 uid;
	u32 action; /* 1 = DENY, 0 = LOG_ONLY */
	u8  guid[16];
} __attribute__((packed));

#endif /* _SENTINEL_MEI_HOOK_H */
