// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MaxLinear, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/tee_drv.h>
#include <linux/genalloc.h>
#include <linux/dma-direct.h>
#include <linux/uaccess.h>

#include <linux/icc/drv_icc.h>
#include <tep_interface.h>

#define IV_SIZE				(64/8)
#define WRAP_DATA_SIZE(len)	((len + IV_SIZE - 1) & ~(IV_SIZE - 1))
#define	ENC_DATA_SIZE(len)	((WRAP_DATA_SIZE(len) + IV_SIZE))
#define	DEC_DATA_SIZE(len)	((WRAP_DATA_SIZE(len) - IV_SIZE))

#define ECDSA_P256_LENGTH	(256/8)
#define ECDSA_P384_LENGTH	(384/8)

static void print_sst_details(struct secure_storage_params_tep *sst)
{
	pr_debug("sst_handle	: 0x%llx\n", sst->handle);
	pr_debug("sst_uid		: %u\n", sst->uid);
	pr_debug("sst_gid		: %u\n", sst->gid);
	pr_debug("sst_pname		: %s\n", sst->pname);
	pr_debug("sst_perm		: %u\n", sst->access_perm);
	pr_debug("sst_policy	: %u\n", sst->policy_attr);
	pr_debug("sst_crypto	: %u\n", sst->crypto_mode_flag);
}

static void print_key_ctx(struct ecdsa_key_pair_tep *key_pair)
{
	struct ec_public_key_tep *pub = NULL;
	struct ecdsa_private_key_tep *pri = NULL;


	pub = &key_pair->pub_key;
	pri = &key_pair->pri_key;

	pr_debug("pub->modulus.num_len	:	%u\n", pub->modulus.num_len);
	pr_debug("pub->modulus.num_ptr	:	0x%x\n", pub->modulus.num_ptr);
	pr_debug("pub->a.num_len		:	%u\n", pub->a.num_len);
	pr_debug("pub->a.num_ptr		:	0x%x\n", pub->a.num_ptr);
	pr_debug("pub->b.num_len		:	%u\n", pub->b.num_len);
	pr_debug("pub->b.num_ptr		:	0x%x\n", pub->b.num_ptr);
	pr_debug("pub->g_order.num_len	:	%u\n", pub->g_order.num_len);
	pr_debug("pub->g_order.num_ptr	:	0x%x\n", pub->g_order.num_ptr);
	pr_debug("pub->Gx.num_len		:	%u\n", pub->Gx.num_len);
	pr_debug("pub->Gx.num_ptr		:	0x%x\n", pub->Gx.num_ptr);
	pr_debug("pub->Gy.num_len		:	%u\n", pub->Gy.num_len);
	pr_debug("pub->Gy.num_ptr		:	0x%x\n", pub->Gy.num_ptr);
	pr_debug("pri->pri_key.num_len	:	%u\n", pri->pri_key.num_len);
	pr_debug("pri->pri_key.num_ptr	:	0x%x\n", pri->pri_key.num_ptr);
}

static void print_asymencdec_struct(struct seccrypto_asym_enc_dec_tep *asymencdec)
{
	pr_debug("struct seccrypto_asym_enc_dec_tep => size:%lu\n", sizeof(*asymencdec));
	pr_debug("encod_schm		: %d\n", asymencdec->encod_schm);
	pr_debug("hash_alg		: %d\n", asymencdec->hash_alg);
	pr_debug("asym_flags		: %d\n", asymencdec->asym_flags);
	pr_debug("input_data ptr       	: 0x%x\n", asymencdec->input_data);
	pr_debug("input_data_len       	: %u\n", asymencdec->input_data_len);
	pr_debug("additional_input ptr 	: 0x%x\n", asymencdec->additional_input);
	pr_debug("additional_input_len 	: %u\n", asymencdec->additional_input_len);
	pr_debug("output_data ptr		: 0x%x\n", asymencdec->output_data);
	pr_debug("output_data_len       	: %u\n", asymencdec->output_data_len);
}

static void print_keygen_struct(struct seccrypto_gen_key_tep *gen_key)
{
	pr_debug("struct seccrypto_gen_key => size:%lu\n", sizeof(*gen_key));
	pr_debug("genkey_algo	: %d\n", gen_key->genkey_algo);
	pr_debug("hash_algo		: %d\n", gen_key->hash_algo);
	pr_debug("genkey_flag	: %d\n", gen_key->flag);
	print_sst_details(&gen_key->sst_params);
	pr_debug("key_ctx		: 0x%x\n", gen_key->key_ptr);
	pr_debug("key_len		: %u\n", gen_key->key_len);
	pr_debug("public_attribute_size: %d\n", gen_key->public_attribute_size);
	pr_debug("public_key_attribute ptr: %u\n", gen_key->public_key_attribute);
	pr_debug("private_attribute_size: %d\n", gen_key->private_attribute_size);
	pr_debug("private_key_attribute ptr: %u\n", gen_key->private_key_attribute);
}

static void print_keyload_struct(struct seccrypto_load_key_tep *loadkey)
{
	pr_debug("struct seccrypto_load_key =>\n");
	print_sst_details(&loadkey->sst_params);
	pr_debug("loadkey_algo	: %d\n", loadkey->load_algo);
	pr_debug("hash_algo		: %d\n", loadkey->hash_algo);
	pr_debug("loadkey_flag	: %d\n", loadkey->load_flags);
	pr_debug("key_type		: %d\n", loadkey->key_type);
}

static void print_attr_struct(struct seccrypto_set_attribute_tep *attr)
{
	pr_debug("struct seccrypto_set_attribute_tep => size:%lu\n", sizeof(*attr));
	pr_debug("attr type	: %d\n", attr->type);
	pr_debug("key_attribute_size: %d\n", attr->key_attribute_size);
	pr_debug("key_attribute ptr: %u\n", attr->key_attributes);
}

static void atom_key_context_init(void *buffer, uint32_t len, enum sec_alg algo,
		enum gen_key_flags flag)
{
	struct ecdsa_key_pair *key_pair = buffer;
	struct ec_public_key *pub = NULL;
	struct ecdsa_private_key *pri = NULL;


