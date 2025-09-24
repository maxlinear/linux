#include <linux/errno.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/tee_drv.h>
#include <linux/genalloc.h>
#include <linux/dma-direct.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/slab.h>

#include <linux/icc/drv_icc.h>
#include <tep_interface.h>

#include <linux/secure_services.h>
#include <linux/secure_storage_client.h>

#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/string.h>

#define MXL_TEE_PIN_SST_OBJNAME 		"token_persistent_info"
#define MXL_TEE_PIN_SST_OBJNAME_ACCESS_POLICY	0x10020CF
#define SS_CREATE_FLAG_RESET			0xEF

#define SO_PIN_DEFAULT				"87654321"

bool mxl_tee_pin_sst_obj_exists = false;

int (*sse_secure_storage_create_open_fn)(sst_param_t *sst_param,
					sst_config_t *secure_store_config);
EXPORT_SYMBOL(sse_secure_storage_create_open_fn);

int (*sse_secure_storage_save_fn)(sst_data_param_t *sst_save_param,
					sst_config_t *secure_store_config);
EXPORT_SYMBOL(sse_secure_storage_save_fn);

int (*sse_secure_storage_load_fn)(sst_data_param_t *sst_load_param,
					sst_config_t *secure_store_config);
EXPORT_SYMBOL(sse_secure_storage_load_fn);

int (*sse_secure_storage_close_delete_fn)(sst_param_t *sst_del_param,
					sst_config_t *secure_store_config);
EXPORT_SYMBOL(sse_secure_storage_close_delete_fn);

#define MAX_KEY_LEN		8
#define SHA256_DIGEST_SIZE 	256
#define TEE_MAX_HASH_SIZE 	SHA256_DIGEST_SIZE
typedef struct {
	unsigned int  version;
	enum pin_hash_alg so_hash_alg;
	enum pin_hash_alg user_hash_alg;
	int so_hash_size;
	int user_hash_size;
	unsigned char so_pin_hash[TEE_MAX_HASH_SIZE];
	unsigned char user_pin_hash[TEE_MAX_HASH_SIZE];
} token_persistent_info_t;

static int compute_sha256_digest(unsigned char *data_to_hash, unsigned char *hash_digest)
{
	struct shash_desc *desc;
	struct crypto_shash *tfm;
	int ret = 0;

	pr_info("Request for sha256 data encryption.\n");

	/* Allocate the SHASH transformation object */
	tfm = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(tfm)) {
		return PTR_ERR(tfm);
	}

	/* Allocate the SHASH descriptor */
	desc = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		crypto_free_shash(tfm);
		return -ENOMEM;
	}
	desc->tfm = tfm;

	/* Initialize the hash */
	ret = crypto_shash_init(desc);
	if (ret) {
		goto out_free_desc;
	}

	/* Update the hash with data */
	crypto_shash_update(desc, data_to_hash, strlen(data_to_hash));
	if (ret) {
		goto out_free_desc;
	}

	/* Finalize and get the hash digest */
	ret = crypto_shash_final(desc, hash_digest);
	if (ret) {
		goto out_free_desc;
	}

	/* Now, hash_digest contains the SHA256 hash of input */
	pr_info("SHA256 hash computed successfully.\n");

out_free_desc:
	kfree(desc);
	crypto_free_shash(tfm);

	return ret;
}

static int encrypt_password(unsigned char *data_to_hash, unsigned char *hash_digest)
{
	return compute_sha256_digest(data_to_hash, hash_digest);
}

