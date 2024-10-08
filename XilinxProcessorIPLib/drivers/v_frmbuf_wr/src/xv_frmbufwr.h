// ==============================================================
// Copyright (c) 1986 - 2023 Xilinx Inc. All rights reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// SPDX-License-Identifier: MIT
// ==============================================================

#ifndef XV_FRMBUFWR_H
#define XV_FRMBUFWR_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xv_frmbufwr_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#else

/**
* This typedef contains configuration information for the frame buffer write core
* Each core instance should have a configuration structure associated.
*/
typedef struct {
#ifndef SDT
  u16 DeviceId;             /**< Unique ID of device */
#else
  char *Name;		    /**< Unique Name of device */
#endif
  UINTPTR BaseAddress;      /**< The base address of the core instance. */
  u16 PixPerClk;            /**< Samples Per Clock */
  u16 MaxWidth;             /**< Maximum columns supported by core instance */
  u16 MaxHeight;            /**< Maximum rows supported by core instance */
  u16 MaxDataWidth;         /**< Maximum Data width of each channel */
  u16 AXIMMDataWidth;       /**< AXI-MM data width */
  u16 AXIMMAddrWidth;       /**< AXI-MM address width */
  u16 RGBX8En;              /**< RGBX8      support */
  u16 YUVX8En;              /**< YUVX8      support */
  u16 YUYV8En;              /**< YUYV8      support */
  u16 RGBA8En;              /**< RGBA8      support */
  u16 YUVA8En;              /**< YUVA8      support */
  u16 BGRA8En;              /**< YUVA8      support */
  u16 RGBX10En;             /**< RGBX10     support */
  u16 YUVX10En;             /**< YUVX10     support */
  u16 Y_UV8En;              /**< Y_UV8      support */
  u16 Y_UV8_420En;          /**< Y_UV8_420  support */
  u16 RGB8En;               /**< RGB8       support */
  u16 YUV8En;               /**< YUV8       support */
  u16 Y_UV10En;             /**< Y_UV10     support */
  u16 Y_UV10_420En;         /**< Y_UV10_420 support */
  u16 Y8En;                 /**< Y8         support */
  u16 Y10En;                /**< Y10        support */
  u16 BGRX8En;              /**< BGRX8      support */
  u16 UYVY8En;              /**< UYVY8      support */
  u16 BGR8En;               /**< BGR8       support */
  u16 RGBX12En;             /**< RGBX12     support */
  u16 RGB16En;              /**< RGB16      support */
  u16 YUVX12En;             /**< YUVX12     support */
  u16 Y_UV12En;             /**< Y_UV12     support */
  u16 Y_UV12_420En;         /**< Y_UV12_420 support */
  u16 Y12En;                /**< Y12      support */
  u16 YUV16En;              /**< YUV16      support */
  u16 Y_UV16En;             /**< Y_UV16     support */
  u16 Y_UV16_420En;         /**< Y_UV16_420 support */
  u16 Y16En;                /**< Y16      support */
  u16 Y_U_V8En;             /**< Y_U_V8   support */
  u16 Y_U_V10En;            /**< Y_U_V10   support */
  u16 Y_U_V8_420En;         /**< Y_U_V8_420   support */
  u16 Y_U_V12En;            /**< Y_U_V12   support */
  u16 Interlaced;           /**< Interlaced support */
  u16 IsTileFormat;           /**< Interlaced support */
#ifdef SDT
  u16 IntrId; 		    /**< Interrupt ID */
  UINTPTR IntrParent; 	    /**< Bit[0] Interrupt parent type Bit[64/32:1] Parent base address */
#endif
} XV_frmbufwr_Config;
#endif

/**
* Driver instance data. An instance must be allocated for each core in use.
*/
typedef struct {
  XV_frmbufwr_Config Config;   /**< Hardware Configuration */
  u32 IsReady;                  /**< Device is initialized and ready */
} XV_frmbufwr;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XV_frmbufwr_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XV_frmbufwr_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XV_frmbufwr_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XV_frmbufwr_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifndef SDT
int XV_frmbufwr_Initialize(XV_frmbufwr *InstancePtr, u16 DeviceId);
XV_frmbufwr_Config* XV_frmbufwr_LookupConfig(u16 DeviceId);
#else
int XV_frmbufwr_Initialize(XV_frmbufwr *InstancePtr, UINTPTR BaseAddress);
XV_frmbufwr_Config* XV_frmbufwr_LookupConfig(UINTPTR BaseAddress);
#endif
int XV_frmbufwr_CfgInitialize(XV_frmbufwr *InstancePtr,
                               XV_frmbufwr_Config *ConfigPtr,
                               UINTPTR EffectiveAddr);
#else
int XV_frmbufwr_Initialize(XV_frmbufwr *InstancePtr, const char* InstanceName);
int XV_frmbufwr_Release(XV_frmbufwr *InstancePtr);
#endif

void XV_frmbufwr_Start(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_IsDone(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_IsIdle(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_IsReady(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_EnableAutoRestart(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_DisableAutoRestart(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_SetFlushbit(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_Get_FlushDone(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_width(XV_frmbufwr *InstancePtr, u32 Data);
u32 XV_frmbufwr_Get_HwReg_width(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_height(XV_frmbufwr *InstancePtr, u32 Data);
u32 XV_frmbufwr_Get_HwReg_height(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_stride(XV_frmbufwr *InstancePtr, u32 Data);
u32 XV_frmbufwr_Get_HwReg_stride(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_video_format(XV_frmbufwr *InstancePtr, u32 Data);
u32 XV_frmbufwr_Get_HwReg_video_format(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_frm_buffer_V(XV_frmbufwr *InstancePtr, u64 Data);
u64 XV_frmbufwr_Get_HwReg_frm_buffer_V(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_frm_buffer2_V(XV_frmbufwr *InstancePtr, u64 Data);
u64 XV_frmbufwr_Get_HwReg_frm_buffer2_V(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_Set_HwReg_frm_buffer3_V(XV_frmbufwr *InstancePtr, u64 Data);
u64 XV_frmbufwr_Get_HwReg_frm_buffer3_V(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_Get_HwReg_field_id(XV_frmbufwr *InstancePtr);

void XV_frmbufwr_InterruptGlobalEnable(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_InterruptGlobalDisable(XV_frmbufwr *InstancePtr);
void XV_frmbufwr_InterruptEnable(XV_frmbufwr *InstancePtr, u32 Mask);
void XV_frmbufwr_InterruptDisable(XV_frmbufwr *InstancePtr, u32 Mask);
void XV_frmbufwr_InterruptClear(XV_frmbufwr *InstancePtr, u32 Mask);
u32 XV_frmbufwr_InterruptGetEnabled(XV_frmbufwr *InstancePtr);
u32 XV_frmbufwr_InterruptGetStatus(XV_frmbufwr *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
