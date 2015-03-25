/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
 *
 * File Name          : mxc622x.c
 * Description        : MXC622X accelerometer sensor API
 *
 *******************************************************************************
 *
 * 思路:
 * 		
 * 流程:
 *		enable后,启动计时器,计时中断到时执行mxc622x_acc_input_work_func()
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *

 ******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
#include <linux/cdc_com.h>
#endif

#include <linux/i2c/mxc622x.h>

// Debug Macro
#undef MXC622X_DEBUG
#ifdef MXC622X_DEBUG
#define GSENSOR_DBG(format, ...)	\
		printk(KERN_INFO "MXC622X " format, ## __VA_ARGS__)
#else
#define GSENSOR_DBG(format, ...)
#endif

#undef MXC622X_INFO
#ifdef MXC622X_INFO
#define GSENSOR_INFO(format, ...)	\
		printk(KERN_INFO "MXC622X " format, ## __VA_ARGS__)
#else
#define GSENSOR_INFO(format, ...)
#endif

// Alex.shi 谨记确认好下面 accele_gpio_scl/accele_gpio_sda 两个gpio口变量,并在pinmap中设置好gpio口  
#define SW_SIMULATE_I2C		0			//if use GPIO simulate I2C, define 1, 

// Check if we expect that this device is enabled when opened
#undef MXC622X_ACC_OPEN_ENABLE

#undef I2C_BUS_NUM_STATIC_ALLOC

#define	G_MAX				16000	/** Maximum polled-device-reported g value */
#define WHOAMI_MXC622X_ACC	0x25	/*	Expctd content for WAI	*/
#define WHOAMI_MXC6255XC_ACC	0x05	/*	Expctd content for WAI	*/


/*	CONTROL REGISTERS	*/
#define WHO_AM_I			0x08	/*	WhoAmI register		*/

#define	FUZZ				32
#define	FLAT				32
#define	I2C_RETRY_DELAY		5
#define	I2C_RETRIES			5
#define	I2C_AUTO_INCREMENT	0x80

/* RESUME STATE INDICES */
#define	RESUME_ENTRIES		20
#define DEVICE_INFO			"Memsic, MXC622X"
#define DEVICE_INFO_LEN		32
/* end RESUME STATE INDICES */

#define	MAX_INTERVAL	50

#ifdef I2C_BUS_NUM_STATIC_ALLOC
#define I2C_STATIC_BUS_NUM		(2)	// Need to be modified according to actual setting

#ifdef __KERNEL__
static struct mxc622x_acc_platform_data mxc622x_plat_data = {
    .poll_interval = 20,
    .min_interval = 10,
};
#endif

static struct i2c_board_info  mxc622x_i2c_boardinfo = {
	I2C_BOARD_INFO(MXC622X_ACC_I2C_NAME, MXC622X_ACC_I2C_ADDR),
	#ifdef __KERNEL__
	.platform_data = &mxc622x_plat_data
	#endif
};
#endif

struct mxc622x_acc_data {
	struct i2c_client	*client;
	struct mxc622x_acc_platform_data	*pdata;

	struct mutex		lock;
	struct delayed_work	input_work;

	struct input_dev	*input_dev;

	int			hw_initialized;
	/* hw_working=-1 means not tested yet */
	int			hw_working;
	atomic_t	enabled;
	int			on_before_suspend;

	u8			resume_state[RESUME_ENTRIES];

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};

/*
 * Because misc devices can not carry a pointer from driver register to
 * open, we keep this global.  This limits the driver to a single instance.
 */
struct mxc622x_acc_data	*mxc622x_acc_misc_data;
struct i2c_client		*mxc622x_i2c_client;
//static	int	s_install_dir = ACC_DIR_INSTAL;
#if SW_SIMULATE_I2C
#define MXC622X_I2C_SLAVE_ADDR    (MXC622X_ACC_I2C_ADDR*2)
#define MXC622X_REG_DATAX0        0x00

#define MXC622X_AXIS_X          0
#define MXC622X_AXIS_Y          1
#define MXC622X_AXIS_Z          2
#define MXC622X_AXES_NUM        3
#define MXC622X_DATA_LEN        2

// 定义用来模拟的GPIO口
#define  accele_gpio_scl  (79)
#define  accele_gpio_sda  (80)

#define ACC_NDELAY(x)  ndelay((x)*100)

//start setup time, >= 0.6 us, 1.2us
#define	DELAY_LOOP_START_SETUP  12
//start hold time, >= 0.6 us, 1.2us
#define	DELAY_LOOP_START_HOLD  12
//LOW period of SCL, >= 1.3 us, 2.6us
#define	DELAY_LOOP_SCL_LOW  26
//HIGH period of SCL, >= 0.6 us, 1.2us
#define	DELAY_LOOP_SCL_HIGH   12
//data hold time, <= 0.9 us, 1.8us
#define	DELAY_LOOP_DATA_HOLD   18
//data setup time, >= 0.1 us, 2.6 - 1.2 = 1.4us
#define	DELAY_LOOP_DATA_SETUP  (DELAY_LOOP_SCL_LOW - DELAY_LOOP_DATA_HOLD)
//stop setup time, >= 0.6 us, 1.2us
#define	DELAY_LOOP_STOP_SETUP  DELAY_LOOP_START_SETUP
//bus free time between STOP and START, >= 1.3 us, 2.6us
#define	DELAY_LOOP_BUS_FREE  26


#define GPIO_OUT_ONE	1
#define GPIO_OUT_ZERO	0

#define ACC_SET_I2C_CLK_OUTPUT			//mt_set_gpio_dir(accele_gpio_scl,GPIO_DIR_OUT)

#define ACC_SET_I2C_CLK_INPUT			gpio_direction_input(accele_gpio_scl)
#define ACC_SET_I2C_CLK_HIGH			gpio_direction_output(accele_gpio_scl,1)
#define ACC_SET_I2C_CLK_LOW				gpio_direction_output(accele_gpio_scl,0)


#define ACC_SET_I2C_DATA_OUTPUT			//mt_set_gpio_dir(accele_gpio_sda,GPIO_DIR_OUT)

#define ACC_SET_I2C_DATA_INPUT			gpio_direction_input(accele_gpio_sda)
#define ACC_SET_I2C_DATA_HIGH			gpio_direction_output(accele_gpio_sda,GPIO_OUT_ONE)
#define ACC_SET_I2C_DATA_LOW			gpio_direction_output(accele_gpio_sda,GPIO_OUT_ZERO)
#define ACC_GET_I2C_DATA_BIT			gpio_get_value(accele_gpio_sda)


void  memsic_acc_i2c_start(void)	
{	
    ACC_SET_I2C_DATA_OUTPUT;	
    ACC_SET_I2C_CLK_OUTPUT; 	
    ACC_SET_I2C_DATA_HIGH;		
    ACC_SET_I2C_CLK_HIGH;		
    ACC_NDELAY(DELAY_LOOP_START_SETUP);	
    ACC_SET_I2C_DATA_LOW;	
    ACC_NDELAY(DELAY_LOOP_START_HOLD);	
    ACC_SET_I2C_CLK_LOW;	
}

void memsic_acc_i2c_write(int data)	
{	
	signed char i;	
	ACC_SET_I2C_DATA_OUTPUT;	
	ACC_SET_I2C_CLK_LOW;	
	for (i=7;i>=0;i--)	
	{	
		ACC_NDELAY(DELAY_LOOP_DATA_HOLD);	
		if ((data) & (1<<i))	
		{	
			ACC_SET_I2C_DATA_HIGH;	
		}	
		else	
		{	
			ACC_SET_I2C_DATA_LOW;	
		}	
		ACC_NDELAY(DELAY_LOOP_DATA_SETUP);		
		ACC_SET_I2C_CLK_HIGH;	
		ACC_NDELAY(DELAY_LOOP_SCL_HIGH);		
		ACC_SET_I2C_CLK_LOW;	
	}	
}

static int memsic_acc_i2c_read(char*data)
{	
	signed char i;	
	//int data=0;
	*data = 0;	
	ACC_SET_I2C_CLK_LOW;	
	ACC_SET_I2C_DATA_INPUT;	
	for (i=7;i>=0;i--)	
	{	
		ACC_NDELAY(DELAY_LOOP_SCL_LOW);		
		ACC_SET_I2C_CLK_HIGH;	
		ACC_NDELAY(DELAY_LOOP_SCL_HIGH);		
		if (ACC_GET_I2C_DATA_BIT)	
		{	
           (*data)|= (1<<i);	
		}	
		ACC_SET_I2C_CLK_LOW;	
	}	
	return *data;
}

int  memsic_acc_i2c_slave_ack(void)	
{	
    int ack;
	
	ACC_SET_I2C_CLK_LOW;	
	ACC_SET_I2C_DATA_INPUT;	
	ACC_NDELAY(DELAY_LOOP_SCL_LOW);		
	ACC_SET_I2C_CLK_HIGH;	
	ACC_NDELAY(DELAY_LOOP_SCL_HIGH);
	ack = ACC_GET_I2C_DATA_BIT;
	ACC_SET_I2C_CLK_LOW;
	
	return ack;
}

void memsic_acc_i2c_master_ack(void)
{	
    ACC_SET_I2C_CLK_LOW;	
    ACC_SET_I2C_DATA_OUTPUT;	
    ACC_NDELAY(DELAY_LOOP_DATA_HOLD);		
    ACC_SET_I2C_DATA_LOW;	
    ACC_NDELAY(DELAY_LOOP_DATA_SETUP);		
    ACC_SET_I2C_CLK_HIGH;	
    ACC_NDELAY(DELAY_LOOP_SCL_HIGH);		
    ACC_SET_I2C_CLK_LOW;	
}

void memsic_acc_i2c_master_nack(void)
{	
	ACC_SET_I2C_CLK_LOW;	
	ACC_SET_I2C_DATA_OUTPUT;	
	ACC_NDELAY(DELAY_LOOP_DATA_HOLD);
	ACC_SET_I2C_DATA_HIGH;	
	ACC_NDELAY(DELAY_LOOP_DATA_SETUP);	
	ACC_SET_I2C_CLK_HIGH;	
	ACC_NDELAY(DELAY_LOOP_SCL_HIGH);		
	ACC_SET_I2C_CLK_LOW;	
}

void memsic_acc_i2c_stop(void)
{
    ACC_SET_I2C_CLK_LOW;	
    ACC_SET_I2C_DATA_OUTPUT;	
    ACC_SET_I2C_DATA_LOW;	
    ACC_NDELAY(DELAY_LOOP_SCL_LOW); 	
    ACC_SET_I2C_CLK_HIGH;	
    ACC_NDELAY(DELAY_LOOP_STOP_SETUP);		
    ACC_SET_I2C_DATA_HIGH;	
}

static int acc_i2c_putbyte(signed char byte)
{	
	int  ack = 1;
	memsic_acc_i2c_write(byte);
	ack=memsic_acc_i2c_slave_ack();
	return ack;	
}

static signed char acc_i2c_getbyte(int LastByte)
{
	signed char data = 0;
	
	memsic_acc_i2c_read(&data);

	if(LastByte == 1){
		memsic_acc_i2c_master_nack();
	}
	else{
		memsic_acc_i2c_master_ack();
	}
	
	return data;
}

static void _acc_i2c_write_bytes(int slave_addr,signed char *data,int len)
{
	int i;
	
	gpio_direction_output(accele_gpio_scl,GPIO_OUT_ONE);
	gpio_direction_output(accele_gpio_sda,GPIO_OUT_ONE);	

	memsic_acc_i2c_start();	

	if(acc_i2c_putbyte(slave_addr)){
		printk("[ACCESS: mxc622x] %s put address no ack!\n", __FUNCTION__);
		memsic_acc_i2c_stop();
		return;
	}

	for(i=0;i<len;i++)
	{
		if(acc_i2c_putbyte(data[i])){
			printk("[ACCESS: mxc622x] %s put data no ack!\n", __FUNCTION__);
			memsic_acc_i2c_stop();
			return;
		}		
	}

	memsic_acc_i2c_stop();	
}

#if 0	// Alex.shi 预留一个读单个byte的接口
static int acc_i2c_read_byte(int slave_addr, signed char *cmd, int len)
{
	signed char data;
	
	gpio_request(accele_gpio_scl, "i2c-scl-mxc");
	gpio_request(accele_gpio_sda, "i2c-sda-mxc");
	
	gpio_direction_output(accele_gpio_scl,GPIO_OUT_ONE);
	gpio_direction_output(accele_gpio_sda,GPIO_OUT_ONE);
	
	_acc_i2c_write_bytes(slave_addr, cmd, len);
		
	memsic_acc_i2c_start();
	acc_i2c_putbyte(slave_addr+1);
	
	data = acc_i2c_getbyte(1); // only read one byte.
	memsic_acc_i2c_stop();
	
    gpio_free(accele_gpio_scl);
	gpio_free(accele_gpio_sda);
	
	return data;
}
#endif

static int acc_i2c_read_muti_bytes(int slave_addr, signed char *cmd, int cmdlen, signed char *data, int datalen)
{
	signed char tmp = 0;
	int i = 0;
	
	gpio_request(accele_gpio_scl, "i2c-scl-mxc");
	gpio_request(accele_gpio_sda, "i2c-sda-mxc");
	
	gpio_direction_output(accele_gpio_scl,GPIO_OUT_ONE);
	gpio_direction_output(accele_gpio_sda,GPIO_OUT_ONE);
	
	_acc_i2c_write_bytes(slave_addr, cmd, cmdlen);	
	
	memsic_acc_i2c_start();
	acc_i2c_putbyte(slave_addr+1);
	
	for(i = 0; i < datalen; i++)
	{
		if(i == (datalen - 1))
			tmp = acc_i2c_getbyte(1);
		else
			tmp = acc_i2c_getbyte(0);
		
		data[i] = tmp;
	}
	memsic_acc_i2c_stop();
	
    gpio_free(accele_gpio_scl);
	gpio_free(accele_gpio_sda);	
	return i;
}
static void acc_i2c_write_bytes(int slave_addr,signed char *data,int len)
{
	gpio_request(accele_gpio_scl, "i2c-scl-mxc");
	gpio_request(accele_gpio_sda, "i2c-sda-mxc");
    _acc_i2c_write_bytes(slave_addr, data, len);
    gpio_free(accele_gpio_scl);
	gpio_free(accele_gpio_sda);	
}

#endif

static int mxc622x_acc_i2c_read(struct mxc622x_acc_data *acc, u8 * buf, int len)
{
	int err;

#if SW_SIMULATE_I2C
    signed char addr = buf[0];
	signed char temp_buf[MXC622X_DATA_LEN] = {0};
	
	if((len != acc_i2c_read_muti_bytes(MXC622X_I2C_SLAVE_ADDR, &addr, 1, temp_buf, len)))
	{
		printk("error: read data error \n");
		err = -EIO;
	}
	else
		err = 0;

	buf[MXC622X_AXIS_X] = (temp_buf[0]);
	buf[MXC622X_AXIS_Y] = (temp_buf[1]);
	buf[MXC622X_AXIS_Z] = 0;
#else 
	int tries = 0;

	struct i2c_msg	msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buf,
		},
		{
			.addr = acc->client->addr,
			.flags = (acc->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}
#endif

	return err;
}

static int mxc622x_acc_i2c_write(struct mxc622x_acc_data *acc, u8 * buf, int len)
{
	int	err;
#if SW_SIMULATE_I2C 
	acc_i2c_write_bytes(MXC622X_I2C_SLAVE_ADDR, buf, len);
	err = 0;	// Alex.shi 没有错误检测
#else 
	int	tries = 0;
	struct i2c_msg msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len = len + 1, .buf = buf, 
		},
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}
#endif

	return err;
}