void set_object_config(sst_obj_config_t *pxSstConfig, uint32_t unObjectConfig)
{
	/* Fill the policy access permission flag */
	pxSstConfig->access_perm.u.field.user = unObjectConfig & 3;  /* rw for user */
	unObjectConfig = unObjectConfig >> 2;
	pxSstConfig->access_perm.u.field.group = unObjectConfig & 3; /* rw for group */
	unObjectConfig = unObjectConfig >> 2;
	pxSstConfig->access_perm.u.field.others = unObjectConfig & 3; /* access for others */
	unObjectConfig = unObjectConfig >> 2;
	pxSstConfig->access_perm.u.field.pname = unObjectConfig & 3;   /* rw for user */
	unObjectConfig = unObjectConfig >> 2;

	/* fill policy attribute members*/
	pxSstConfig->policy_attr.u.field.lock = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.no_load_to_userspace = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.read_once = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;

	/* Ignore the uid, pname and gid bit */
	pxSstConfig->policy_attr.u.field.ignore_uid = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.ignore_gid = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.ignore_pname = 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.wrap_flag = unObjectConfig & 3;
	unObjectConfig = unObjectConfig >> 2;
	/* Admin/Normal node access */
	pxSstConfig->policy_attr.u.field.admin_store = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.tee_only = unObjectConfig & 1;
	unObjectConfig = unObjectConfig >> 1;
	pxSstConfig->policy_attr.u.field.reserve = unObjectConfig & 0x3F;
	unObjectConfig = unObjectConfig >> 6;

	/* Crypto mode flag */
	pxSstConfig->crypto_mode_flag = unObjectConfig & 0xFF;

}

void display_object_config(sst_obj_config_t *pxSstConfig)
{
	pr_debug("user permission: 0x%x\n", pxSstConfig->access_perm.u.field.user);
	pr_debug("group permission: 0x%x\n", pxSstConfig->access_perm.u.field.group);
	pr_debug("other permission: 0x%x\n", pxSstConfig->access_perm.u.field.others);
	pr_debug("pname permission: 0x%x\n", pxSstConfig->access_perm.u.field.pname);
	pr_debug("lock bit: 0x%x\n", pxSstConfig->policy_attr.u.field.lock);
	pr_debug("load_to_userspace: 0x%x\n", pxSstConfig->policy_attr.u.field.no_load_to_userspace);
	pr_debug("read once bit: 0x%x\n", pxSstConfig->policy_attr.u.field.read_once);
	pr_debug("ignore uid bit: 0x%x\n", pxSstConfig->policy_attr.u.field.ignore_uid);
	pr_debug("ignore gid bit: 0x%x\n", pxSstConfig->policy_attr.u.field.ignore_gid);
	pr_debug("ignore pname bit: 0x%x\n", pxSstConfig->policy_attr.u.field.ignore_pname);
	pr_debug("crypto mode flag: 0x%x\n", pxSstConfig->crypto_mode_flag);
	pr_debug("admin node flag: 0x%x\n", pxSstConfig->policy_attr.u.field.admin_store);
	pr_debug("wrap flag: 0x%x\n", pxSstConfig->policy_attr.u.field.wrap_flag);
	pr_debug("wrap key location: 0x%x\n", pxSstConfig->wrap_asset.key_location);
	if (pxSstConfig->wrap_asset.key_location & KEY_IN_OTP) {
		pr_debug("wrap key asset ID: 0x%x\n", pxSstConfig->wrap_asset.u.asset_number);
	}
}

static void display_token_persistent_info(token_persistent_info_t *token_persistent_info)
{
	int i;

	pr_debug("version:%d\n", token_persistent_info->version);
	pr_debug("so_hash_alg:%u\n", token_persistent_info->so_hash_alg);
	pr_debug("user_hash_alg: %u\n", token_persistent_info->user_hash_alg);
	pr_debug("so_hash_size:%d\n", token_persistent_info->so_hash_size);
	pr_debug("user_hash_size:%d\n", token_persistent_info->user_hash_size);

	if (token_persistent_info->so_hash_size) {
		pr_debug("so_pin_hash:0x");
		for (i = 0; i < token_persistent_info->so_hash_size; i++) {
			pr_debug("%02x", token_persistent_info->so_pin_hash[i]);
		}
	}

	if (token_persistent_info->user_hash_size) {
		pr_debug("user_pin_hash:0x");
		for (i = 0; i < token_persistent_info->user_hash_size; i++) {
			pr_debug("%02x", token_persistent_info->user_pin_hash[i]);
		}
	}
}