	pub = &key_pair->pub_key;
	pri = &key_pair->pri_key;
	switch (algo) {
	case SEC_ALG_ECDSA_P256:
		pub->modulus.num_len = ECDSA_P256_LENGTH;
		pub->a.num_len = ECDSA_P256_LENGTH;
		pub->b.num_len = ECDSA_P256_LENGTH;
		pub->g_order.num_len = ECDSA_P256_LENGTH;
		pub->Gx.num_len = ECDSA_P256_LENGTH;
		pub->Gy.num_len = ECDSA_P256_LENGTH;
		pub->Qx.num_len = ECDSA_P256_LENGTH;
		pub->Qy.num_len = ECDSA_P256_LENGTH;
		if (flag == SEC_CRYPTO_RETURN_KEY_PAIR_PLAIN)
			pri->pri_key.num_len = ECDSA_P256_LENGTH;
		else
			pri->pri_key.num_len = ENC_DATA_SIZE(ECDSA_P256_LENGTH);
		break;
	case SEC_ALG_ECDSA_P384:
		pub->modulus.num_len = ECDSA_P384_LENGTH;
		pub->a.num_len = ECDSA_P384_LENGTH;
		pub->b.num_len = ECDSA_P384_LENGTH;
		pub->g_order.num_len = ECDSA_P384_LENGTH;
		pub->Gx.num_len = ECDSA_P384_LENGTH;
		pub->Gy.num_len = ECDSA_P384_LENGTH;
		pub->Qx.num_len = ECDSA_P384_LENGTH;
		pub->Qy.num_len = ECDSA_P384_LENGTH;
		if (flag == SEC_CRYPTO_RETURN_KEY_PAIR_PLAIN)
			pri->pri_key.num_len = ECDSA_P384_LENGTH;
		else
			pri->pri_key.num_len = ENC_DATA_SIZE(ECDSA_P384_LENGTH);
		break;
	default:
		pr_debug("algorithm %d is not supported\n", algo);
		return;
	}
	pub->modulus.num_ptr = buffer + sizeof(*key_pair);
	pub->a.num_ptr = pub->modulus.num_ptr + pub->modulus.num_len;
	pub->b.num_ptr = pub->a.num_ptr + pub->a.num_len;
	pub->g_order.num_ptr = pub->b.num_ptr + pub->b.num_len;
	pub->Gx.num_ptr = pub->g_order.num_ptr + pub->g_order.num_len;
	pub->Gy.num_ptr = pub->Gx.num_ptr + pub->Gx.num_len;
	pub->Qx.num_ptr = pub->Gy.num_ptr + pub->Gy.num_len;
	pub->Qy.num_ptr = pub->Qx.num_ptr + pub->Qx.num_len;
	pri->pri_key.num_ptr = pub->Qy.num_ptr + pub->Qy.num_len;
}

static void tep_key_context_init(dma_addr_t dma, void *buffer, uint32_t len,
		enum sec_alg algo, enum gen_key_flags flag)
{
	struct ecdsa_key_pair_tep *key_pair = buffer;
	struct ec_public_key_tep *pub = NULL;
	struct ecdsa_private_key_tep *pri = NULL;


	pub = &key_pair->pub_key;
	pri = &key_pair->pri_key;
	switch (algo) {
	case SEC_ALG_ECDSA_P256:
		pub->modulus.num_len = ECDSA_P256_LENGTH;
		pub->a.num_len = ECDSA_P256_LENGTH;
		pub->b.num_len = ECDSA_P256_LENGTH;
		pub->g_order.num_len = ECDSA_P256_LENGTH;
		pub->Gx.num_len = ECDSA_P256_LENGTH;
		pub->Gy.num_len = ECDSA_P256_LENGTH;
		pub->Qx.num_len = ECDSA_P256_LENGTH;
		pub->Qy.num_len = ECDSA_P256_LENGTH;
		if (flag == SEC_CRYPTO_RETURN_KEY_PAIR_PLAIN)
			pri->pri_key.num_len = ECDSA_P256_LENGTH;
		else
			pri->pri_key.num_len = ENC_DATA_SIZE(ECDSA_P256_LENGTH);
		break;
	case SEC_ALG_ECDSA_P384:
		pub->modulus.num_len = ECDSA_P384_LENGTH;
		pub->a.num_len = ECDSA_P384_LENGTH;
		pub->b.num_len = ECDSA_P384_LENGTH;
		pub->g_order.num_len = ECDSA_P384_LENGTH;
		pub->Gx.num_len = ECDSA_P384_LENGTH;
		pub->Gy.num_len = ECDSA_P384_LENGTH;
		pub->Qx.num_len = ECDSA_P384_LENGTH;
		pub->Qy.num_len = ECDSA_P384_LENGTH;
		if (flag == SEC_CRYPTO_RETURN_KEY_PAIR_PLAIN)
			pri->pri_key.num_len = ECDSA_P384_LENGTH;
		else
			pri->pri_key.num_len = ENC_DATA_SIZE(ECDSA_P384_LENGTH);
		break;
	default:
		pr_debug("algorithm %d is not supported\n", algo);
		return;
	}
	pub->modulus.num_ptr = dma + sizeof(*key_pair);
	memset(bus_to_virt(pub->modulus.num_ptr), 0x20, pub->modulus.num_len);
	pub->a.num_ptr = pub->modulus.num_ptr + pub->modulus.num_len;
	pub->b.num_ptr = pub->a.num_ptr + pub->a.num_len;
	pub->g_order.num_ptr = pub->b.num_ptr + pub->b.num_len;
	pub->Gx.num_ptr = pub->g_order.num_ptr + pub->g_order.num_len;
	pub->Gy.num_ptr = pub->Gx.num_ptr + pub->Gx.num_len;
	pub->Qx.num_ptr = pub->Gy.num_ptr + pub->Gy.num_len;
	pub->Qy.num_ptr = pub->Qx.num_ptr + pub->Qx.num_len;
	pri->pri_key.num_ptr = pub->Qy.num_ptr + pub->Qy.num_len;
}