/********************************************************************
 * 功能：确认 I2C 硬件是否ok
 *
 * 参数:
 *			acc
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-5: IO出错
 *				
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_hw_init(struct mxc622x_acc_data *acc)
{
	int	err = -1;
	u8	buf[7];

	GSENSOR_DBG("%s: hw init start\n", MXC622X_ACC_DEV_NAME);

	buf[0] = WHO_AM_I;
	err = mxc622x_acc_i2c_read(acc, buf, 1);
	if (err < 0)
		goto error_firstread;
	else
		acc->hw_working = 1;
	if (((buf[0] & 0x3F) != WHOAMI_MXC622X_ACC) &&((buf[0] & 0x3F) != WHOAMI_MXC6255XC_ACC))  {
		err = -1; /* choose the right coded error */
		goto error_unknown_device;
	}

	acc->hw_initialized = 1;
	GSENSOR_DBG("%s: hw init done\n", MXC622X_ACC_DEV_NAME);
	return 0;

error_firstread:
	acc->hw_working = 0;
	dev_warn(&acc->client->dev, "Error reading WHO_AM_I: is device "
			"available/working?\n");
	goto error1;
error_unknown_device:
	dev_err(&acc->client->dev,
			"device unknown. Expected: 0x%x,"
			" Replies: 0x%x\n", WHOAMI_MXC622X_ACC, buf[0]);
