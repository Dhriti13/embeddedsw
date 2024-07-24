/******************************************************************************
* Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file	xilsecure_versal_net_keyunwrap_client_example.c
* @addtogroup xsecure_keyunwrap_example_apis XilSecure Key Unwrap API Example Usage
* @{
* This example illustrates on how to unwrap the wrapped key using IPI calls.
* To build this application, xilmailbox library must be included in BSP and xilsecure
* must be in client mode
*
* @note
* Procedure to link and compile the example for the default ddr less designs
* ------------------------------------------------------------------------------------------------------------
* The default linker settings places a software stack, heap and data in DDR memory. For this example to work,
* any data shared between client running on A72/R5/PL and server running on PMC, should be placed in area
* which is accessible to both client and server.
*
* Following is the procedure to compile the example on OCM or any memory region which can be accessed by server
*
*		1. Open example linker script(lscript.ld) in Vitis project and section to memory mapping should
*			be updated to point all the required sections to shared memory(OCM or TCM)
*			using a memory region drop down selection
*
*						OR
*
*		1. In linker script(lscript.ld) user can add new memory section in source tab as shown below
*			.sharedmemory : {
*   			. = ALIGN(4);
*   			__sharedmemory_start = .;
*   			*(.sharedmemory)
*   			*(.sharedmemory.*)
*   			*(.gnu.linkonce.d.*)
*   			__sharedmemory_end = .;
* 			} > versal_cips_0_pspmc_0_psv_ocm_ram_0_psv_ocm_ram_0
*
* 		2. In this example ".data" section elements that are passed by reference to the server-side should
* 		   be stored in the above shared memory section. To make it happen in below example,
*		   replace ".data" in attribute section with ".sharedmemory". For example,
* 		   static XSecure_KeyWrapData KeyWrapData __attribute__ ((aligned (64)))
*					__attribute__ ((section (".data.KeyWrapData")));
* 					should be changed to
* 		   static XSecure_KeyWrapData KeyWrapData __attribute__ ((aligned (64)))
* 	                __attribute__ ((section (".sharedmemory.KeyWrapData")));
*
* To keep things simple, by default the cache is disabled for this example
*
* MODIFICATION HISTORY:
* <pre>
* Ver   Who    Date     Changes
* ----- ------ -------- -------------------------------------------------
* 1.0   kpt    07/03/23 Initial release
* 5.3   kpt    12/13/23 Added support for RSA CRT
* 5.3   ng     01/28/24 Added SDT support
* 5.4   kpt    06/13/24 Add AES key wrap with padding support
*
* </pre>
******************************************************************************/

/***************************** Include Files *********************************/
#include "xil_cache.h"
#include "xil_util.h"
#include "xsecure_plat_client.h"
#include "xsecure_katclient.h"
#include "xsecure_rsaclient.h"
#include "xocp_client.h"

#ifdef SDT
#include "xsecure_config.h"
#endif

/************************** Constant Definitions *****************************/

#define XSECURE_AES_CMK_SIZE_IN_BYTES (40U) /**< CMK size in bytes */

#define XSECURE_AES_CMK_WRAP_DATA	   "e3c2ee8876b89396ddbe1409a7b5cd0e9030eb1dfaa1a8898d6d0d97f72f00ebe8017b69a4ba48f3"
											/**< Wrapped CMK key using AES */

#define XSECURE_RSA_PUB_EXP_SIZE	  (4U) /**< Public exponent size */

#define XSECURE_KEY_WRAP_PUB_KEY_OFFSET (0U) /** Public key offset */

#define XSECURE_ECC_SIGN_TOTAL_LEN		(96U) /** ECC signature length */

/**************************** Type Definitions *******************************/

typedef struct {
	u8 PubKey[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES + XSECURE_RSA_PUB_EXP_SIZE]; /* Public Key */
	u8 CertBuf[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES]; /* Cerificate buffer which will be provided by user */
} XSecure_KeyWrapBuf;

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

static void XSecure_PrintData(const u8 *Data, u32 Size);

/************************** Variable Definitions *****************************/