static void map_tep_key_context_to_atom(struct ecdsa_key_pair_tep *tep, struct ecdsa_key_pair *atom)
{
	atom->pub_key.modulus.num_len = tep->pub_key.modulus.num_len;
	memcpy(atom->pub_key.modulus.num_ptr, bus_to_virt(tep->pub_key.modulus.num_ptr), tep->pub_key.modulus.num_len);

	atom->pub_key.a.num_len = tep->pub_key.a.num_len;
	memcpy(atom->pub_key.a.num_ptr, bus_to_virt(tep->pub_key.a.num_ptr), tep->pub_key.a.num_len);

	atom->pub_key.b.num_len = tep->pub_key.b.num_len;
	memcpy(atom->pub_key.b.num_ptr, bus_to_virt(tep->pub_key.b.num_ptr), tep->pub_key.b.num_len);

	atom->pub_key.g_order.num_len = tep->pub_key.g_order.num_len;
	memcpy(atom->pub_key.g_order.num_ptr, bus_to_virt(tep->pub_key.g_order.num_ptr), tep->pub_key.g_order.num_len);

	atom->pub_key.Gx.num_len = tep->pub_key.Gx.num_len;
	memcpy(atom->pub_key.Gx.num_ptr, bus_to_virt(tep->pub_key.Gx.num_ptr), tep->pub_key.Gx.num_len);

	atom->pub_key.Gy.num_len = tep->pub_key.Gy.num_len;
	memcpy(atom->pub_key.Gy.num_ptr, bus_to_virt(tep->pub_key.Gy.num_ptr), tep->pub_key.Gy.num_len);

	atom->pub_key.Qx.num_len = tep->pub_key.Qx.num_len;
	memcpy(atom->pub_key.Qx.num_ptr, bus_to_virt(tep->pub_key.Qx.num_ptr), tep->pub_key.Qx.num_len);

	atom->pub_key.Qy.num_len = tep->pub_key.Qy.num_len;
	memcpy(atom->pub_key.Qy.num_ptr, bus_to_virt(tep->pub_key.Qy.num_ptr), tep->pub_key.Qy.num_len);

	atom->pri_key.pri_key.num_len = tep->pri_key.pri_key.num_len;
	memcpy(atom->pri_key.pri_key.num_ptr, bus_to_virt(tep->pri_key.pri_key.num_ptr), tep->pri_key.pri_key.num_len);
}

static int keygen_params_validate(struct seccrypto_gen_key *ugenkey,
		uint32_t ugenkey_size, struct seccrypto_gen_key_tep *genkey)
{
	struct secure_storage_params *sst_params = NULL;

	if (ugenkey_size < sizeof(*genkey)) {
		pr_debug("keygen: keyegen parameter is invalid\n");
		return -EINVAL;
	}

	if ((ugenkey->genkey_algo != SEC_ALG_ECDSA_P256) && (ugenkey->genkey_algo != SEC_ALG_ECDSA_P384)) {
		pr_debug("keygen: algorithm type is not supported\n");
		return -ENOTSUPP;
	}
	if ((ugenkey->flag < SEC_CRYPTO_RETURN_KEY_PAIR) || (ugenkey->flag > SEC_CRYPTO_RETURN_KEY_NONE)) {
		pr_debug("keygen: flag option is invalid\n");
		return -EINVAL;
	}
	if ((ugenkey->flag != SEC_CRYPTO_RETURN_KEY_NONE) && !ugenkey->key_ptr) {
		pr_debug("keygen: key context pointer is invalid\n");
		return -EINVAL;
	}

	sst_params = &ugenkey->sst_params;
	if (sst_params) {
		genkey->sst_params.handle = sst_params->handle;
		genkey->sst_params.access_perm = sst_params->access_perm;
		genkey->sst_params.policy_attr = sst_params->policy_attr;
		genkey->sst_params.crypto_mode_flag = sst_params->crypto_mode_flag;
		fill_secure_storage_params(&genkey->sst_params);
	} else {
		pr_debug("keygen: secure storage parameters required for key generation\n");
		return -EINVAL;
	}

	genkey->genkey_algo = ugenkey->genkey_algo;
	genkey->hash_algo = get_hash_algo(genkey->genkey_algo);
	genkey->flag = ugenkey->flag;
	if (ugenkey->flag == SEC_CRYPTO_RETURN_KEY_NONE)
		genkey->key_len = 0;
	else
		genkey->key_len = ugenkey->key_len;
	genkey->public_attribute_size = ugenkey->public_attribute_size;
	genkey->private_attribute_size = ugenkey->private_attribute_size;

	return TEEC_SUCCESS;
}

static int keyload_params_validate(uint32_t num_params, struct tee_param *param, struct seccrypto_load_key_tep *loadkey)
{
	struct secure_storage_params *sst_params = NULL;
	struct seccrypto_load_key *uloadkey = NULL;
	size_t uloadkey_size = 0;


	uloadkey = param[0].u.memref.shm->kaddr;
	uloadkey_size = param[0].u.memref.shm->size;
	if (!uloadkey || (uloadkey_size < sizeof(*loadkey))) {
		pr_debug("keyload: keyload parameter is invalid\n");
		return -EINVAL;
	}

	if ((uloadkey->load_algo < SEC_ALG_RSA_2048) || (uloadkey->load_algo >= SEC_ALG_MAX)) {
		pr_debug("keyload: alogorithm type is invalid\n");
		return -ENOTSUPP;
	}
	if ((uloadkey->hash_algo < RSA_PKCS1_5_SHA1) || (uloadkey->hash_algo > ECDSA_ASN1_SHA384)) {
		pr_debug("keyload: hash alogorithm type is not supported\n");
		return -ENOTSUPP;
	}
	if ((uloadkey->load_flags < PRIVATE_KEY_BLOB) || (uloadkey->load_flags >= MAX_LOAD_KEY_FLAG)) {
		pr_debug("keyload: options are invalid\n");
		return -EINVAL;
	}
	if ((uloadkey->key_type < KEY_AES) || (uloadkey->key_type > KEY_RSA)) {
		pr_debug("keyload: key type is invalid\n");
		return -EINVAL;
	}

	sst_params = &uloadkey->sst_params;
	if (sst_params) {
		loadkey->sst_params.handle = sst_params->handle;
		loadkey->sst_params.access_perm = sst_params->access_perm;
		loadkey->sst_params.policy_attr = sst_params->policy_attr;
		loadkey->sst_params.crypto_mode_flag = sst_params->crypto_mode_flag;
		fill_secure_storage_params(&loadkey->sst_params);
	} else {
		pr_debug("keyload: secure storage parameters required\n");
		return -EINVAL;
	}

	loadkey->load_algo = uloadkey->load_algo;
	loadkey->hash_algo = uloadkey->hash_algo;
	loadkey->load_flags = uloadkey->load_flags;
	loadkey->key_type = uloadkey->key_type;
	return TEEC_SUCCESS;
}

