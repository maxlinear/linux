/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NET_EVENT_H
#define _NET_EVENT_H

/*
 *	Generic netevent notifiers
 *
 *	Authors:
 *      Tom Tucker              <tom@opengridcomputing.com>
 *      Steve Wise              <swise@opengridcomputing.com>
 *
 * 	Changes:
 */

struct dst_entry;
struct neighbour;

struct netevent_redirect {
	struct dst_entry *old;
	struct dst_entry *new;
	struct neighbour *neigh;
	const void *daddr;
};

enum netevent_notif_type {
	NETEVENT_NEIGH_UPDATE = 1, /* arg is struct neighbour ptr */
	NETEVENT_REDIRECT,	   /* arg is struct netevent_redirect ptr */
	NETEVENT_DELAY_PROBE_TIME_UPDATE, /* arg is struct neigh_parms ptr */
	NETEVENT_IPV4_MPATH_HASH_UPDATE, /* arg is struct net ptr */
	NETEVENT_IPV6_MPATH_HASH_UPDATE, /* arg is struct net ptr */
	NETEVENT_IPV4_FWD_UPDATE_PRIORITY_UPDATE, /* arg is struct net ptr */
	NETEVENT_NF_HOOK_SLOW_DROP, /* arg is struct sk_buff */
	NETEVENT_EBT_DO_TABLE_DROP, /* arg is struct sk_buff */
	NETEVENT_IPT_DO_TABLE_DROP, /* arg is struct sk_buff */
	NETEVENT_IP6T_DO_TABLE_DROP, /* arg is struct sk_buff */
	NETEVENT_NF_REINJECT_DROP, /* arg is struct sk_buff */
	NETEVENT_IPV4_MARTIAN_SRC, /* arg is struct sk_buff */
	NETEVENT_IPV4_MARTIAN_DST, /* arg is struct sk_buff */
	NETEVENT_BR_FLOOD, /* arg is struct sk_buff */
};

int register_netevent_notifier(struct notifier_block *nb);
int unregister_netevent_notifier(struct notifier_block *nb);
int call_netevent_notifiers(unsigned long val, void *v);

#endif