static void populate_sst_param(sst_param_t *sst_param)
{
	set_object_config(&sst_param->sst_access_policy,
			 MXL_TEE_PIN_SST_OBJNAME_ACCESS_POLICY);
	display_object_config(&sst_param->sst_access_policy);

	sst_param->objectname = MXL_TEE_PIN_SST_OBJNAME;
	sst_param->sobject_len = strlen(MXL_TEE_PIN_SST_OBJNAME);
	if (mxl_tee_pin_sst_obj_exists == false)
		sst_param->secure_store_flags = sst_param->sst_access_policy.crypto_mode_flag | SS_CREATE;
	pr_debug("objname:[%s] SST Flags:0x%x \n", sst_param->objectname, sst_param->secure_store_flags);

	return;
}

static void populate_secure_store_config(sst_config_t *secure_store_config)
{
	const struct cred *cred = NULL;

	/* Get uid & GID & process name */
	cred = current_cred();
	secure_store_config->uid_val = cred->uid.val;
	secure_store_config->gid_val = cred->gid.val;
	strncpy(secure_store_config->pname, current->comm, TASK_COMM_LEN);
	secure_store_config->policy.policy_attr.u.field.admin_store = true;

	return;
}

static s32 mxl_tee_securestore_create_open(struct mxltee_driver *drv,
					sst_config_t *secure_store_config,
					sst_param_t *sst_param)
{
	s32 ret = 0;
	pr_debug("Create/Open Secure Storage Object.\n");

	populate_sst_param(sst_param);

	populate_secure_store_config(secure_store_config);

	if (sse_secure_storage_create_open_fn) {
		ret = sse_secure_storage_create_open_fn(sst_param, secure_store_config);
	} else {
		pr_err("sse_secure_storage_create_open_fn not available.\n");
		ret = -ENOENT;
	}
	if (ret < 0) {
		if (ret == -SST_OBJ_ALREADY_EXIST_ERR) {
			pr_info("secure storage object (%s) exits.\n", MXL_TEE_PIN_SST_OBJNAME);
			sst_param->secure_store_flags = sst_param->sst_access_policy.crypto_mode_flag & SS_CREATE_FLAG_RESET;
			ret = sse_secure_storage_create_open_fn(sst_param, secure_store_config);
			if (ret < 0) {
				pr_err("secure storage Failed to open the Object(%s):ret:%d\n", MXL_TEE_PIN_SST_OBJNAME, ret);
				goto finish;
			} else {
				pr_info("secure storage to open success ret:%d\n", ret);
				mxl_tee_pin_sst_obj_exists = true;
			}
		} else {
			pr_err("secure storage Failed to create the Object(%s):ret:%d\n", MXL_TEE_PIN_SST_OBJNAME, ret);
			goto finish;
		}
	} else {
		pr_info("secure storage to open success ret:%d\n", ret);
		mxl_tee_pin_sst_obj_exists = true;
	}

finish:
	return ret;
}

static s32 mxl_tee_securestore_close(struct mxltee_driver *drv,
                                        sst_config_t *secure_store_config,
                                        sst_param_t *sst_param)
{
	s32 ret = 0;
	pr_debug("Create/Open Secure Storage Object.\n");
	if (sse_secure_storage_close_delete_fn) {
		ret = sse_secure_storage_close_delete_fn(sst_param, secure_store_config);
	}
	return ret;

}