int handle_keyload_command(struct mxltee_driver *drv, struct mxltee_session *session,
		uint32_t num_params, struct tee_param *param)
{
	struct active_session_param *active_session = NULL;
	dma_addr_t dma_active_session = 0x0;
	struct seccrypto_load_key_tep *loadkey = NULL;
	dma_addr_t dma_loadkey = 0x0;
	icc_msg_t icc_msg = {0};
	int ret = 0;

	active_session = get_active_scsa_session(drv, session, TA_SECURE_CRYPTO_LOAD_KEY,
			&dma_active_session);
	if (IS_ERR_OR_NULL(active_session)) {
		pr_debug("keyload: memory allocation failed for session\n");
		return PTR_ERR(active_session);
	}

	loadkey = (void *)gen_pool_alloc(drv->iccpool, sizeof(*loadkey));
	if (!loadkey) {
		pr_debug("keyload: memory allocation failed for key load\n");
		return -ENOMEM;
	}
	memset(loadkey, 0x0, sizeof(*loadkey));

	ret =  keyload_params_validate(num_params, param, loadkey);
	if (ret < 0)
		goto gen_free;

	dma_loadkey = dma_map_single_attrs(drv->iccdev, loadkey, sizeof(*loadkey),
			DMA_BIDIRECTIONAL, DMA_ATTR_NON_CONSISTENT);
	if (dma_mapping_error(drv->iccdev, dma_loadkey)) {
		pr_debug("keyload: DMA mapping failed for key load structure\n");
		ret = -ENOMEM;
		goto gen_free;
	}
	dma_sync_single_for_device(drv->iccdev, dma_loadkey, sizeof(*loadkey), DMA_TO_DEVICE);

	icc_msg.src_client_id = SECURE_SIGN_SERVICE;
	icc_msg.dst_client_id = SECURE_SIGN_SERVICE;
	icc_msg.msg_id = ICC_CMD_ID_INVOKE_CMD;
	icc_msg.param_attr = ICC_PARAM_PTR | (ICC_PARAM_PTR_NON_IOCU << 1);
	icc_msg.param[0] = dma_active_session;
	icc_msg.param[1] = dma_loadkey;
	icc_msg.param[2] = LOADKEY_VERSION_1;

	print_keyload_struct(loadkey);

	ret = icc_write_and_read(&icc_msg);
	if (ret < 0) {
		pr_err("keyload: icc_write_and_read failed, ret:%d\n",ret);
		goto dma_unmap;
	}

	ret = validate_icc_reply(&icc_msg, session->session_id);
	if (ret < 0) {
		pr_err("keyload: validate_icc_reply failed, ret:%d\n",ret);
	}

dma_unmap:
	if (dma_loadkey) {
		dma_sync_single_for_cpu(drv->iccdev, dma_loadkey, sizeof(*loadkey),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_loadkey, sizeof(*loadkey),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
gen_free:
	if (loadkey)
		gen_pool_free(drv->iccpool, (unsigned long)loadkey, sizeof(struct seccrypto_load_key));
	if (active_session) {
		dma_sync_single_for_cpu(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		gen_pool_free(drv->iccpool, (unsigned long)active_session, sizeof(*active_session));
	}
	return ret;
}

int handle_keygen_command(struct mxltee_driver *drv, struct mxltee_session *session,
		uint32_t num_params, struct tee_param *param)
{
	struct active_session_param *active_session = NULL;
	struct seccrypto_gen_key_tep *genkey = NULL;
	struct seccrypto_gen_key *ugenkey = NULL;
	void *genkey_out_buffer = NULL;
	void *genkey_public_attribute_buffer = NULL;
	void *genkey_private_attribute_buffer = NULL;
	dma_addr_t dma_genkey_out_buffer = 0x0;
	dma_addr_t dma_genkey_public_attribute_buffer = 0x0;
	dma_addr_t dma_genkey_private_attribute_buffer = 0x0;
	dma_addr_t dma_active_session = 0x0;
	dma_addr_t dma_genkey = 0x0;
	uint32_t ugenkey_size = 0;
	icc_msg_t icc_msg = {0};
	int ret = 0;

	active_session = get_active_scsa_session(drv, session, TA_SECURE_CRYPTO_GEN_KEYPAIR,
			&dma_active_session);
	if (IS_ERR_OR_NULL(active_session)) {
		pr_debug("keygen: memory allocation failed for session\n");
		return PTR_ERR(active_session);
	}

	genkey = (void *)gen_pool_alloc(drv->iccpool, sizeof(*genkey));
	if (!genkey) {
		pr_debug("keygen: memory allocation failed for key generation\n");
		return -ENOMEM;
	}
	memset(genkey, 0x0, sizeof(*genkey));

	ugenkey = param[0].u.memref.shm->kaddr;
	ugenkey_size = param[0].u.memref.shm->size;
	ret =  keygen_params_validate(ugenkey, ugenkey_size, genkey);
	if (ret < 0)
		goto gen_free;

	if (genkey->key_len) {
		genkey_out_buffer = (void *)gen_pool_alloc(drv->iccpool, genkey->key_len);
		if (!genkey_out_buffer) {
			pr_debug("keygen: memory allocation failed for key generation key_len:%u\n", genkey->key_len);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(genkey_out_buffer, 0x0, genkey->key_len);

		dma_genkey_out_buffer = dma_map_single_attrs(drv->iccdev, genkey_out_buffer, genkey->key_len,
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_genkey_out_buffer)) {
			pr_debug("keygen: DMA mapping failed for key generation\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		tep_key_context_init(dma_genkey_out_buffer, genkey_out_buffer, genkey->key_len, genkey->genkey_algo, genkey->flag);
		print_key_ctx(genkey_out_buffer);
		dma_sync_single_for_device(drv->iccdev, dma_genkey_out_buffer, genkey->key_len, DMA_TO_DEVICE);
		genkey->key_ptr = dma_genkey_out_buffer;
	}

	if (genkey->public_attribute_size) {
		genkey_public_attribute_buffer = (void *)gen_pool_alloc(drv->iccpool, genkey->public_attribute_size);
		if (!genkey_public_attribute_buffer) {
			pr_debug("keygen: memory allocation failed for key public attribute public_attribute_size:%u\n",
					 genkey->public_attribute_size);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(genkey_public_attribute_buffer, 0x0, genkey->public_attribute_size);
		if (copy_from_user(genkey_public_attribute_buffer, ugenkey->public_key_attribute, ugenkey->public_attribute_size))
			return -EFAULT;
		printk(KERN_INFO "public_key_attribute content => %*phD\n", genkey->public_attribute_size, (uint32_t *)genkey_public_attribute_buffer);

		dma_genkey_public_attribute_buffer = dma_map_single_attrs(drv->iccdev,
									genkey_public_attribute_buffer,
									genkey->public_attribute_size,
									DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_genkey_public_attribute_buffer)) {
			pr_debug("keygen: DMA mapping failed for genkey public attribute buffer\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_genkey_public_attribute_buffer,
						 genkey->public_attribute_size, DMA_TO_DEVICE);
		genkey->public_key_attribute = dma_genkey_public_attribute_buffer;
	}

	if (genkey->private_attribute_size) {
		genkey_private_attribute_buffer = (void *)gen_pool_alloc(drv->iccpool, genkey->private_attribute_size);
		if (!genkey_private_attribute_buffer) {
			pr_debug("keygen: memory allocation failed for key private attribute private_attribute_size:%u\n",
					 genkey->private_attribute_size);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(genkey_private_attribute_buffer, 0x0, genkey->private_attribute_size);
		if (copy_from_user(genkey_private_attribute_buffer, ugenkey->private_key_attribute, ugenkey->private_attribute_size))
			return -EFAULT;
		printk(KERN_INFO "private_key_attribute content => %*phD\n", genkey->private_attribute_size, (uint32_t *)genkey_private_attribute_buffer);

		dma_genkey_private_attribute_buffer = dma_map_single_attrs(drv->iccdev,
									genkey_private_attribute_buffer,
									genkey->private_attribute_size,
									DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_genkey_private_attribute_buffer)) {
			pr_debug("keygen: DMA mapping failed for genkey private attribute buffer\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_genkey_private_attribute_buffer,
						 genkey->private_attribute_size, DMA_TO_DEVICE);
		genkey->private_key_attribute = dma_genkey_private_attribute_buffer;
	}

	dma_genkey = dma_map_single_attrs(drv->iccdev, genkey, sizeof(*genkey),
			DMA_BIDIRECTIONAL, DMA_ATTR_NON_CONSISTENT);
	if (dma_mapping_error(drv->iccdev, dma_genkey)) {
		pr_debug("keygen: DMA mapping failed for key generation\n");
		ret = -ENOMEM;
		goto dma_unmap;
	}
	dma_sync_single_for_device(drv->iccdev, dma_genkey, sizeof(*genkey), DMA_TO_DEVICE);

	icc_msg.src_client_id = SECURE_SIGN_SERVICE;
	icc_msg.dst_client_id = SECURE_SIGN_SERVICE;
	icc_msg.msg_id = ICC_CMD_ID_INVOKE_CMD;
	icc_msg.param_attr = ICC_PARAM_PTR | (ICC_PARAM_PTR_NON_IOCU << 1);
	icc_msg.param[0] = dma_active_session;
	icc_msg.param[1] = dma_genkey;
	icc_msg.param[2] = BASE_VERSION_1;

	print_keygen_struct(genkey);

	ret = icc_write_and_read(&icc_msg);
	dma_sync_single_for_cpu(drv->iccdev, dma_genkey_out_buffer, genkey->key_len, DMA_FROM_DEVICE);
	if (ret < 0)
		goto dma_unmap;

	ret = validate_icc_reply(&icc_msg, session->session_id);
	if (ret < 0)
		goto dma_unmap;

	ret = icc_msg.param[2];
	pr_debug("keygen: key length:%d\n", ret);
	if (genkey->key_len) {
		atom_key_context_init(ugenkey->key_ptr, ugenkey->key_len, ugenkey->genkey_algo, genkey->flag);
		map_tep_key_context_to_atom(genkey_out_buffer, ugenkey->key_ptr);
	}
	ugenkey->key_len = ret;

dma_unmap:
	if (dma_genkey) {
		dma_sync_single_for_cpu(drv->iccdev, dma_genkey, sizeof(*genkey), DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_genkey, sizeof(*genkey), DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_genkey_out_buffer)
		dma_unmap_single_attrs(drv->iccdev, dma_genkey_out_buffer, genkey->key_len, DMA_FROM_DEVICE, DMA_ATTR_NON_CONSISTENT);
	if (dma_genkey_public_attribute_buffer)
		dma_unmap_single_attrs(drv->iccdev, dma_genkey_public_attribute_buffer,
					genkey->public_attribute_size, DMA_FROM_DEVICE, DMA_ATTR_NON_CONSISTENT);
	if (dma_genkey_private_attribute_buffer)
		dma_unmap_single_attrs(drv->iccdev, dma_genkey_private_attribute_buffer,
					genkey->public_attribute_size, DMA_FROM_DEVICE, DMA_ATTR_NON_CONSISTENT);

gen_free:
	if (genkey_out_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)genkey_out_buffer, genkey->key_len);
	if (genkey_public_attribute_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)genkey_public_attribute_buffer, genkey->public_attribute_size);
	if (genkey_private_attribute_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)genkey_private_attribute_buffer, genkey->public_attribute_size);
	if (genkey)
		gen_pool_free(drv->iccpool, (unsigned long)genkey, sizeof(*genkey));

	if (active_session) {
		dma_sync_single_for_cpu(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		gen_pool_free(drv->iccpool, (unsigned long)active_session, sizeof(*active_session));
	}
	return ret;
}

static int attr_params_validate(uint32_t num_params, struct tee_param *param, struct seccrypto_set_attribute_tep *attr, struct secure_storage_params_tep *attr_sst_params)
{
	struct secure_storage_params *sst_params = NULL;
	struct seccrypto_set_attribute *uattr = NULL;
	size_t uattr_size = 0;

	uattr = param[0].u.memref.shm->kaddr;
	uattr_size = param[0].u.memref.shm->size;
	if (!uattr || (uattr_size < sizeof(*attr))) {
		pr_err("set attribute: set attribute parameter is invalid\n");
		return -EINVAL;
	}

	if ((uattr->type < ATTRIBUTE_TYPE_PUBLIC) || (uattr->type > ATTRIBUTE_TYPE_PRIVATE)) {
		pr_err("set attribute: attribute type is invalid\n");
		return -ENOTSUPP;
	}

	if (uattr->key_attribute_size <= 0) {
		pr_err("set attribute: key attribute size is invalid [%d]\n", attr->key_attribute_size);
		return -ENOTSUPP;
	}

	sst_params = &uattr->sst_params;
	if (sst_params) {
		attr_sst_params->handle = sst_params->handle;
		attr_sst_params->access_perm = sst_params->access_perm;
		attr_sst_params->policy_attr = sst_params->policy_attr;
		attr_sst_params->crypto_mode_flag = sst_params->crypto_mode_flag;
		fill_secure_storage_params(attr_sst_params);
	} else {
		pr_err("set attribute: secure storage parameters required\n");
		return -EINVAL;
	}

	attr->type = uattr->type;
	attr->key_attribute_size = uattr->key_attribute_size;
	return TEEC_SUCCESS;
}

int handle_attribute_set_command(struct mxltee_driver *drv, struct mxltee_session *session,
		uint32_t num_params, struct tee_param *param)
{
	struct active_session_param *active_session = NULL;
	struct seccrypto_set_attribute_tep *attr = NULL;
	struct seccrypto_set_attribute *uattr = NULL;
	struct secure_storage_params_tep *attr_sst_params = NULL;
	void *key_attribute_buffer = NULL;
	dma_addr_t dma_key_attributes_buffer = 0x0;
	dma_addr_t dma_active_session = 0x0;
	dma_addr_t dma_attr = 0x0;
	dma_addr_t dma_attr_sst_params = 0x0;
	uint32_t uattr_size = 0;
	icc_msg_t icc_msg = {0};
	int ret = 0;

	active_session = get_active_scsa_session(drv, session, TA_SECURE_CRYPTO_SET_ATTRIBUTE,
			&dma_active_session);
	if (IS_ERR_OR_NULL(active_session)) {
		pr_err("set attribute: memory allocation failed for session\n");
		return PTR_ERR(active_session);
	}

	attr = (void *)gen_pool_alloc(drv->iccpool, sizeof(*attr));
	if (!attr) {
		pr_err("set attribute: memory allocation failed for key generation\n");
		return -ENOMEM;
	}
	memset(attr, 0x0, sizeof(*attr));

	attr_sst_params = (void *)gen_pool_alloc(drv->iccpool, sizeof(struct secure_storage_params_tep));
	if (!attr_sst_params) {
		pr_err("set attribute: memory allocation failed for sst params\n");
		return -ENOMEM;
	}
	memset(attr_sst_params, 0x0, sizeof(struct secure_storage_params_tep));

	uattr = param[0].u.memref.shm->kaddr;
	uattr_size = param[0].u.memref.shm->size;
	ret =  attr_params_validate(num_params, param, attr, (struct secure_storage_params_tep *) attr_sst_params);
	if (ret < 0)
		goto attr_free;

	dma_attr_sst_params = dma_map_single_attrs(drv->iccdev, attr_sst_params, sizeof(struct secure_storage_params_tep),
			DMA_BIDIRECTIONAL, DMA_ATTR_NON_CONSISTENT);
	if (dma_mapping_error(drv->iccdev, dma_attr_sst_params)) {
		pr_err("set attribute: DMA mapping failed for key attribute set\n");
		ret = -ENOMEM;
		goto dma_unmap;
	}
	dma_sync_single_for_device(drv->iccdev, dma_attr_sst_params, sizeof(struct secure_storage_params_tep), DMA_TO_DEVICE);
	attr->sst_params = dma_attr_sst_params;

	if (attr->key_attribute_size) {
		key_attribute_buffer = (void *)gen_pool_alloc(drv->iccpool, attr->key_attribute_size);
		if (!key_attribute_buffer) {
			pr_err("set attribute: memory allocation failed for key attribute key_attribute_size:%u\n",
					 attr->key_attribute_size);
			ret = -ENOMEM;
			goto attr_free;
		}
		memset(key_attribute_buffer, 0x0, attr->key_attribute_size);
		if (copy_from_user(key_attribute_buffer, uattr->key_attributes,
					 uattr->key_attribute_size))
			return -EFAULT;
		printk(KERN_INFO "key_attribute content => %*phD\n",
				attr->key_attribute_size,
				(uint32_t *)key_attribute_buffer);

		dma_key_attributes_buffer = dma_map_single_attrs(drv->iccdev,
						key_attribute_buffer,
						attr->key_attribute_size,
						DMA_TO_DEVICE,
						DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_key_attributes_buffer)) {
			pr_err("set attribute: DMA mapping failed for key attribute buffer\n");
			ret = -ENOMEM;
			goto attr_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_key_attributes_buffer,
						 attr->key_attribute_size, DMA_TO_DEVICE);
		attr->key_attributes = dma_key_attributes_buffer;
	}

	dma_attr = dma_map_single_attrs(drv->iccdev, attr, sizeof(*attr),
			DMA_BIDIRECTIONAL, DMA_ATTR_NON_CONSISTENT);
	if (dma_mapping_error(drv->iccdev, dma_attr)) {
		pr_err("set attribute: DMA mapping failed for key attribute set\n");
		ret = -ENOMEM;
		goto dma_unmap;
	}
	dma_sync_single_for_device(drv->iccdev, dma_attr, sizeof(*attr), DMA_TO_DEVICE);

	icc_msg.src_client_id = SECURE_SIGN_SERVICE;
	icc_msg.dst_client_id = SECURE_SIGN_SERVICE;
	icc_msg.msg_id = ICC_CMD_ID_INVOKE_CMD;
	icc_msg.param_attr = ICC_PARAM_PTR | (ICC_PARAM_PTR_NON_IOCU << 1);
	icc_msg.param[0] = dma_active_session;
	icc_msg.param[1] = dma_attr;

	print_attr_struct(attr);
	print_sst_details(attr_sst_params);

	ret = icc_write_and_read(&icc_msg);
	if (ret < 0)
		goto dma_unmap;

	ret = validate_icc_reply(&icc_msg, session->session_id);
	if (ret < 0)
		goto dma_unmap;

	ret = icc_msg.param[2];

dma_unmap:
	if (dma_attr) {
		dma_sync_single_for_cpu(drv->iccdev, dma_attr, sizeof(*attr), DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_attr, sizeof(*attr), DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_attr_sst_params) {
		dma_sync_single_for_cpu(drv->iccdev, dma_attr_sst_params, sizeof(struct secure_storage_params_tep), DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_attr_sst_params, sizeof(struct secure_storage_params_tep), DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_key_attributes_buffer)
		dma_unmap_single_attrs(drv->iccdev, dma_key_attributes_buffer,
					attr->key_attribute_size, DMA_FROM_DEVICE, DMA_ATTR_NON_CONSISTENT);

attr_free:
	if (key_attribute_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)key_attribute_buffer, attr->key_attribute_size);
	if (attr)
		gen_pool_free(drv->iccpool, (unsigned long)attr, sizeof(*attr));

	if (active_session) {
		dma_sync_single_for_cpu(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		gen_pool_free(drv->iccpool, (unsigned long)active_session, sizeof(*active_session));
	}
	return ret;
}

static int asymencdec_params_validate(struct seccrypto_asym_enc_dec *uasymencdec, 
		uint32_t asymencdec_size, struct seccrypto_asym_enc_dec_tep *asymencdec)
{
	if (!asymencdec || (asymencdec_size < sizeof(*asymencdec))) {
		pr_debug("asym enc dec: asymencdec parameter is invalid\n");
		return -EINVAL;
	}

	if ((uasymencdec->encod_schm < RSA_ENCOD_OAEP) || (uasymencdec->encod_schm > RSA_ENCOD_PKCS1_V1_5)) {
		pr_debug("keyload: alogorithm type is invalid\n");
		return -ENOTSUPP;
	}
	if ((uasymencdec->hash_alg < ASYM_ENC_HASH_SHA1) || (uasymencdec->hash_alg > ASYM_ENC_HASH_SHA512)) {
		pr_debug("keyload: hash alogorithm type is not supported\n");
		return -ENOTSUPP;
	}
	if ((uasymencdec->asym_flags < ASYM_FLAG_OAEP_ADDNL_INPUT_HASH) || (uasymencdec->asym_flags > ASYM_FLAG_OAEP_ADDNL_INPUT_DATA)) {
		pr_debug("keyload: options are invalid\n");
		return -EINVAL;
	}

	asymencdec->hash_alg = uasymencdec->hash_alg;
	asymencdec->encod_schm = uasymencdec->encod_schm;
	asymencdec->asym_flags = uasymencdec->asym_flags;
	asymencdec->input_data_len = uasymencdec->input_data_len;
	asymencdec->additional_input_len = uasymencdec->additional_input_len;
	asymencdec->output_data_len = uasymencdec->output_data_len;

	return TEEC_SUCCESS;
}

s32 handle_asym_decrypt_command(struct mxltee_driver *drv, struct mxltee_session *session,
		uint32_t num_params, struct tee_param *param)
{
	int ret = 0;
	uint32_t uasymencdec_size;
	struct active_session_param *active_session = NULL;
	struct seccrypto_asym_enc_dec *uasymencdec = NULL;
	struct seccrypto_asym_enc_dec_tep *asymencdec = NULL;
	void *asymencdec_out_buffer = NULL;
	void *asymencdec_data_buffer = NULL;
	void *asymencdec_additional_input_buffer = NULL;
	dma_addr_t dma_active_session = 0x0;
	dma_addr_t dma_asymencdec = 0x0;
	dma_addr_t dma_asymencdec_out_buffer = 0x0;
	dma_addr_t dma_asymencdec_data_buffer = 0x0;
	dma_addr_t dma_asymencdec_additional_input_buffer = 0x0;
	icc_msg_t icc_msg = {0};

	active_session = get_active_scsa_session(drv, session, 
			TA_SECURE_CRYPTO_ASYM_DECRYPT, &dma_active_session);
	if (IS_ERR_OR_NULL(active_session)) {
		pr_debug("keyload: memory allocation failed for session\n");
		return PTR_ERR(active_session);
	}

	asymencdec = (void *)gen_pool_alloc(drv->iccpool, sizeof(*asymencdec));
	if (!asymencdec) {
		pr_debug("asym enc dec: memory allocation failed for asym enc dec.\n");
		return -ENOMEM;
	}
	memset(asymencdec, 0x0, sizeof(*asymencdec));

	uasymencdec = param[0].u.memref.shm->kaddr;
	uasymencdec_size = param[0].u.memref.shm->size;

	ret = asymencdec_params_validate(uasymencdec, uasymencdec_size, asymencdec);
	if (ret < 0)
		goto gen_free;

	if (uasymencdec->output_data_len) {
		asymencdec_out_buffer = (void *)gen_pool_alloc(drv->iccpool, uasymencdec->output_data_len);
		if (!asymencdec_out_buffer) {
			pr_debug("asym enc dec: memory allocation failed for asym enc dec output_data_len:%u\n", uasymencdec->output_data_len);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(asymencdec_out_buffer, 0x0, uasymencdec->output_data_len);

		dma_asymencdec_out_buffer = dma_map_single_attrs(drv->iccdev, asymencdec_out_buffer, uasymencdec->output_data_len,
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_asymencdec_out_buffer)) {
			pr_debug("asym enc dec: DMA mapping failed for asym enc dec\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_asymencdec_out_buffer, uasymencdec->output_data_len, DMA_TO_DEVICE);
		asymencdec->output_data = dma_asymencdec_out_buffer;
	}

	if (uasymencdec->input_data_len) {
		asymencdec_data_buffer = (void *)gen_pool_alloc(drv->iccpool, uasymencdec->input_data_len);
		if (!asymencdec_data_buffer) {
			pr_debug("asym enc dec: memory allocation failed for key public attribute input_data_len:%u\n",
					 uasymencdec->input_data_len);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(asymencdec_data_buffer, 0x0, uasymencdec->input_data_len);
		if (copy_from_user(asymencdec_data_buffer, uasymencdec->input_data, uasymencdec->input_data_len))
			return -EFAULT;
		printk(KERN_INFO "data content => %*phD\n", uasymencdec->input_data_len, (uint32_t *)asymencdec_data_buffer);

		dma_asymencdec_data_buffer = dma_map_single_attrs(drv->iccdev,
									asymencdec_data_buffer,
									uasymencdec->input_data_len,
									DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_asymencdec_data_buffer)) {
			pr_debug("asym enc dec: DMA mapping failed for uasymencdec public attribute buffer\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_asymencdec_data_buffer,
						 uasymencdec->input_data_len, DMA_TO_DEVICE);
		asymencdec->input_data = dma_asymencdec_data_buffer;
	}

	if (uasymencdec->additional_input_len) {
		asymencdec_additional_input_buffer = (void *)gen_pool_alloc(drv->iccpool, uasymencdec->additional_input_len);
		if (!asymencdec_additional_input_buffer) {
			pr_debug("asym enc dec: memory allocation failed for key public attribute additional_input_len:%u\n",
					 uasymencdec->additional_input_len);
			ret = -ENOMEM;
			goto gen_free;
		}
		memset(asymencdec_additional_input_buffer, 0x0, uasymencdec->additional_input_len);
		if (copy_from_user(asymencdec_additional_input_buffer, uasymencdec->additional_input, uasymencdec->additional_input_len))
			return -EFAULT;
		printk(KERN_INFO "additional_input content => %*phD\n", uasymencdec->additional_input_len, (uint32_t *)asymencdec_additional_input_buffer);

		dma_asymencdec_additional_input_buffer = dma_map_single_attrs(drv->iccdev,
									asymencdec_additional_input_buffer,
									uasymencdec->additional_input_len,
									DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		if (dma_mapping_error(drv->iccdev, dma_asymencdec_additional_input_buffer)) {
			pr_debug("asym enc dec: DMA mapping failed for uasymencdec public attribute buffer\n");
			ret = -ENOMEM;
			goto gen_free;
		}
		dma_sync_single_for_device(drv->iccdev, dma_asymencdec_additional_input_buffer,
						 uasymencdec->additional_input_len, DMA_TO_DEVICE);
		asymencdec->additional_input = dma_asymencdec_additional_input_buffer;
	}

	dma_asymencdec = dma_map_single_attrs(drv->iccdev, asymencdec, sizeof(*asymencdec),
			DMA_BIDIRECTIONAL, DMA_ATTR_NON_CONSISTENT);
	if (dma_mapping_error(drv->iccdev, dma_asymencdec)) {
		pr_debug("asymencdec: DMA mapping failed for asym enc dec structure\n");
		ret = -ENOMEM;
		goto gen_free;
	}
	dma_sync_single_for_device(drv->iccdev, dma_asymencdec, sizeof(*asymencdec), DMA_TO_DEVICE);

	icc_msg.src_client_id = SECURE_SIGN_SERVICE; /* REE_TEE_SERVICE */
	icc_msg.dst_client_id = SECURE_SIGN_SERVICE; /* TEP_SCSA_CLIENT_ID */
	icc_msg.msg_id = ICC_CMD_ID_INVOKE_CMD;
	icc_msg.param_attr = ICC_PARAM_PTR | (ICC_PARAM_PTR_NON_IOCU << 1);
	icc_msg.param[0] = dma_active_session;
	icc_msg.param[1] = dma_asymencdec;

	print_asymencdec_struct(asymencdec);

	ret = icc_write_and_read(&icc_msg);
	if (ret < 0)
		goto dma_unmap;

	ret = validate_icc_reply(&icc_msg, session->session_id);
	if (ret < 0)
		goto dma_unmap;

	ret = icc_msg.param[2];
	pr_debug("asym enc dec:: data length:%d [%d]\n", ret, asymencdec->output_data_len);
	uasymencdec->output_data_len = asymencdec->output_data_len;
	memcpy(uasymencdec->output_data, bus_to_virt(asymencdec->output_data), asymencdec->output_data_len);
	asymencdec->output_data_len = ret;

dma_unmap:
	if (dma_asymencdec) {
		dma_sync_single_for_cpu(drv->iccdev, dma_asymencdec, sizeof(*asymencdec),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_asymencdec, sizeof(*asymencdec),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_asymencdec_out_buffer) {
		dma_sync_single_for_cpu(drv->iccdev, dma_asymencdec_out_buffer, uasymencdec->output_data_len,
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_asymencdec_out_buffer, uasymencdec->output_data_len,
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_asymencdec_data_buffer) {
		dma_sync_single_for_cpu(drv->iccdev, dma_asymencdec_data_buffer, uasymencdec->input_data_len,
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_asymencdec_data_buffer, uasymencdec->input_data_len,
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}
	if (dma_asymencdec_additional_input_buffer) {
		dma_sync_single_for_cpu(drv->iccdev, dma_asymencdec_additional_input_buffer, uasymencdec->additional_input_len,
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_asymencdec_additional_input_buffer, uasymencdec->additional_input_len,
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
	}

gen_free:
	if (asymencdec)
		gen_pool_free(drv->iccpool, (unsigned long)asymencdec, sizeof(struct seccrypto_asym_enc_dec_tep));
	if (asymencdec_out_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)asymencdec_out_buffer, uasymencdec->output_data_len);
	if (asymencdec_data_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)asymencdec_data_buffer, uasymencdec->input_data_len);
	if (asymencdec_additional_input_buffer)
		gen_pool_free(drv->iccpool, (unsigned long)asymencdec_additional_input_buffer, uasymencdec->additional_input_len);
	if (active_session) {
		dma_sync_single_for_cpu(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE);
		dma_unmap_single_attrs(drv->iccdev, dma_active_session, sizeof(*active_session),
				DMA_TO_DEVICE, DMA_ATTR_NON_CONSISTENT);
		gen_pool_free(drv->iccpool, (unsigned long)active_session, sizeof(*active_session));
	}

	return ret;
}
