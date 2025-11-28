/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *  Copyright (C) 2020-2025 MaxLinear, Inc.
 *  Copyright (C) 2017-2018 Intel Corporation.
 *****************************************************************************/

#ifndef _SECURE_SERVICES_H
#define _SECURE_SERVICES_H

#define MAX_USER_NAME 16
#define FILEPNAME_MAX 128

typedef unsigned long sshandle_t;
typedef uint8_t sst_flags_t;

/* SST ICC Reply Message Code */
enum sst_response_code {
	SST_SUCCESSFUL = 0,				/* Successful */
	SST_OBJ_NOT_FOUND_ERR,			/* Object not found */
	SST_OBJ_POLICY_NOT_FOUND_ERR,	/* Policy not found */
	SST_POLICY_NOT_MATCHED_ERR,		/* Policy not matched */
	SST_SSHANDLE_NOT_MATCHED_ERR,	/* SShandle not matched */
	SST_OBJ_ATR_NOT_MATCHED_ERR,	/* Object attr not matched */
	SST_OBJ_ID_GENERATION_ERR,		/* Object ID geneation error */
	SST_OBJ_NODE_MEM_ALLOC_ERR,		/* Node momory allocation error */
	SST_ICC_POOL_ALLOC_ERR,			/* ICC mempool alloc error */
	SST_OBJ_ALREADY_EXIST_ERR,		/* Object already exist error */
	SST_OBJ_CREATE_REQ_ERR,			/* Create request error */
	SST_OBJ_OPEN_REQ_ERR,			/* Open request error */
	SST_OBJ_SAVE_REQ_ERR,			/* Save request error */
	SST_OBJ_LOAD_REQ_ERR,			/* Load request error */
	SST_OBJ_DELETE_REQ_ERR,			/* Delete requesr error */
};

/*!
 *     \brief enum for the secure store crypto mode flags
 */
typedef enum {
	SS_CI = 0x01,               /* confidentiality and integrity */
	SS_I = 0x02,                /* Integrity protection only */
	SS_RP = 0x08,               /* Anti-replay protection */
	SS_OVWINV = 0x40,           /* overwrite is the object is invalid */
	SS_OVW = 0x80               /* Modify/Overwrite the object */
} sst_crypto_mode_flag_t;

enum key_location {
	NO_WRAP_KEY = 0b00,
	KEY_IN_OTP = 0b01,            /* OTP */
	KEY_IN_SST = 0b10             /* Secure Storage */
};

typedef enum {
	SS_CREATE = 0x10,           /* Create the object if does not exist */
	SS_DELETE = 0x20,           /* Delete the object if it exist */
} sst_obj_ops_flag_t;

typedef struct {
	union {
		struct {
			uint8_t user:2;		/* !< UID access permissions */
			uint8_t group:2;	/* !< GID access permissions */
			uint8_t others:2;	/* !< Others access permissions */
			uint8_t pname:2;
		} field;
		uint8_t perms;
	} u;
} sst_access_perm_t;

typedef struct {
	union {
		struct {
			uint16_t lock:1;			/* !< set lock bit secure store. secure store
								   will no longer modify this file. */
			uint16_t no_load_to_userspace:1;	/* !< SSC holds onto object and allows
								   application in userspace to use it
								   in signing/encryption operations by
								   handle */
			uint16_t read_once:1;		/* !< Read the object once per boot */
			uint16_t ignore_uid:1;		/* !< Ignore UID in Policy */
			uint16_t ignore_gid:1;		/* !< Ignore GID in Policy Enforcement */
			uint16_t ignore_pname:1;	/* !< Ignore pname in policy Enforcement */
			uint16_t wrap_flag:2;		/* Wrapped flag */
			uint16_t admin_store:1;		/* Admin/Normal store access */
			uint16_t tee_only:1;		/* TEE only access mode */
			uint16_t reserve:6;		/* Reserved for furture use */
		} field;
		uint16_t attr;
	} u;
} sst_policy_attr_t;

/**
 * struct sst_wrap_params - represents secure storage information of the wrap key
 * @handle  : 64-bit secure storage handle of opened sst wrap key object
 * @access_perm : access permissions associated with sst wrap key object
 * @policy_attr : policy attributes for enforcing policy check
 * @crypto_mode_flag  : crypto mode options like integrity, encryption etc...
 */
struct sst_wrap_params {
	uint64_t handle;
	uint8_t access_perm;
	uint16_t policy_attr;
	uint16_t crypto_mode_flag;
};

typedef struct {
	enum key_location key_location;
	union {/* secure storage object wrap params or OTP asset ID */
		struct sst_wrap_params sst_wrap;
		unsigned int asset_number;
	} u;
} secure_wrap_asset_t;

typedef struct {
	enum key_location key_location;
	union {/* secure storage object wrap param pointer or OTP asset ID */
		unsigned int wrap_handle;
		unsigned int asset_number;
	} u;
} secure_wrap_asset_tep_t;

/*!
 *     \brief Contains the secure storage access policy parameters
 */
typedef struct {
	sst_access_perm_t  access_perm;				/* Object access permission*/
	sst_policy_attr_t policy_attr;				/* Object access attributs */
	sst_crypto_mode_flag_t crypto_mode_flag;	/* Object crypto flag */
	secure_wrap_asset_t wrap_asset;				/* Wrap Object configuration */
} sst_obj_config_t;


typedef struct {
	sshandle_t ss_handle;       		/* 16 byte Handle to save/retrieve the object */
	int sobject_len;					/* length of the object */
	const char *objectname;				/* Object name to be opened/created */
	sst_obj_config_t sst_access_policy; /* Object policy configuration */
	sst_flags_t secure_store_flags;		/* Object crypto flag */
} sst_param_t;

typedef struct {
	sshandle_t ss_handle;
	secure_wrap_asset_t wrap_asset;	/* Wrap Object configuration */
	size_t payload_len;
	const unsigned char *payload;
} sst_data_param_t;

#endif /* _SECURE_SERVICES_H */