error1:
	acc->hw_initialized = 0;
	dev_err(&acc->client->dev, "hw init error 0x%x,0x%x: %d\n", buf[0], buf[1], err);
	return err;
}

/********************************************************************
 * 功能：进入休眠模式
 *
 * 参数:
 *			acc
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-5: IO出错
 *				
 * 说明：	
 ********************************************************************/
static void mxc622x_acc_device_power_off(struct mxc622x_acc_data *acc)
{
	int	err;
	u8	buf[2] = { MXC622X_REG_CTRL, MXC622X_CTRL_PWRDN };

	err = mxc622x_acc_i2c_write(acc, buf, 1);

	if (err < 0)
		dev_err(&acc->client->dev, "soft power off failed: %d\n", err);
}

/********************************************************************
 * 功能：进入工作模式,并判断i2c是否正常
 *
 * 参数:
 *			acc
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-5: IO出错
 *				
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_device_power_on(struct mxc622x_acc_data *acc)
{
	int	err = -1;
	u8	buf[2] = {MXC622X_REG_CTRL, MXC622X_CTRL_PWRON};

	err = mxc622x_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power on failed: %d\n", err);

	if (!acc->hw_initialized) {
		err = mxc622x_acc_hw_init(acc);
		if (acc->hw_working == 1 && err < 0) {
			mxc622x_acc_device_power_off(acc);
			return err;
		}
	}

	return 0;
}

static int mxc622x_acc_register_read(struct mxc622x_acc_data *acc, u8 *buf,
		u8 reg_address)
{
	int	err = -1;

	buf[0] = (reg_address);
	err = mxc622x_acc_i2c_read(acc, buf, 1);
	return err;
}

#if 0
static int mxc622x_acc_register_write(struct mxc622x_acc_data *acc, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int	err = -1;

	if (atomic_read(&acc->enabled)) {
		/* Sets configuration register at reg_address
		*  NOTE: this is a straight overwrite  */
		buf[0] = reg_address;
		buf[1] = new_value;
		err = mxc622x_acc_i2c_write(acc, buf, 1);
		if (err < 0)
			return err;
	}
	return err;
}

