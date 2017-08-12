/*
 *
 * FocalTech fts TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: Focaltech_Gestrue.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-10-26
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define  KEY_GESTURE_U		KEY_U
#define  KEY_GESTURE_UP		KEY_UP
#define  KEY_GESTURE_DOWN		KEY_DOWN
#define  KEY_GESTURE_LEFT		KEY_LEFT 
#define  KEY_GESTURE_RIGHT		KEY_RIGHT
#define  KEY_GESTURE_O		KEY_O
#define  KEY_GESTURE_E		KEY_E
#define  KEY_GESTURE_M		KEY_M 
#define  KEY_GESTURE_L		KEY_L
#define  KEY_GESTURE_W		KEY_W
#define  KEY_GESTURE_S		KEY_S 
#define  KEY_GESTURE_V		KEY_V
#define  KEY_GESTURE_Z		KEY_Z
#define KEY_GESTURE_C		KEY_C //ADD

#define GESTURE_LEFT		0x20
#define GESTURE_RIGHT		0x21
#define GESTURE_UP		    0x22
#define GESTURE_DOWN		0x23
#define GESTURE_DOUBLECLICK	0x24
#define GESTURE_O		    0x30
#define GESTURE_W		    0x31
#define GESTURE_M		    0x32
#define GESTURE_E		    0x33
#define GESTURE_C		    0x34  //add, reserved.
#define GESTURE_L		    0x44
#define GESTURE_S		    0x46
#define GESTURE_V		    0x54
#define GESTURE_Z		    0x65 //modify by liuyang.
#define FTS_GESTRUE_POINTS 255
#define FTS_GESTRUE_POINTS_ONETIME  62
#define FTS_GESTRUE_POINTS_HEADER 8
#define FTS_GESTURE_OUTPUT_ADRESS 0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 4

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
short pointnum = 0;
unsigned short coordinate_x[150] = {0};
unsigned short coordinate_y[150] = {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
//Start:Reqxxx,liuyang3.wt,ADD,20160520, add gesture node.
#define GESTURE_NODE "goodix_gesture"
#define GESTURE_NUM  (7)
//begin:Reqxxx,anhengxuan,20161121
extern struct ap3426_data *g_ap3426_data;
//end:Reqxxx,anhengxuan,20161121
int fts_gesture_enabled = 0;    /* module switch */
static int gestures_flag; /* gesture flag, every bit stands for a gesture, [7]- all switch [6:0] = w, s, e, c, z, v, double click.  */
static u8 gestures_data[GESTURE_NUM] = {GESTURE_W, GESTURE_S, GESTURE_E, GESTURE_C, GESTURE_Z, GESTURE_V, GESTURE_DOUBLECLICK}; /*  ASUS surport gesture type,*/ 
static const int  gesture_key_codes[GESTURE_NUM] = {KEY_W, KEY_S, KEY_E, KEY_C, KEY_Z, KEY_V, KEY_POWER};

static ssize_t fts_gesture_switch_read(struct file *file, char __user * page, size_t size, loff_t * ppos);
static ssize_t fts_gesture_switch_write(struct file *filp, const char __user * buff, size_t len, loff_t * off);
static const struct file_operations fts_gestures_fops = {
	.owner = THIS_MODULE,
	.read = fts_gesture_switch_read,
	.write = fts_gesture_switch_write,
};
static ssize_t fts_gesture_switch_read(struct file *file, char __user * page, size_t size, loff_t * ppos)
{
	s32 ret = -1;
	char buf[20] = {0};

	snprintf(buf, 20, "%u\n", gestures_flag);
	ret = simple_read_from_buffer(page, size, ppos, buf, strlen(buf));
	return ret;
}

