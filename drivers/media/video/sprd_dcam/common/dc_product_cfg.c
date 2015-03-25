/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "dc_product_cfg.h"
#include "jpeg_exif_header_k.h"
typedef int8_t int8;

#define SCI_TRUE 1
#define SCI_FALSE 0
LOCAL uint32 _DC_GetProductCfgInfo(uint32 param);
LOCAL uint32 _DC_GetExifPrimaryPriDescInfo(uint32 param);
LOCAL uint32 _DC_GetExifSpecUserInfo(uint32 param);

#define DC_MEM_SIZE (3*1024*1024)
#define DV_MEM_SIZE (3*1024*1024)
#define VT_MEM_SIZE (3*1024*1024)
LOCAL EXIF_PRI_DESC_T s_dc_exif_pri_desc_info = {
	{
	 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}, "Default Date",	/*Date */
	"ImageDescription",	/*ImageDescription */
	"Maker",		/*Make */
	"Model",		/*Model */
	"Software Version v0.0.0",	/*Software */
	"Artist",		/*Artist */
	"CopyRight"		/*Copyright */
};

LOCAL uint8 exif_user_comments[20] = {
	"User Comments"
};

LOCAL EXIF_SPEC_USER_T s_dc_exif_spec_user_info = {
	0
};

LOCAL DC_PRODUCT_CFG_T s_dc_product_cfg_info = {
	SCI_TRUE, SCI_TRUE, SCI_FALSE, SCI_FALSE, DC_MEM_SIZE,
	DV_MEM_SIZE, VT_MEM_SIZE, DC_MAX_VIDEO_MODE_CIF,
	DC_PRODUCT_FLASH_TYPE_DISABLE
};

LOCAL DC_PRODUCT_CFG_FUNC_TAB_T s_dc_product_cfg_fun = {
	_DC_GetProductCfgInfo, _DC_GetExifPrimaryPriDescInfo,
	_DC_GetExifSpecUserInfo
};

LOCAL uint32 _DC_GetProductCfgInfo(uint32 param)
{
	return (uint32) & s_dc_product_cfg_info;
}

LOCAL uint32 _DC_GetExifPrimaryPriDescInfo(uint32 param)
{
	EXIF_PRI_DESC_T *exif_ptr = &s_dc_exif_pri_desc_info;

#ifdef KERNEL_TIME
	SCI_DATE_T cur_date = {
		0
	};
	SCI_TIME_T cur_time = {
		0
	};
	*(uint32 *) & s_dc_exif_pri_desc_info.valid = (uint32) 0x7F;
	TM_GetSysDate(&cur_date);
	TM_GetSysTime(&cur_time);
	sprintf((int8 *) exif_ptr->DateTime, "%04d:%02d:%02d %02d:%02d:%02d",
		cur_date.year, cur_date.mon, cur_date.mday, cur_time.hour,
		cur_time.min, cur_time.sec);

#endif /*  */
	/*    sprintf((int8*)s_dc_exif_pri_desc_info.Copyright, "%s", "CopyRight"); */
	/*    sprintf((int8*)s_dc_exif_pri_desc_info.ImageDescription, "%s", "ImageDescription"); */
	sprintf((int8 *) s_dc_exif_pri_desc_info.Make, "%s", "Spreadtrum");
	sprintf((int8 *) s_dc_exif_pri_desc_info.Model, "%s", "SP8810ga");

	/*   sprintf((int8*)s_dc_exif_pri_desc_info.Software, "%s", "Test Version v0.0.0.1"); */
	/*/   sprintf((int8*)s_dc_exif_pri_desc_info.Artist, "%s", "Artist"); */
	sprintf((int8 *) s_dc_exif_pri_desc_info.Copyright, "%s",
		"CopyRight, Spreadtrum, 2012");
	return (uint32) exif_ptr;
}

LOCAL uint32 _DC_GetExifSpecUserInfo(uint32 param)
{
	EXIF_SPEC_USER_T *exif_ptr = &s_dc_exif_spec_user_info;

	/*
	   s_dc_exif_spec_user_info.valid.MakerNote = 0;
	   s_dc_exif_spec_user_info.valid.UserComment = 0;
	   s_dc_exif_spec_user_info.UserComment.count = 1;
	   s_dc_exif_spec_user_info.UserComment.count = strlen((char*)exif_user_comments);
	   s_dc_exif_spec_user_info.UserComment.ptr = (void*)exif_user_comments;
	   s_dc_exif_spec_user_info.UserComment.type = EXIF_ASCII;
	   s_dc_exif_spec_user_info.UserComment.size = 1;
	 */
	DC_PRO_CFG_PRINT
	    ("DC_PRODUCT: _DC_GetExifSpecUserInfo, valid.UserComment %d \n",
	     s_dc_exif_spec_user_info.valid.UserComment);
	return (uint32) exif_ptr;
}

DC_PRODUCT_CFG_FUNC_TAB_T_PTR DC_GetDcProductCfgFun(void)
{
	return &s_dc_product_cfg_fun;
}