static int mxc622x_acc_register_update(struct mxc622x_acc_data *acc, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int	err = -1;
	u8	init_val;
	u8	updated_val;

	err = mxc622x_acc_register_read(acc, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[1];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		err = mxc622x_acc_register_write(acc, buf, reg_address,
		updated_val);
	}
	return err;
}
#endif

/********************************************************************
 * 功能：通过i2c总线获取acc数据
 *
 * 参数:
 *			
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-5: IO出错
 *
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_get_acceleration_data(struct mxc622x_acc_data *acc,
		int *xyz)
{
	int	err = -1;
	u8	acc_data[2];	/* Data bytes from hardware x, y */

	acc_data[0] = MXC622X_REG_DATA;
	err = mxc622x_acc_i2c_read(acc, acc_data, 2);

	if (err < 0) {
		printk("%s I2C read error %d\n", MXC622X_ACC_I2C_NAME, err);
		return err;
	}

	xyz[0] = (signed char)acc_data[0];
	xyz[1] = (signed char)acc_data[1];
	xyz[2] = 0;

	GSENSOR_DBG("%s Raw Data: read x=%d, y=%d, z=%d\n",
			__func__, xyz[0], xyz[1], xyz[2]);
	GSENSOR_DBG("%s poll interval %d\n", MXC622X_ACC_DEV_NAME, acc->pdata->poll_interval);

	return err;
}

static void coordinate_real_to_android(int *out, const int *in, int dir)
{
	if ((!out) || (!in)) {
		return;
	}

	switch (dir) {
	case 0:
		//x'=-y y'=x z'=z
		out[0] = -in[1];
		out[1] = in[0];
		out[2] = in[2];
		break;
	case 1:
		//x'=x y'=y z'=z
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		break;
	case 2:
		//x'=y y'=-x z'=z
		out[0] = in[1];
		out[1] = -in[0];
		out[2] = in[2];
		break;
	case 3:
		//x'=-x y'=-y z'=z
		out[0] = -in[0];
		out[1] = -in[1];
		out[2] = in[2];
		break;
	case 4:
		//x'=y y'=x z'=-z
		out[0] = in[1];
		out[1] = in[0];
		out[2] = -in[2];
		break;
	case 5:
		//x'=x y'=-y z'=-z
		out[0] = in[0];
		out[1] = -in[1];
		out[2] = -in[2];
		break;
	case 6:
		//x'=-y y'=-x z'=-z
		out[0] = -in[1];
		out[1] = -in[0];
		out[2] = -in[2];
		break;
	case 7:
		//x'=-x y'=y z'=-z
		out[0] = -in[0];
		out[1] = in[1];
		out[2] = -in[2];
		break;
	default:
		//x'=x y'=y z'=z
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		break;
	}
}