#if (XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES == 384U)
static u8 RsaEncodedData[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES] __attribute__ ((aligned (64)))
__attribute__ ((section (".data.WrappedKey"))) = {
		/**< Ephemeral key padded with RSA OAEP */
	0x00U, 0xBFU, 0xB5U, 0x83U, 0xBAU, 0x1FU, 0x32U, 0x12U, 0x1FU, 0x8EU, 0x46U, 0x9FU, 0x28U, 0x2EU, 0x44U, 0x4DU,
	0xCAU, 0x81U, 0x7BU, 0xA4U, 0x07U, 0xD8U, 0xE1U, 0xA8U, 0xC3U, 0x17U, 0xD6U, 0x24U, 0xC6U, 0xA2U, 0xA3U, 0x9AU,
	0x21U, 0xFFU, 0x89U, 0x06U, 0xC7U, 0x01U, 0x6BU, 0xDDU, 0xD6U, 0xF6U, 0x35U, 0xDAU, 0xCDU, 0x3CU, 0x0AU, 0xC4U,
	0xB3U, 0x77U, 0x6EU, 0xEBU, 0x6AU, 0xF0U, 0x2EU, 0x75U, 0xB0U, 0xD9U, 0x02U, 0x0FU, 0xE1U, 0x8BU, 0x88U, 0xB8U,
	0x6FU, 0xDDU, 0x60U, 0xA8U, 0x28U, 0xF2U, 0x73U, 0x01U, 0xB5U, 0x44U, 0x81U, 0x7FU, 0xA7U, 0xF3U, 0x8AU, 0x71U,
	0xE3U, 0x17U, 0x3AU, 0x8DU, 0xF1U, 0x38U, 0x5AU, 0x0BU, 0x77U, 0xDBU, 0xB4U, 0xDFU, 0x46U, 0x12U, 0xC8U, 0x91U,
	0x11U, 0xA2U, 0xE7U, 0xB7U, 0x46U, 0x3AU, 0xF1U, 0xCFU, 0x95U, 0x74U, 0x27U, 0x76U, 0xD8U, 0x5CU, 0x0CU, 0xD2U,
	0x9AU, 0x97U, 0x5FU, 0x34U, 0x42U, 0x78U, 0xFCU, 0x47U, 0x20U, 0x09U, 0xEEU, 0x81U, 0x9DU, 0xBCU, 0x1DU, 0x62U,
	0x8BU, 0x00U, 0xCAU, 0xD1U, 0x20U, 0x17U, 0x60U, 0x65U, 0x9AU, 0x81U, 0x5EU, 0x75U, 0xB6U, 0x71U, 0xFDU, 0x1BU,
	0x44U, 0x05U, 0x8CU, 0x88U, 0xB3U, 0x29U, 0x84U, 0x62U, 0x2BU, 0xC3U, 0x80U, 0x0FU, 0xF9U, 0x49U, 0x02U, 0x44U,
	0x31U, 0xD1U, 0x83U, 0x1CU, 0x13U, 0x1CU, 0xEFU, 0x11U, 0x67U, 0x5BU, 0x28U, 0x5CU, 0x6FU, 0x00U, 0xB9U, 0x75U,
	0x60U, 0xEAU, 0x3FU, 0x3FU, 0x65U, 0xDEU, 0x29U, 0x99U, 0xCDU, 0xA5U, 0x14U, 0x7FU, 0x62U, 0xE8U, 0x93U, 0x57U,
	0x7BU, 0x27U, 0x6EU, 0xB2U, 0x28U, 0x04U, 0x9DU, 0xC6U, 0xAEU, 0x10U, 0xB4U, 0x27U, 0x5DU, 0xD7U, 0x4FU, 0x0AU,
	0xDAU, 0x03U, 0x9FU, 0x39U, 0x71U, 0x7DU, 0x2FU, 0xBEU, 0x5FU, 0xE0U, 0x7CU, 0xA9U, 0x20U, 0xBBU, 0x9FU, 0xE8U,
	0x7AU, 0x90U, 0x00U, 0xF7U, 0x0EU, 0x61U, 0xBBU, 0xA0U, 0xE1U, 0x57U, 0x6AU, 0xAAU, 0xCCU, 0x7CU, 0xDAU, 0x85U,
	0x7DU, 0xE2U, 0x7FU, 0xD1U, 0x18U, 0x9FU, 0x81U, 0x1DU, 0x4EU, 0xFFU, 0xE1U, 0xA1U, 0x6DU, 0x57U, 0xCCU, 0xFCU,
	0xC7U, 0x0DU, 0x39U, 0xA4U, 0x7DU, 0x37U, 0xC1U, 0x50U, 0x52U, 0x43U, 0xF4U, 0x4EU, 0xD1U, 0x09U, 0x10U, 0xC1U,
	0xA2U, 0x1DU, 0xD0U, 0xDAU, 0xF5U, 0xACU, 0x1EU, 0x3BU, 0x12U, 0x98U, 0xC7U, 0x99U, 0x96U, 0x5FU, 0x77U, 0xC6U,
	0x3DU, 0x6FU, 0xB0U, 0xE9U, 0xAAU, 0x1AU, 0x2EU, 0xDAU, 0x39U, 0x25U, 0xDFU, 0x5EU, 0xA9U, 0x96U, 0x12U, 0x5CU,
	0xCDU, 0xE5U, 0x0AU, 0x15U, 0xD0U, 0x65U, 0xBDU, 0xA5U, 0xC4U, 0xC3U, 0x93U, 0x8DU, 0x8FU, 0x50U, 0x00U, 0xECU,
	0x37U, 0x7CU, 0x57U, 0x63U, 0x24U, 0xCAU, 0x87U, 0x47U, 0x77U, 0x34U, 0x4AU, 0x0DU, 0x3FU, 0x00U, 0x19U, 0x6DU,
	0xE7U, 0xA7U, 0x9DU, 0x59U, 0x5EU, 0x4EU, 0x27U, 0x2EU, 0xC4U, 0xD1U, 0xF0U, 0x6CU, 0xEFU, 0x89U, 0x3FU, 0xBDU,
	0xF4U, 0xB2U, 0x6EU, 0x07U, 0x75U, 0xB0U, 0x91U, 0x8BU, 0x8BU, 0xC4U, 0x72U, 0x81U, 0x23U, 0x04U, 0xD9U, 0xFAU,
	0x28U, 0x78U, 0xAFU, 0x13U, 0x9AU, 0x47U, 0xD9U, 0xBCU, 0xFCU, 0x60U, 0xE5U, 0x78U, 0xC8U, 0x8BU, 0x00U, 0xE3U,
};
#elif (XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES == 512U)
static u8 RsaEncodedData[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES] __attribute__ ((aligned (64)))
__attribute__ ((section (".data.WrappedKey"))) = {
		/**< Ephemeral key padded with RSA OAEP */
	0x00, 0xA9, 0x09, 0x25, 0x45, 0x41, 0x50, 0xEA, 0xEE, 0xEF, 0xA5, 0x53, 0x6C, 0x61, 0xC7, 0x59,
	0x59, 0xF2, 0x55, 0xC3, 0x08, 0x57, 0x0F, 0x4C, 0x2D, 0x0A, 0x4F, 0x6C, 0x92, 0xD1, 0x6E, 0x14,
	0xA5, 0x32, 0xB5, 0x8E, 0x62, 0x31, 0x2E, 0xEE, 0xE0, 0x35, 0xBC, 0xBC, 0xF9, 0x56, 0x5D, 0xAB,
	0x19, 0xC4, 0x26, 0xAC, 0xD6, 0xBE, 0xEA, 0xAE, 0x1F, 0x98, 0x9A, 0xB9, 0x78, 0x4D, 0xDD, 0xF0,
	0xC7, 0xA6, 0x5C, 0xC3, 0x95, 0xE9, 0xFA, 0xEF, 0x90, 0x46, 0xB0, 0x35, 0xB9, 0x34, 0xE3, 0x06,
	0xBE, 0x3C, 0xF8, 0x55, 0xCD, 0x1E, 0x2E, 0xD5, 0x28, 0x95, 0x87, 0xE0, 0x38, 0xFF, 0x11, 0xD2,
	0x43, 0x32, 0xAD, 0x04, 0xAE, 0x85, 0xC5, 0xCD, 0x0C, 0x84, 0x22, 0x53, 0x3E, 0x40, 0xBB, 0x24,
	0xDD, 0x32, 0x91, 0xD1, 0xFF, 0x16, 0xAB, 0x49, 0xED, 0x8B, 0xA9, 0x1F, 0x68, 0x2C, 0x0D, 0xBB,
	0xE5, 0xA9, 0xAD, 0xE3, 0xCE, 0x2F, 0x72, 0x58, 0xF9, 0x95, 0xE7, 0xEA, 0x23, 0xEF, 0xD9, 0x9B,
	0xC1, 0xB2, 0x06, 0x1F, 0x2C, 0x56, 0x15, 0xF3, 0x21, 0xDC, 0xCE, 0xC2, 0x84, 0xBF, 0xEB, 0x99,
	0x79, 0x0D, 0x83, 0xC4, 0xA1, 0x75, 0xB4, 0x86, 0x86, 0x46, 0x5A, 0xED, 0x32, 0xDA, 0x20, 0x6D,
	0x7D, 0x28, 0x6D, 0xAE, 0x69, 0xD1, 0x28, 0x12, 0x6C, 0x09, 0xDD, 0xE6, 0x13, 0xD4, 0x78, 0xDA,
	0x12, 0xE7, 0x6A, 0x47, 0x07, 0xA3, 0xED, 0x21, 0xB2, 0x56, 0xBE, 0x9D, 0x62, 0x15, 0x2A, 0x28,
	0x3F, 0x2B, 0x10, 0xA0, 0x7E, 0xCB, 0xF0, 0x15, 0xB5, 0xC9, 0x7B, 0x69, 0x38, 0x67, 0x76, 0xBB,
	0x02, 0x0E, 0xD4, 0xB8, 0xAC, 0x16, 0xE3, 0x1F, 0x09, 0xB8, 0x8B, 0x81, 0x85, 0x65, 0x4C, 0x06,
	0xD3, 0x4A, 0x70, 0x3E, 0x82, 0x0D, 0xDF, 0x77, 0x4A, 0xE3, 0x0A, 0x5C, 0x11, 0x14, 0x4B, 0x36,
	0x29, 0x5B, 0xED, 0xCB, 0xF3, 0xB7, 0x95, 0xBE, 0xD3, 0xBD, 0x31, 0x01, 0xB3, 0x29, 0x77, 0x04,
	0x1A, 0xDA, 0xB0, 0x0A, 0xCB, 0x87, 0xB0, 0x0C, 0x54, 0x54, 0xB0, 0x5F, 0xA8, 0x0C, 0x09, 0xD0,
	0x9F, 0xD0, 0x71, 0x38, 0xD6, 0xB2, 0x8E, 0xD8, 0x08, 0xE6, 0x80, 0x51, 0x68, 0x09, 0x22, 0xD5,
	0xD7, 0x98, 0x31, 0xD1, 0x2B, 0x0A, 0x9C, 0xAE, 0x20, 0x26, 0x5B, 0x35, 0x97, 0x7D, 0xE1, 0x4C,
	0xB2, 0xB0, 0x21, 0xE3, 0xE4, 0xB4, 0xD1, 0x3E, 0xFB, 0x6C, 0xCE, 0x40, 0xB5, 0xDA, 0x0E, 0xE0,
	0x69, 0xC5, 0x83, 0x07, 0xAB, 0x4C, 0xC9, 0x4D, 0xB0, 0x43, 0xF8, 0x10, 0x0A, 0x16, 0x80, 0xA1,
	0xC7, 0xDC, 0x7A, 0xC7, 0xC6, 0xC4, 0xFC, 0xF3, 0x77, 0x87, 0xCC, 0x57, 0xDD, 0xAA, 0xCB, 0x10,
	0x73, 0xC7, 0x16, 0xE3, 0xFC, 0xC2, 0xFE, 0x48, 0x3C, 0xCF, 0x94, 0x11, 0xFB, 0x7B, 0x08, 0x80,
	0xBD, 0xC4, 0xA1, 0x55, 0x3A, 0xDC, 0x85, 0xD4, 0x58, 0x1F, 0xA3, 0xF7, 0x8E, 0x0F, 0x30, 0xD6,
	0xC5, 0x1A, 0xD4, 0xC7, 0x1C, 0x9C, 0x7C, 0x29, 0x06, 0x58, 0x9A, 0xA8, 0x19, 0xD3, 0x5A, 0x5E,
	0xC1, 0xC5, 0xC8, 0x44, 0x37, 0x91, 0xDA, 0x2C, 0x75, 0xCE, 0xD4, 0xCC, 0xF8, 0xB1, 0xDB, 0xA1,
	0xCB, 0x49, 0xF0, 0x35, 0x86, 0xB8, 0x4F, 0xFC, 0xC8, 0xBB, 0x10, 0x40, 0xA7, 0x37, 0x5F, 0x18,
	0xAE, 0x2D, 0x4F, 0x35, 0x51, 0x15, 0x3E, 0xAF, 0x3F, 0x03, 0x94, 0xEE, 0x98, 0x4C, 0x66, 0xE7,
	0x04, 0x7E, 0x5D, 0xEC, 0x58, 0xB0, 0xCA, 0x37, 0x79, 0x5E, 0x2E, 0xF9, 0x60, 0x7D, 0x86, 0x41,
	0xED, 0x5F, 0xDC, 0x0C, 0x78, 0xC5, 0x04, 0xBE, 0x9D, 0xA3, 0x6D, 0x1C, 0x12, 0x09, 0x18, 0xBE,
	0x07, 0xBF, 0x2E, 0xFF, 0x3A, 0x04, 0xF0, 0x4B, 0x24, 0xD8, 0x04, 0xFA, 0xF0, 0xB0, 0x51, 0x8C
};
#elif (XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES == 256U)
static u8 RsaEncodedData[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES] __attribute__ ((aligned (64)))
__attribute__ ((section (".data.WrappedKey"))) = {
		/**< Ephemeral key padded with RSA OAEP */
	0x00, 0x9C, 0x9F, 0x8F, 0x55, 0x8D, 0x12, 0x98, 0x54, 0x3C, 0xE8, 0x6A, 0x55, 0xB8, 0x10, 0xCB,
	0xD0, 0x64, 0x35, 0xD4, 0x8D, 0xB8, 0x58, 0xC0, 0x49, 0x9D, 0xAF, 0x80, 0xEE, 0x87, 0x9A, 0xC7,
	0xB1, 0xAB, 0x58, 0x09, 0x3F, 0x4F, 0x00, 0x3E, 0xB3, 0xFA, 0xBD, 0x6E, 0x10, 0xFB, 0x1D, 0x34,
	0xDD, 0x2E, 0xF5, 0x18, 0xDC, 0xBE, 0xF3, 0xB9, 0x09, 0x04, 0xD8, 0xA3, 0x86, 0x0E, 0x1F, 0x7A,
	0xC8, 0xB2, 0x54, 0x0B, 0x09, 0xBC, 0xB1, 0x0D, 0x28, 0x2C, 0x9C, 0x87, 0x19, 0xEE, 0xDC, 0x4D,
	0x8B, 0x13, 0xA9, 0x88, 0x52, 0xBE, 0x6D, 0x34, 0x84, 0xFC, 0x0A, 0x9D, 0x43, 0x56, 0x5C, 0x0E,
	0xEF, 0x60, 0xA7, 0x58, 0xCC, 0x88, 0x94, 0x52, 0xF9, 0x68, 0x55, 0xAF, 0xF6, 0xED, 0xB5, 0x48,
	0x21, 0x98, 0x87, 0xEE, 0xCE, 0x98, 0x15, 0x07, 0x44, 0x38, 0x20, 0xC1, 0xF6, 0xB2, 0xC0, 0x64,
	0xCC, 0x00, 0xBE, 0x88, 0x12, 0x01, 0x39, 0xA0, 0x58, 0xBF, 0x33, 0x34, 0x86, 0x77, 0x58, 0xFA,
	0x46, 0x48, 0xD3, 0xEF, 0xB9, 0x9D, 0xF9, 0x0E, 0x9A, 0x00, 0x5C, 0x05, 0xA3, 0xD3, 0x34, 0xF2,
	0x36, 0x43, 0xC0, 0x22, 0x03, 0x53, 0xEA, 0x2B, 0x02, 0x4D, 0xEC, 0x73, 0xCE, 0x37, 0xBD, 0x5F,
	0x94, 0x45, 0x7E, 0xF4, 0x6D, 0x16, 0x8A, 0xE1, 0xC4, 0x82, 0x2D, 0x51, 0xA8, 0x17, 0x5D, 0xFA,
	0x9E, 0xA9, 0xEC, 0x6E, 0x4C, 0x26, 0x2F, 0x90, 0x1B, 0x43, 0x61, 0x10, 0xAF, 0x7F, 0x1E, 0x82,
	0x69, 0x2B, 0xDF, 0x5A, 0x0F, 0x1F, 0x65, 0x18, 0x92, 0x29, 0x75, 0x9C, 0xE7, 0x0A, 0x45, 0x85,
	0x64, 0x44, 0x5E, 0x70, 0x7E, 0x7D, 0x83, 0x4F, 0xBF, 0x21, 0xE7, 0x65, 0xE3, 0xE5, 0xEB, 0x67,
	0x61, 0xF2, 0x9F, 0x16, 0xA1, 0xA3, 0x2F, 0xEA, 0x45, 0x16, 0xA9, 0x61, 0x89, 0xA4, 0x8F, 0x83
};
#else
	static u8 RsaEncodedData[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES +
		XSECURE_AES_CMK_SIZE_IN_BYTES] __attribute__ ((aligned (64)));