static s32 mxl_tee_securestore_save(sst_config_t *secure_store_config,
				sshandle_t ss_handle,
				token_persistent_info_t *token_persistent_info)
{
	s32 ret = 0;
	sst_data_param_t xSaveObject = {0};

	pr_info("Save data to Secure Storage Object.\n");
	pr_debug("Data to be saved in SST Object:\n");
	display_token_persistent_info(token_persistent_info);

	if (sse_secure_storage_save_fn) {
		xSaveObject.ss_handle = ss_handle;
		xSaveObject.payload_len = sizeof(token_persistent_info_t);
		xSaveObject.payload = (const unsigned char *)token_persistent_info;

		ret = sse_secure_storage_save_fn(&xSaveObject, secure_store_config);
	} else {
		pr_err("sse_secure_storage_save_fn not available.\n");
		ret = -ENOENT;
	}

	if (ret < 0) {
		pr_err("secure storage Failed to save :ret:%d\n", ret);
		goto finish;
	} else {
		pr_info("secure storage save success ret:%d\n", ret);
	}

finish:
	return ret;
}

static s32 mxl_tee_securestore_load(sst_config_t *secure_store_config,
				sshandle_t ss_handle,
				token_persistent_info_t *token_persistent_info)
{
	s32 ret = 0;
	sst_data_param_t xLoadObject = {0};

	pr_debug("Load data from Secure Storage Object.\n");

	if (sse_secure_storage_load_fn) {
		xLoadObject.ss_handle = ss_handle;
		xLoadObject.payload_len = sizeof(token_persistent_info_t);
		xLoadObject.payload = (const unsigned char *)token_persistent_info;

		ret = sse_secure_storage_load_fn(&xLoadObject, secure_store_config);
	} else {
		pr_err("sse_secure_storage_load_fn not available.\n");
		ret = -ENOENT;
	}

	if (ret < 0) {
		pr_err("secure storage Failed to load :ret:%d\n", ret);
		goto finish;
	} else {
		pr_info("secure storage load success ret:%d\n", ret);
	}

	pr_debug("Data read from SST Object:\n");
	display_token_persistent_info(token_persistent_info);

finish:
	return ret;
}