static void mxc622x_acc_report_values(struct mxc622x_acc_data *acc, int *xyz)
{
	int	real_acc[3]={0};

	GSENSOR_INFO("%s Raw Data: x=%d, y=%d, z=%d\n",
			__func__, xyz[0], xyz[1], xyz[2]);

	// convert data to android coordinate system
	coordinate_real_to_android(real_acc, xyz, acc->pdata->g_position);	
	GSENSOR_INFO("%s Report Data: x=%d, y=%d, z=%d, dir=%d\n",
			__func__, real_acc[0], real_acc[1], real_acc[2], acc->pdata->g_position);

	input_report_abs(acc->input_dev, ABS_X, real_acc[0]);
	input_report_abs(acc->input_dev, ABS_Y, real_acc[1]);
	input_report_abs(acc->input_dev, ABS_Z, real_acc[2]);
	input_sync(acc->input_dev);
}

static int mxc622x_acc_enable(struct mxc622x_acc_data *acc)
{
	int	err;

	if (!atomic_cmpxchg(&acc->enabled, 0, 1)) {
		err = mxc622x_acc_device_power_on(acc);
		if (err < 0) {
			atomic_set(&acc->enabled, 0);
			return err;
		}

		schedule_delayed_work(&acc->input_work, msecs_to_jiffies(
			acc->pdata->poll_interval));
	}

	return 0;
}

static int mxc622x_acc_disable(struct mxc622x_acc_data *acc)
{
	if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
		cancel_delayed_work_sync(&acc->input_work);
		mxc622x_acc_device_power_off(acc);
	}

	return 0;
}

static int mxc622x_acc_misc_open(struct inode *inode, struct file *file)
{
	int	err;

	err = nonseekable_open(inode, file);
	if (err < 0)
		return err;

	file->private_data = mxc622x_acc_misc_data;

	return 0;
}

static long mxc622x_acc_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user	*argp = (void __user *)arg;
//	u8			buf[4], mask, reg_address, bit_values;
	int			err;
	int			interval;
	int			xyz[3] = {0};
	struct mxc622x_acc_data	*acc = file->private_data;

	GSENSOR_DBG("%s: %s call with cmd 0x%x and arg 0x%x\n",
			MXC622X_ACC_DEV_NAME, __func__, cmd, (unsigned int)arg);

	switch (cmd) {
	case MXC622X_ACC_IOCTL_GET_DELAY:
		interval = acc->pdata->poll_interval;
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EFAULT;
		break;

	case MXC622X_ACC_IOCTL_SET_DELAY:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval < 0 || interval > 1000)
			return -EINVAL;
		if(interval > MAX_INTERVAL)
			interval = MAX_INTERVAL;
		acc->pdata->poll_interval = max(interval,
			acc->pdata->min_interval);
		break;

	case MXC622X_ACC_IOCTL_SET_ENABLE:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval > 1)
			return -EINVAL;
		if (interval)
			err = mxc622x_acc_enable(acc);
		else
			err = mxc622x_acc_disable(acc);
		return err;
		break;

	case MXC622X_ACC_IOCTL_GET_ENABLE:
		interval = atomic_read(&acc->enabled);
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EINVAL;
		break;

	case MXC622X_ACC_IOCTL_GET_COOR_XYZ:	       
		err = mxc622x_acc_get_acceleration_data(acc, xyz);                
		if (err < 0)                    
			return err;
		if (copy_to_user(argp, xyz, sizeof(xyz))) {			
			printk(KERN_ERR " %s %d error in copy_to_user \n", __func__, __LINE__);
			return -EINVAL;
		}                
		break;

	case MXC622X_ACC_IOCTL_GET_CHIP_ID:
		{
			u8	devid = 0;
			u8	devinfo[DEVICE_INFO_LEN] = {0};

			err = mxc622x_acc_register_read(acc, &devid, WHO_AM_I);
			if (err < 0) {
				printk("%s, error read register WHO_AM_I\n", __func__);
				return -EAGAIN;
			}
			sprintf(devinfo, "%s, %#x", DEVICE_INFO, devid);

			if (copy_to_user(argp, devinfo, sizeof(devinfo))) {
				printk("%s error in copy_to_user(IOCTL_GET_CHIP_ID)\n", __func__);
				return -EINVAL;
			}
		}
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations mxc622x_acc_misc_fops = {
	.owner = THIS_MODULE,
	.open = mxc622x_acc_misc_open,
	.unlocked_ioctl = mxc622x_acc_misc_ioctl,
};

static struct miscdevice mxc622x_acc_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MXC622X_ACC_DEV_NAME,
	.fops = &mxc622x_acc_misc_fops,
};

static void mxc622x_acc_input_work_func(struct work_struct *work)
{
	struct mxc622x_acc_data	*acc;
	int	xyz[3] = { 0 };
	int	err;

	acc = container_of((struct delayed_work *)work,
			struct mxc622x_acc_data, input_work);

	mutex_lock(&acc->lock);
	err = mxc622x_acc_get_acceleration_data(acc, xyz);
	//GSENSOR_DBG("%s Raw Data: x=%d, y=%d, z=%d, err=%d\n",
	//		MXC622X_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2], err);
	if (err < 0)
		dev_err(&acc->client->dev, "get_acceleration_data failed\n");
	else
		mxc622x_acc_report_values(acc, xyz);

	schedule_delayed_work(&acc->input_work, msecs_to_jiffies(
		acc->pdata->poll_interval));
	mutex_unlock(&acc->lock);
}