#endif

static XSecure_KeyWrapData KeyWrapData __attribute__ ((aligned (64)))
__attribute__ ((section (".data.KeyWrapData")));

static u8 WrappedKey[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES +
		XSECURE_AES_CMK_SIZE_IN_BYTES] __attribute__ ((aligned (64)))
__attribute__ ((section (".data.WrappedKey")));

static XSecure_KeyWrapBuf KeyWrapBuf __attribute__ ((aligned (64)))
__attribute__ ((section (".data.KeyWrapBuf")));

static u8 AesWrappedKey[XSECURE_AES_CMK_SIZE_IN_BYTES]  __attribute__ ((aligned (64)))
__attribute__ ((section (".data.AesWrappedKey")));

static u8 Signature[XSECURE_ECC_SIGN_TOTAL_LEN] __attribute__ ((section (".data.Signature")));

/* shared memory allocation */
static u8 SharedMem[XSECURE_SHARED_MEM_SIZE] __attribute__((aligned(64U)))
										__attribute__ ((section (".data.SharedMem")));

/* Exponent of Public key */
static const u32 PublicExp = 0x1000100;

/*****************************************************************************/
/**
* @brief	Main function to call the XSecure_KeyUnwrap to unwrap
* 		the wrapped key.
*
* @return
*		- XST_SUCCESS - On successful key unwrap
*		- Errorcode - On failure
*
******************************************************************************/
int main(void)
{
	int Status = XST_FAILURE;
	XMailbox MailboxInstance;
	XSecure_ClientInstance SecureClientInstance;
	XOcp_ClientInstance OcpClientInstance;
	u32 Index = 0U;

#ifdef XSECURE_CACHE_DISABLE
	Xil_DCacheDisable();
#endif

	Status = XMailbox_Initialize(&MailboxInstance, 0U);
	if (Status != XST_SUCCESS) {
		xil_printf("Mailbox initialize failed:%08x \r\n", Status);
		goto END;
	}

	Status = XSecure_ClientInit(&SecureClientInstance, &MailboxInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("Secure client initialize failed:%08x \r\n", Status);
		goto END;
	}

	Status = XOcp_ClientInit(&OcpClientInstance, &MailboxInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("OCP client initialize failed:%08x \r\n", Status);
		goto END;
	}

	Status = Xil_ConvertStringToHexBE(XSECURE_AES_CMK_WRAP_DATA, AesWrappedKey, XSECURE_AES_CMK_SIZE_IN_BYTES * 8U);
	if (Status != XST_SUCCESS) {
		goto END;
	}

	Status = XOcp_ClientAttestWithKeyWrapDevAk(&OcpClientInstance, (u64)(UINTPTR)&KeyWrapBuf, sizeof(KeyWrapBuf),
			 XSECURE_KEY_WRAP_PUB_KEY_OFFSET, (u64)(UINTPTR)Signature);
	if (Status != XST_SUCCESS) {
		xil_printf("\r\n Attest and get RSA public key failed with status:%02x",Status);
		goto END;
	}

	xil_printf("\r\n Attestation signature using keywrap DevAK:");
	xil_printf("\r\n Signature R:");
	XSecure_PrintData(Signature, XSECURE_ECC_SIGN_TOTAL_LEN/2);
	xil_printf("\r\n Signature S:");
	XSecure_PrintData(Signature, XSECURE_ECC_SIGN_TOTAL_LEN/2);

	xil_printf("\r\n RSA Public key:");
	xil_printf("\r\n Modulus:");
	for (Index = 0U; Index < XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES; Index++) {
		xil_printf("%02x", KeyWrapBuf.PubKey[Index]);
	}

	xil_printf("\r\n Exponent:");
	for (; Index < (XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES + XSECURE_RSA_PUB_EXP_SIZE); Index++) {
		xil_printf("%02x", KeyWrapBuf.PubKey[Index]);
	}

	Status = XSecure_RsaPublicEncKat(&SecureClientInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("RSA public encrypt KAT failed\n\r");
	}


	/* Set shared memory */
	Status = XMailbox_SetSharedMem(&MailboxInstance, (u64)(UINTPTR)(SharedMem), XSECURE_SHARED_MEM_SIZE);
	if (Status != XST_SUCCESS) {
		xil_printf("\r\n shared memory initialization failed");
		goto END;
	}

	Status = Xil_SMemCpy(KeyWrapBuf.PubKey + XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES, 4U, &PublicExp, 4U, 4U);
	if (Status != XST_SUCCESS) {
		goto END;
	}

	/* Wrap ephemeral key with RSA public key */
	Status = XSecure_RsaPublicEncrypt(&SecureClientInstance, (UINTPTR)KeyWrapBuf.PubKey, (UINTPTR)RsaEncodedData,
			XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES, (UINTPTR)WrappedKey);
	if(XST_SUCCESS != Status)	{
		xil_printf("\r\nFailed at RSA signature encryption\n\r");
		goto END;
	}

	xil_printf("\r\n Wrapped Key:");
	XSecure_PrintData(WrappedKey, XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES);

	Status = Xil_SMemCpy(&WrappedKey[XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES], XSECURE_AES_CMK_SIZE_IN_BYTES, AesWrappedKey, XSECURE_AES_CMK_SIZE_IN_BYTES,
				XSECURE_AES_CMK_SIZE_IN_BYTES);
	if (Status != XST_SUCCESS) {
		goto END;
	}

	/* Update key store size and bit map */
	Xil_Out32((XSECURE_KEY_SLOT_ADDR + XSECURE_SHARED_KEY_STORE_SIZE_OFFSET), 32U);

	KeyWrapData.KeyWrapAddr = (u64)(UINTPTR)WrappedKey;
	KeyWrapData.TotalWrappedKeySize = XSECURE_RSA_KEY_GEN_SIZE_IN_BYTES + XSECURE_AES_CMK_SIZE_IN_BYTES;
	KeyWrapData.KeyMetaData.KeyOp = XSECURE_ENC_OP;
	KeyWrapData.KeyMetaData.AesKeySize = XSECURE_AES_CMK_SIZE_IN_BYTES;
	Status = XSecure_KeyUnwrap(&SecureClientInstance, &KeyWrapData);
END:
	if (Status != XST_SUCCESS) {
		xil_printf("\r\n RSA key unwrap example failed");
	} else {
		xil_printf("\r\n Successfully ran key unwrap example");
	}

	return Status;
}

/****************************************************************************/
/**
* @brief   This function prints the given data on the console
*
* @param   Data - Pointer to any given data buffer
*
* @param   Size - Size of the given buffer
*
****************************************************************************/
static void XSecure_PrintData(const u8 *Data, u32 Size)
{
	u32 Index;

	for (Index = 0U; Index < Size; Index++) {
		xil_printf("%02x", Data[Index]);
	}
	xil_printf("\r\n");
}

/** @} */