static ssize_t fts_gesture_switch_write(struct file *filp, const char __user * buff, size_t len, loff_t * off)
{
	int ret = 0;
	char  temp[32] = {0};
        //unsigned int mask =0;

	ret = copy_from_user(temp, buff, len );
	if (ret) {
		pr_err("kstrtou8_from_user failed.");
		return -EPERM;
	}
	ret = kstrtouint(temp, 0, &gestures_flag);
        //mask = gestures_flag & (~((~0) << (GESTURE_NUM + 1))); 
	if (gestures_flag >> (GESTURE_NUM + 1)){
		pr_err("error,gestures_flag = 0x%x\n", gestures_flag);
		return len;	
	}
	pr_info("temp = %s,gestures_flag = %d, ret:%d \n", temp, gestures_flag, ret);
	//if ((gestures_flag - 0x80) > 0 ){
	//begin anhengxuan.wt 2016.8.8 for bug203102  dclik don't wake up screen
	if ((gestures_flag - (0x1 << GESTURE_NUM)) > 0 ||(gestures_flag==0x01) ){
	//end anhengxuan.wt 2016.8.8 for bug203102  dclik don't wake up screen
	//if (mask - (0x1 << GESTURE_NUM)){  
		fts_gesture_enabled = 1;
		pr_info("fts_gesture_enabled = %d\n", fts_gesture_enabled);
	}else {
		fts_gesture_enabled = 0;
		pr_info("fts_gesture_enabled = %d\n", fts_gesture_enabled);
	}

	return len;
}
int fts_gesture_init_node(void)
{

	struct proc_dir_entry *proc_entry = NULL;
	//mutex_init(&gesture_data_mutex);
	memset(&gestures_flag, 0, sizeof(gestures_flag));
	//memset(gestures_data, 0, sizeof(gestures_data)/sizeof(unsigned char));

	proc_entry = proc_create(GESTURE_NODE, 0666, NULL, &fts_gestures_fops);
	if (proc_entry == NULL) {
		pr_err("CAN't create proc entry /proc/%s.", GESTURE_NODE);
		return -1;
	} else {
		pr_info("Created proc entry /proc/%s success.", GESTURE_NODE);
	}

	return 0;
}

void fts_gesture_deinit_node(void)
{
	remove_proc_entry(GESTURE_NODE, NULL);
}

//End:Reqxxx,liuyang3.wt,ADD,20160520, add gesture node.