#ifdef MXC622X_ACC_OPEN_ENABLE
int mxc622x_acc_input_open(struct input_dev *input)
{
	struct mxc622x_acc_data	*acc = input_get_drvdata(input);

	return mxc622x_acc_enable(acc);
}

void mxc622x_acc_input_close(struct input_dev *dev)
{
	struct mxc622x_acc_data	*acc = input_get_drvdata(dev);

	mxc622x_acc_disable(acc);
}
#endif

/********************************************************************
 * 功能：确认 acc 参数是否有效
 *
 * 参数:	acc
 *			
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-22:参数无效
 *				
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_validate_pdata(struct mxc622x_acc_data *acc)
{
	acc->pdata->poll_interval = max(acc->pdata->poll_interval,
			acc->pdata->min_interval);

	/* Enforce minimum polling interval */
	if (acc->pdata->poll_interval < acc->pdata->min_interval) {
		dev_err(&acc->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}

	return 0;
}

/********************************************************************
 * 功能：注册input设备
 *
 * 参数:	acc
 *			
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-12:存储空间不够
 *				
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_input_init(struct mxc622x_acc_data *acc)
{
	int	err;

	// Polling rx data when the interrupt is not used.
	if (1/*acc->irq1 == 0 && acc->irq1 == 0*/) {
		INIT_DELAYED_WORK(&acc->input_work, mxc622x_acc_input_work_func);
	}

	acc->input_dev = input_allocate_device();
	if (!acc->input_dev) {
		err = -ENOMEM;
		dev_err(&acc->client->dev, "input device allocate failed\n");
		goto err0;
	}

#ifdef MXC622X_ACC_OPEN_ENABLE
	acc->input_dev->open = mxc622x_acc_input_open;
	acc->input_dev->close = mxc622x_acc_input_close;
#endif

	input_set_drvdata(acc->input_dev, acc);

	set_bit(EV_ABS, acc->input_dev->evbit);

#if 1	// Alex.shi fuzz=0,flat=0;解决问题:不灵敏
	/* acceleration x-axis */
	input_set_abs_params(acc->input_dev, ABS_X, -32768*4, 32768*4, 0, 0);
	/* acceleration y-axis */
	input_set_abs_params(acc->input_dev, ABS_Y, -32768*4, 32768*4, 0, 0);
	/* acceleration z-axis */
	input_set_abs_params(acc->input_dev, ABS_Z, -32768*4, 32768*4, 0, 0);
#else
	input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, FUZZ, FLAT);
#endif

	acc->input_dev->name = MXC622X_ACC_INPUT_NAME;

	err = input_register_device(acc->input_dev);
	if (err) {
		dev_err(&acc->client->dev,
				"unable to register input polled device %s\n",
				acc->input_dev->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(acc->input_dev);
err0:
	return err;
}

static void mxc622x_acc_input_cleanup(struct mxc622x_acc_data *acc)
{
	input_unregister_device(acc->input_dev);
	input_free_device(acc->input_dev);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxc622x_early_suspend (struct early_suspend* es);
static void mxc622x_early_resume (struct early_suspend* es);
#endif

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
static ssize_t mxc622x_delay_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int						interval;
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;

	interval = acc->pdata->poll_interval;
	GSENSOR_DBG("%s:delay%d\n", __FUNCTION__, interval);
	return sprintf(buf, "%d\n", interval);
}

static ssize_t mxc622x_delay_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;
	unsigned long			interval;

	interval = simple_strtoul(buf, NULL, 10);    
	GSENSOR_DBG("%s:delay=0x%lx\n", __FUNCTION__, interval);
	if (interval < 0 || interval > 1000)
		return -EINVAL;
	if(interval > MAX_INTERVAL)
		interval = MAX_INTERVAL;

	acc->pdata->poll_interval = max((int)interval, acc->pdata->min_interval);

	return count;
}

static ssize_t mxc622x_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int						enable;
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;

	enable = atomic_read(&acc->enabled);
	GSENSOR_DBG("%s:enable%d\n", __FUNCTION__, enable);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mxc622x_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;
	int						ret;
	unsigned long			enable;

	enable = simple_strtoul(buf, NULL, 10);    
	GSENSOR_DBG("%s:enable=0x%lx\n", __FUNCTION__, enable);
	enable = (enable > 0) ? 1 : 0;
	if(enable)
	{  
		ret = mxc622x_acc_enable(acc);
		if(!ret){
			GSENSOR_DBG("mxc622x enable setting active \n");
		}
	}
	else
	{
		ret = mxc622x_acc_disable(acc);
		if(!ret){
			GSENSOR_DBG("mxc622x enable setting inactive \n");
		}
	}

	return count;
}

static ssize_t mxc622x_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;

	return sprintf(buf, "%d\n", acc->pdata->g_position);
}

static ssize_t mxc622x_position_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct mxc622x_acc_data	*acc = mxc622x_acc_misc_data;
	int						position;

	position = simple_strtoul(buf, NULL, 10);    
	mutex_lock(&acc->lock);
	acc->pdata->g_position = position;
	mutex_unlock(&acc->lock);

	return count;
}

static ssize_t mxc622x_chip_id_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 0x6225);
}

static DEVICE_ATTR(delay, S_IRWXU | S_IRWXG | S_IRWXO,
			mxc622x_delay_show, mxc622x_delay_store);