s32 handle_initpin_command(struct mxltee_driver *drv, struct mxltee_session *session,
		u32 num_params, struct tee_param *param)
{
	s32 ret = 0;
	sst_config_t *secure_store_config = NULL;
	sshandle_t ss_handle = 0;
	token_persistent_info_t token_persistent_info = {0};
#ifdef DEBUG
	token_persistent_info_t token_persistent_info_load = {0};
#endif
	sst_param_t *sst_param = NULL;
	sst_param = (sst_param_t *)kzalloc(sizeof(sst_param_t), GFP_DMA);
	if (sst_param == NULL) {
		pr_err("Allocation failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(sst_param, 0, sizeof(sst_param_t));

	secure_store_config = (void *)gen_pool_alloc(drv->iccpool, sizeof(sst_config_t));
	if (secure_store_config == NULL) {
		pr_err("Memory Allocation for secure storage struct failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(secure_store_config, 0, sizeof(sst_config_t));

	/* Create open SST object */
	ret = mxl_tee_securestore_create_open(drv, secure_store_config, sst_param);
	if (ret < 0) {
		pr_err("Securestore Object (%s) creation failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}
	ss_handle = sst_param->ss_handle;

	/* Generate hmac of the default pin */
	token_persistent_info.so_hash_size = TEE_MAX_HASH_SIZE;
	ret = encrypt_password(SO_PIN_DEFAULT, token_persistent_info.so_pin_hash);
	if (ret < 0) {
		pr_err("encrypt_password failed.\n");
		goto finish;
	}

	/* Save data in SST object */
	ret = mxl_tee_securestore_save(secure_store_config, sst_param->ss_handle, &token_persistent_info);
	if (ret < 0) {
		pr_err("Saving data to Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}

#ifdef DEBUG
	/* Load data from SST object */
	ret = mxl_tee_securestore_load(secure_store_config, sst_param->ss_handle, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("Loading data from Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}
#endif

finish:
	if(ss_handle)
		mxl_tee_securestore_close(drv, secure_store_config, sst_param);
	if(sst_param)
		kfree(sst_param);
	if (secure_store_config)
		gen_pool_free(drv->iccpool, (unsigned long)secure_store_config, sizeof(sst_config_t));

	return ret;
}

s32 authenicate_pin(seccrypto_pin_info_t *pin_info,
			token_persistent_info_t *token_persistent_info_load)
{
	s32 ret = 0;
	unsigned char pin_auth[MAX_KEY_LEN] = {0};
	unsigned char auth_pin_hash[TEE_MAX_HASH_SIZE] = {0};

	if (pin_info->pin_len <= MAX_KEY_LEN) {
		memcpy(pin_auth, pin_info->pin, pin_info->pin_len);
	} else {
		pr_err("Pin length %d is > MAX PIN LEN:%d\n", pin_info->pin_len, MAX_KEY_LEN);
		ret = -EINVAL;
		return ret;
	}

	ret = encrypt_password(pin_auth, auth_pin_hash);
	if (ret < 0) {
		pr_err("encrypt_password failed.\n");
		return ret;
	}

	if (pin_info->user_type == TYPE_USER) {
		pr_info("Authenticating USER pin.\n");

		if (memcmp((const void *)auth_pin_hash, (const void *)token_persistent_info_load->user_pin_hash,
					(size_t)token_persistent_info_load->user_hash_size) != 0) {
			pr_err("PIN not matched.\n");
			ret = -EINVAL;
		} else {
			pr_info("PIN matched.\n");
		}
	} else if (pin_info->user_type == TYPE_SO) {
		pr_info("Authenticating SO pin.\n");

		if (memcmp((const void *)auth_pin_hash, (const void *)token_persistent_info_load->so_pin_hash,
					(size_t)token_persistent_info_load->so_hash_size) != 0) {
			pr_err("PIN not matched.\n");
			ret = -EINVAL;
		} else {
			pr_info("PIN matched.\n");
		}
	} else {
		pr_err("Invalid User Type.\n");
		ret = -EINVAL;
	}

	return ret;
}

s32 handle_authpin_command(struct mxltee_driver *drv, struct mxltee_session *session,
		u32 num_params, struct tee_param *param)
{
	s32 ret = 0;
	sst_config_t *secure_store_config = NULL;
	sshandle_t ss_handle = 0;
	token_persistent_info_t token_persistent_info_load = {0};
	seccrypto_pin_info_t *pin_info = NULL;
	sst_param_t *sst_param = NULL;
	pin_info = param[0].u.memref.shm->kaddr;

	sst_param = (sst_param_t *)kzalloc(sizeof(sst_param_t), GFP_DMA);
	if (sst_param == NULL) {
		pr_err("Allocation failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(sst_param, 0, sizeof(sst_param_t));

	secure_store_config = (void *)gen_pool_alloc(drv->iccpool, sizeof(sst_config_t));
	if (secure_store_config == NULL) {
		pr_err("Memory Allocation for secure storage struct failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(secure_store_config, 0, sizeof(sst_config_t));

	/* Open SST object */
	ret = mxl_tee_securestore_create_open(drv, secure_store_config, sst_param);
	if (ret < 0) {
		pr_err("Securestore Object (%s) creation failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}
	ss_handle = sst_param->ss_handle;

	/* Load data from SST object */
	ret = mxl_tee_securestore_load(secure_store_config, sst_param->ss_handle, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("Loading data from Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}

	/* Auth with the pin */
	ret = authenicate_pin(pin_info, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("PIN Authenication failed\n");
		goto finish;
	} else {
		pr_info("PIN Authenication passed.\n");
	}

finish:
	if(ss_handle)
                mxl_tee_securestore_close(drv, secure_store_config, sst_param);
	if(sst_param)
		kfree(sst_param);
	if (secure_store_config)
		gen_pool_free(drv->iccpool, (unsigned long)secure_store_config, sizeof(sst_config_t));

	return ret;
}

s32 handle_setpin_command(struct mxltee_driver *drv, struct mxltee_session *session,
		u32 num_params, struct tee_param *param)
{
	s32 ret = 0;
	sst_config_t *secure_store_config = NULL;
	sshandle_t ss_handle = 0;
	token_persistent_info_t token_persistent_info_load = {0};
	seccrypto_pin_set_info_t *pin_info = NULL;
	sst_param_t *sst_param = NULL;
	unsigned char new_pin[MAX_KEY_LEN] = {0};

	pin_info = param[0].u.memref.shm->kaddr;

	sst_param = (sst_param_t *)kzalloc(sizeof(sst_param_t), GFP_DMA);
	if (sst_param == NULL) {
		pr_err("Allocation failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(sst_param, 0, sizeof(sst_param_t));

	secure_store_config = (void *)gen_pool_alloc(drv->iccpool, sizeof(sst_config_t));
	if (secure_store_config == NULL) {
		pr_err("Memory Allocation for secure storage struct failed. \r\n");
		ret = -ENOMEM;
		goto finish;
	}
	memset(secure_store_config, 0, sizeof(sst_config_t));

	/* Open SST object */
	ret = mxl_tee_securestore_create_open(drv, secure_store_config, sst_param);
	if (ret < 0) {
		pr_err("Securestore Object (%s) creation failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}

	/* Load data from SST object */
	ret = mxl_tee_securestore_load(secure_store_config, sst_param->ss_handle, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("Loading data from Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}
	ss_handle = sst_param->ss_handle;
	/* Auth with the pin */
	if (pin_info->old_pin_info.pin_len) {
		ret = authenicate_pin(&pin_info->old_pin_info, &token_persistent_info_load);
		if (ret < 0) {
			pr_err("PIN Authenication failed\n");
			goto finish;
		}
	} else {
		pr_debug("PIN Authenication not opted.\n");
	}

	if (copy_from_user(new_pin, pin_info->new_pin, pin_info->new_pin_len)) {
		pr_err("copy_from_user failed.\n");
		ret = -EFAULT;
		goto finish;
	}

	/* SET/Modify the PIN */
	if (pin_info->old_pin_info.user_type == TYPE_USER) {
		pr_info("Set USER pin.\n");

		token_persistent_info_load.user_hash_alg = pin_info->old_pin_info.pin_hash_algo;
		token_persistent_info_load.user_hash_size = TEE_MAX_HASH_SIZE;

		ret = encrypt_password(new_pin, token_persistent_info_load.user_pin_hash);
		if (ret < 0) {
			pr_err("encrypt_password failed.\n");
			goto finish;
		}

	} else if (pin_info->old_pin_info.user_type == TYPE_SO) {
		pr_info("Set SO pin.\n");

		token_persistent_info_load.so_hash_alg = pin_info->old_pin_info.pin_hash_algo;
		token_persistent_info_load.so_hash_size = TEE_MAX_HASH_SIZE;

		ret = encrypt_password(new_pin, token_persistent_info_load.so_pin_hash);
		if (ret < 0) {
			pr_err("encrypt_password failed.\n");
			goto finish;
		}

	} else {
		pr_err("Invalid User Type.\n");
	}

	/* Save data in SST object */
	ret = mxl_tee_securestore_save(secure_store_config, sst_param->ss_handle, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("Saving data to Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}

#ifdef DEBUG
	/* Load data from SST object */
	memset(&token_persistent_info_load, 0x0, sizeof(token_persistent_info_t));
	ret = mxl_tee_securestore_load(secure_store_config, sst_param->ss_handle, &token_persistent_info_load);
	if (ret < 0) {
		pr_err("Loading data from Securestore Object (%s) failed\n", MXL_TEE_PIN_SST_OBJNAME);
		goto finish;
	}
#endif

finish:
	if(ss_handle)
                mxl_tee_securestore_close(drv, secure_store_config, sst_param);
	if (sst_param)
		kfree(sst_param);
	if (secure_store_config)
		gen_pool_free(drv->iccpool, (unsigned long)secure_store_config, sizeof(sst_config_t));

	return ret;
}