/*******************************************************************************
* Name: fts_Gesture_init
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_Gesture_init(struct input_dev *input_dev)
{
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S); 
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z); 
	return 0;
}

/*******************************************************************************
* Name: fts_check_gesture
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int fts_check_gesture(struct input_dev *input_dev,int gesture_id)
{
	int i =0;
	int enable=0;
	pr_info("gesture_id = 0x%x", gesture_id);
	  //add by anhengxuan 20161121
      if(g_ap3426_data!=NULL){
           enable=1;
	  ap3426_enable_ps(g_ap3426_data, enable);
	  msleep(5);
	  if(ap3426_enable_ps(g_ap3426_data, enable))	     
           {
              pr_info("enable ps failed!");
	          return  -EFAULT;
           } 
      	}
     // end by anhengxuan 20161121      
	for(i = 0; i < GESTURE_NUM; i++){
		if (gesture_id == gestures_data[i]){
			if (gestures_flag & (0x01 << (GESTURE_NUM -i -1))&&(1==g_ap3426_data->last_ps))
			{
				pr_info("Wakeup by gesture(%x), light up the screen!", gesture_id);
                	 	input_report_key(input_dev, gesture_key_codes[i], 1);
               	 	input_sync(input_dev);
               	 	input_report_key(input_dev, gesture_key_codes[i], 0);
               	 	input_sync(input_dev);                
				break;
			}
		}
	}
/*
	switch(gesture_id)
	{
	        case GESTURE_LEFT:
	                input_report_key(input_dev, KEY_GESTURE_LEFT, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_LEFT, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_RIGHT:
	                input_report_key(input_dev, KEY_GESTURE_RIGHT, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_RIGHT, 0);
	                input_sync(input_dev);
			    break;
	        case GESTURE_UP:
	                input_report_key(input_dev, KEY_GESTURE_UP, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_UP, 0);
	                input_sync(input_dev);                                  
	                break;
	        case GESTURE_DOWN:
	                input_report_key(input_dev, KEY_GESTURE_DOWN, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_DOWN, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_DOUBLECLICK:
	                input_report_key(input_dev, KEY_GESTURE_U, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_U, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_O:
	                input_report_key(input_dev, KEY_GESTURE_O, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_O, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_W:
	                input_report_key(input_dev, KEY_GESTURE_W, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_W, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_M:
	                input_report_key(input_dev, KEY_GESTURE_M, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_M, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_E:
	                input_report_key(input_dev, KEY_GESTURE_E, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_E, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_L:
	                input_report_key(input_dev, KEY_GESTURE_L, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_L, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_S:
	                input_report_key(input_dev, KEY_GESTURE_S, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_S, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_V:
	                input_report_key(input_dev, KEY_GESTURE_V, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_V, 0);
	                input_sync(input_dev);
	                break;
	        case GESTURE_Z:
	                input_report_key(input_dev, KEY_GESTURE_Z, 1);
	                input_sync(input_dev);
	                input_report_key(input_dev, KEY_GESTURE_Z, 0);
	                input_sync(input_dev);
	                break;
	        default:
	                break;
	}
*/
          return 0;
}

 /************************************************************************
* Name: fts_read_Gestruedata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
int fts_read_Gestruedata(void)
{
	unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
	int ret = -1;
	int i = 0;
	int gestrue_id = 0;

	buf[0] = 0xd3;
	pointnum = 0;

	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);

	if (ret < 0)
	{
		printk( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	 if (fts_updateinfo_curr.CHIP_ID == 0x54 || fts_updateinfo_curr.CHIP_ID == 0x58 || fts_updateinfo_curr.CHIP_ID == 0x86 || fts_updateinfo_curr.CHIP_ID == 0x87)
	 {
		 gestrue_id = buf[0];
		 pointnum = (short)(buf[1]) & 0xff;
		 buf[0] = 0xd3;

		 if((pointnum * 4 + 2)<255)
		 {
		    	 ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 2));
		 }
		 else
		 {
		        ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 255);
		        ret = fts_i2c_read(fts_i2c_client, buf, 0, buf+255, (pointnum * 4 + 2) -255);
		 }
		 if (ret < 0)
		 {
		       printk( "%s read touchdata failed.\n", __func__);
		       return ret;
		 }

		 fts_check_gesture(fts_input_dev,gestrue_id);
		 for(i = 0;i < pointnum;i++)
		 {
		    	coordinate_x[i] =  (((s16) buf[2 + (4 * i)]) & 0x0F) <<
		        	8 | (((s16) buf[3 + (4 * i)])& 0xFF);
		    	coordinate_y[i] = (((s16) buf[4 + (4 * i)]) & 0x0F) <<
		        	8 | (((s16) buf[5 + (4 * i)]) & 0xFF);
		 }
		 return -1;
	}
	else
	{
/*	
		if (0x24 == buf[0])
		{
			gestrue_id = 0x24;
			fts_check_gesture(fts_input_dev,gestrue_id);
			printk( "%d check_gesture gestrue_id.\n", gestrue_id);
			return -1;
		}

		pointnum = (short)(buf[1]) & 0xff;
		buf[0] = 0xd3;
		if((pointnum * 4 + 8)<255)
		{
			ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, (pointnum * 4 + 8));
		}
		else
		{
			ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 255);
			ret = fts_i2c_read(fts_i2c_client, buf, 0, buf+255, (pointnum * 4 + 8) -255);
		}
		if (ret < 0)
		{
			printk( "%s read touchdata failed.\n", __func__);
			return ret;
		}

		gestrue_id = fetch_object_sample(buf, pointnum);
		fts_check_gesture(fts_input_dev,gestrue_id);
		printk( "%d read gestrue_id.\n", gestrue_id);

		for(i = 0;i < pointnum;i++)
		{
		    coordinate_x[i] =  (((s16) buf[0 + (4 * i)]) & 0x0F) <<
		        8 | (((s16) buf[1 + (4 * i)])& 0xFF);
		    coordinate_y[i] = (((s16) buf[2 + (4 * i)]) & 0x0F) <<
		        8 | (((s16) buf[3 + (4 * i)]) & 0xFF);
		}
*/		
		return -1;
	
	}
}