static DEVICE_ATTR(enable, S_IRWXU | S_IRWXG | S_IRWXO,
			mxc622x_enable_show, mxc622x_enable_store);
static DEVICE_ATTR(position, S_IRWXU | S_IRWXG | S_IRWXO,
			mxc622x_position_show, mxc622x_position_store);
static DEVICE_ATTR(chip_id, S_IRWXU | S_IRWXG | S_IRWXO,
			mxc622x_chip_id_show, NULL);

static struct attribute *mxc622x_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	&dev_attr_chip_id.attr,
	NULL
};

static const struct attribute_group mxc622x_attr_group = {
	.attrs = mxc622x_attributes,
};

static int check_gsensor_chip(void)
{
	CDC_Gsensor_Device_Id(0x6225);

	return 0;
}

static int remove_gsensor_chip(void)
{
	CDC_Gsensor_Device_Id(0xFFFF);

	return 0;
}
#endif

/********************************************************************
 * 功能：probe接口
 *
 * 参数:
 *			
 *
 * 返回值: 
 * 			成功则返回0, 若失败则返回错误码的负值
 *				-5: 接口出错;	-12:存储空间不够
 *				-19: 无此设备;	-22:参数无效
 *				
 * 说明：	
 ********************************************************************/
static int mxc622x_acc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct mxc622x_acc_data	*acc;
	int						err = -1;

	printk("%s: probe start.\n", MXC622X_ACC_DEV_NAME);

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
	if(CDC_Gsensor_Device_Id(0)!=0)
	{
		printk("GSensor(0x%x)Exist!", CDC_Gsensor_Device_Id(0));
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
#endif

#if 0
	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE |
			I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}


	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK)){
		dev_err(&client->dev, "client not smb-i2c capable:3\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}

	/*
	* OK. From now, we presume we have a valid client. We now create the
	* client structure, even though we cannot fill it completely yet.
	*/
	//LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
	//LDO_TurnOnLDO(LDO_LDO_SIM2);

	acc = kzalloc(sizeof(struct mxc622x_acc_data), GFP_KERNEL);
	if (acc == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"failed to allocate memory for module data: "
			"%d\n", err);
		goto exit_alloc_data_failed;
	}

	mutex_init(&acc->lock);
	mutex_lock(&acc->lock);

	acc->client = client;
	mxc622x_i2c_client = client;
	i2c_set_clientdata(client, acc);

#if SW_SIMULATE_I2C
	{
		u8	devid = 0;

		err = mxc622x_acc_register_read(acc, &devid, WHO_AM_I);
		if (err < 0) {
			printk("%s, error read register WHO_AM_I\n", __func__);
			err = -EIO;
			goto err_mutexunlockfreedata;
		}
		else
			printk("%s:0x%x is detected\n", DEVICE_INFO, devid);
	}
#else
	{
		int						tempvalue;

		/* read chip id */
		tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);
		if (((tempvalue & 0x003F) == WHOAMI_MXC622X_ACC) || ((tempvalue & 0x003F) == WHOAMI_MXC6255XC_ACC)){
			printk(KERN_INFO "%s I2C driver registered!\n", MXC622X_ACC_DEV_NAME);
		} else {
			acc->client = NULL;
			printk(KERN_INFO "I2C driver not registered!" " Device unknown 0x%x\n", tempvalue);
			err = -EIO;
			goto err_mutexunlockfreedata;
		}
	}
#endif

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
	if(check_gsensor_chip()<0)
	{
		err = -ENODEV;
		printk("Fail to find MXC6225");
		goto exit_check_chip_failed;
	}
#endif

	acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
	if (acc->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n",
				err);
		goto exit_kfree_pdata;
	}

	memcpy(acc->pdata, client->dev.platform_data, sizeof(*acc->pdata));

	err = mxc622x_acc_validate_pdata(acc);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}

	i2c_set_clientdata(client, acc);

	if (acc->pdata->init) {
		err = acc->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err2;
		}
	}

	err = mxc622x_acc_device_power_on(acc);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&acc->enabled, 1);

	err = mxc622x_acc_input_init(acc);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}
	mxc622x_acc_misc_data = acc;

	err = misc_register(&mxc622x_acc_misc_device);
	if (err < 0) {
		dev_err(&client->dev, "misc MXC622X_ACC_DEV_NAME register failed\n");
		goto err_input_cleanup;
	}

	mxc622x_acc_device_power_off(acc);

	/* As default, do not report information */
	atomic_set(&acc->enabled, 0);

	acc->on_before_suspend = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
	acc->early_suspend.suspend = mxc622x_early_suspend;
	acc->early_suspend.resume  = mxc622x_early_resume;
	acc->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&acc->early_suspend);
#endif

	mutex_unlock(&acc->lock);

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
    err = sysfs_create_group(&(acc->input_dev->dev.kobj), &mxc622x_attr_group);
	if (err) {
		dev_err(&client->dev, "create device file failed!\n");
		err = -EINVAL;
		goto err_create_sysfs;
	}
#endif

	dev_info(&client->dev, "%s: probed\n", MXC622X_ACC_DEV_NAME);

	return 0;

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
err_create_sysfs:
	#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&acc->early_suspend);
	#endif
	misc_deregister(&mxc622x_acc_misc_device);
#endif
err_input_cleanup:
	mxc622x_acc_misc_data = NULL;
	mxc622x_acc_input_cleanup(acc);
err_power_off:
	mxc622x_acc_device_power_off(acc);
err2:
	if (acc->pdata->exit)
		acc->pdata->exit();
exit_kfree_pdata:
	kfree(acc->pdata);
#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
exit_check_chip_failed:
	remove_gsensor_chip();
#endif
err_mutexunlockfreedata:
	i2c_set_clientdata(client, NULL);
	mutex_unlock(&acc->lock);
	mutex_destroy(&acc->lock);
	kfree(acc);
exit_alloc_data_failed:
exit_check_functionality_failed:
	printk(KERN_ERR "%s: Driver Init failed\n", MXC622X_ACC_DEV_NAME);
	return err;
}

static int mxc622x_acc_remove(struct i2c_client *client)
{
	/* TODO: revisit ordering here once _probe order is finalized */
	struct mxc622x_acc_data *acc = i2c_get_clientdata(client);

#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
	sysfs_remove_group(&(acc->input_dev->dev.kobj), &mxc622x_attr_group);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&acc->early_suspend);
#endif
	misc_deregister(&mxc622x_acc_misc_device);
	mxc622x_acc_misc_data = NULL;
	mxc622x_acc_input_cleanup(acc);
	mxc622x_acc_device_power_off(acc);
	if (acc->pdata->exit)
		acc->pdata->exit();
	kfree(acc->pdata);
#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
	remove_gsensor_chip();
#endif
	i2c_set_clientdata(client, NULL);
	mutex_destroy(&acc->lock);
	kfree(acc);

	return 0;
}

static int mxc622x_acc_resume(struct i2c_client *client)
{
	struct mxc622x_acc_data *acc = i2c_get_clientdata(client);

	GSENSOR_DBG("%s.\n", __func__);
	if (acc != NULL && acc->on_before_suspend) {
		acc->on_before_suspend = 0;
		return mxc622x_acc_enable(acc);
	}

	return 0;
}

static int mxc622x_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxc622x_acc_data *acc = i2c_get_clientdata(client);

	GSENSOR_DBG("%s.\n", __func__);
	if (acc != NULL) {
		if (atomic_read(&acc->enabled)) {
			acc->on_before_suspend = 1;
			return mxc622x_acc_disable(acc);
		}
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxc622x_early_suspend (struct early_suspend* es)
{
	GSENSOR_DBG("%s.\n", __func__);
	mxc622x_acc_suspend(mxc622x_i2c_client, (pm_message_t){.event=0});
}

static void mxc622x_early_resume (struct early_suspend* es)
{
	GSENSOR_DBG("%s.\n", __func__);
	mxc622x_acc_resume(mxc622x_i2c_client);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id mxc622x_acc_id[]
				= { { MXC622X_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, mxc622x_acc_id);

static struct i2c_driver mxc622x_acc_driver = {
	.driver = {
		.name	= MXC622X_ACC_I2C_NAME,
	},
	.probe		= mxc622x_acc_probe,
	.remove		= mxc622x_acc_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.resume		= mxc622x_acc_resume,
	.suspend	= mxc622x_acc_suspend,
#endif
	.id_table	= mxc622x_acc_id,
};


#ifdef I2C_BUS_NUM_STATIC_ALLOC
int i2c_static_add_device(struct i2c_board_info *info, struct i2c_driver *driver)
{
	struct i2c_adapter	*adapter;
	struct i2c_client	*client;
	int					err;

	adapter = i2c_get_adapter(I2C_STATIC_BUS_NUM);
	if (!adapter) {
		pr_err("%s: can't get i2c adapter\n", __FUNCTION__);
		err = -ENODEV;
		goto err_adapter;
	}

	client = i2c_new_device(adapter, info);
	if (!client) {
		pr_err("%s:  can't add i2c device at 0x%x\n", __FUNCTION__, (unsigned int)info->addr);
		err = -ENODEV;
		goto err_device;
	}

	i2c_put_adapter(adapter);

	err = i2c_add_driver(driver);
	if (err != 0) {
		printk("%s: can't add i2c driver\n", __FUNCTION__);
		err = -ENODEV;
		goto err_driver;
	}	

	return 0;

err_driver:
	i2c_unregister_device(client);
	return err;
err_device:
	i2c_put_adapter(adapter);
err_adapter:
	return err;
}
#endif /*I2C_BUS_NUM_STATIC_ALLOC*/

static int __init mxc622x_acc_init(void)
{
	printk("%s accelerometer driver: init\n", MXC622X_ACC_I2C_NAME);
#ifdef CONFIG_ZYT_GSENSOR_COMPATIBLE
	if(CDC_Gsensor_Device_Id(0)!=0)
	{
		printk("GSensor(0x%x)Exist!", CDC_Gsensor_Device_Id(0));
		return -ENODEV;
	}
#endif

#ifdef I2C_BUS_NUM_STATIC_ALLOC
	return i2c_static_add_device(&mxc622x_i2c_boardinfo, &mxc622x_acc_driver);
#else
	return i2c_add_driver(&mxc622x_acc_driver);
#endif
}

static void __exit mxc622x_acc_exit(void)
{
	printk("%s accelerometer driver exit\n", MXC622X_ACC_DEV_NAME);
#ifdef I2C_BUS_NUM_STATIC_ALLOC
	if (NULL != mxc622x_i2c_client)
		i2c_unregister_device(mxc622x_i2c_client);
#endif
	i2c_del_driver(&mxc622x_acc_driver);

	return;
}

module_init(mxc622x_acc_init);
module_exit(mxc622x_acc_exit);

MODULE_DESCRIPTION("mxc622x accelerometer misc driver");
MODULE_AUTHOR("Memsic");
MODULE_LICENSE("GPL");

