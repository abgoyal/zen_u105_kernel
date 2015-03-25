// ****************************************** //
// access IIC, GPIO, etc. from user space by anli.wei
// 2012-11-25
// 2013-01-22 add key info anli.wei
// ****************************************** //

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/module.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/pinmap.h>
//
#include "autotstdrv.h"

//
#define DBG_ENABLE_FUN
#define DBG_ENABLE_INFO

#ifdef DBG_ENABLE_FUN
#define FUN_ENTER               printk(KERN_INFO "autotst-> %s ++.\n", __FUNCTION__)
#define FUN_LEAVE               printk(KERN_INFO "autotst-> %s --.\n", __FUNCTION__)
#else
#define FUN_ENTER               do {} while(0)
#define FUN_LEAVE               do {} while(0)
#endif // DBG_ENABLE_FUN

#ifdef DBG_ENABLE_INFO
#define DBG_INFO(fmt, arg...)   printk(KERN_INFO "autotst-> " fmt, ##arg)
#else
#define DBG_INFO(fmt, arg...)   do {} while(0)
#endif // DBG_ENABLE_INFO

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define SPRD_MAX_PIN                200
#define SPRD_GPIO_PIN_INVALID_VALUE 0xFFFFFFFF

//static uint32_t  sGpioPins[SPRD_MAX_PIN];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
extern int     lcm_init( void );
extern int32_t lcm_send_data (uint32_t data);
//del by kl 201490904 extern int sprdfb_test_init( uint32_t rgb , int phy_feq );
//del by kl 20140904 extern int32_t autotst_dispc_uninit(int display_type);
//------------------------------------------------------------------------------
#define CDEV_NAME   "autotst"

static dev_t         s_devt;
static struct cdev   s_cdev;
static struct class *s_class;

//------------------------------------------------------------------------------

static int autotst_open (struct inode *inode, struct file *filp)
{    
	FUN_ENTER;
	FUN_LEAVE;
    return 0;
}

static int autotst_release (struct inode *inode, struct file *filp)
{
	FUN_ENTER;
	FUN_LEAVE;
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int i2c_read( struct autotst_i2c_info_t * info ) 
{
    struct i2c_msg xfer[2];
    u8  reg[2], reg_len = 0;
    u8  addr = (info->addr >> 1);
    int ret;
    
    struct i2c_adapter * adpt = i2c_get_adapter(info->bus);
    if( NULL == adpt ) {
        printk(KERN_ERR "get adapter(%d) fail!\n", info->bus);
        return -ENXIO;
    }
    
    DBG_INFO("i2c read: bus = %d, addr = %X, reg = %X\n", info->bus, addr, info->reg);
    
    if( 16 == info->regBits ) {
        reg[reg_len++] = (u8)(info->reg >> 8);
    }
    reg[reg_len++] = (u8)(info->reg);
    
	// Write address
	xfer[0].addr  = addr;
	xfer[0].flags = 0;
	xfer[0].len   = reg_len;
	xfer[0].buf   = reg;

	/* Read data */
	xfer[1].addr  = addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len   = info->data_len;
	xfer[1].buf   = info->data;

	ret = i2c_transfer(adpt, xfer, 2);
	if( ret < 0 ) {
		printk(KERN_ERR "i2c_transfer() returned %d\n", ret);
        
        i2c_put_adapter(adpt);
		return ret;
	}
    
    DBG_INFO("i2c read done: bus = %d, addr = 0x%04X, reg = 0x%04X\n", 
        info->bus, info->addr, info->reg);
        
    i2c_put_adapter(adpt);
    return 0;
}

//------------------------------------------------------------------------------
static int i2c_write( struct autotst_i2c_info_t * info ) 
{
    u8  buf[64];
    u8  num = 0;
    int ret, i;
    
    struct i2c_msg xfer;
    
    struct i2c_adapter * adpt = i2c_get_adapter(info->bus);
    if( NULL == adpt ) {
        printk(KERN_ERR "get adapter(%d) fail!\n", info->bus);
        return -ENXIO;
    }
    
    if( 16 == info->regBits ) {
        buf[num++] = (u8)(info->reg >> 8);
    }
    
    buf[num++] = (u8)(info->reg);
        
    for( i = 0; i < info->data_len; ++i )
    {
        buf[num++] = info->data[i];
    }
        
    xfer.addr  = info->addr;
    xfer.flags = 0;
    xfer.len   = num;
    xfer.buf   = buf;
    
    ret = i2c_transfer(adpt, &xfer, 1);
	if( ret < 0 ) {
		printk(KERN_ERR "i2c_transfer() returned %d\n", ret);
        
        i2c_put_adapter(adpt);
		return ret;
	}

    DBG_INFO("i2c write done: bus = %d, addr = 0x%04X, reg = 0x%04X\n", 
        info->bus, info->addr, info->reg);
        
    i2c_put_adapter(adpt);
	return 0;
}

//------------------------------------------------------------------------------
static int i2c_ioctl( unsigned int cmd, unsigned long arg )
{
    int ret = 0;
    struct autotst_i2c_info_t iit;
    
    if( copy_from_user(&iit, (const void __user *)arg, sizeof(struct autotst_i2c_info_t)) ) {
        printk(KERN_ERR "copy_from_user fail: arg = %lu\n", arg);
        return -EFAULT;
    } 
    
    switch( cmd ) {
    case AUTOTST_IOCTL_I2C_READ:
        ret = i2c_read( &iit );
        if( ret == 0 ) {
            if( copy_to_user((void __user *)arg, &iit, sizeof(struct autotst_i2c_info_t)) ) {
                printk(KERN_ERR "copy_to_user fail: arg = %lu\n", arg);
                ret = -EFAULT;
            }
        }
        break;
    case AUTOTST_IOCTL_I2C_WRITE:
        ret = i2c_write( &iit );
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct {
            uint32_t reg;
            uint32_t val;
} pinmap_lcd_t;

pinmap_lcd_t const pinmap_lcd[] = {
    {REG_PIN_LCD_CSN1,            BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_RSTN,            BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_CD,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D0,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D1,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D2,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D3,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D4,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D5,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D6,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D7,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D8,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_WRN,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_RDN,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_CSN0,            BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D9,              BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D10,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D11,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D12,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D13,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D14,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D15,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D16,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_D17,             BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
    {REG_PIN_LCD_FMARK,           BITS_PIN_DS(1)|BITS_PIN_AF(3)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
};

static int pin_init(void)
{
            int i;
            printk("#####pin_init\n");
            for (i = 0; i < ARRAY_SIZE(pinmap_lcd); i++) {
                __raw_writel(pinmap_lcd[i].val, CTL_PIN_BASE + pinmap_lcd[i].reg);
                                                }
            return 0;
}

static int gpio_ioctl( unsigned int cmd, unsigned long arg )
{
    int ret = 0;
    struct autotst_gpio_info_t git;
    
    if( copy_from_user(&git, (const void __user *)arg, sizeof(struct autotst_gpio_info_t)) ) {
        printk(KERN_ERR "copy_from_user fail: arg = %lu\n", arg);
        return -EFAULT;
    } 

    printk("#####cmd = %dat_gpio %d\n", cmd , git.gpio);
    
    switch( cmd ) {
    case AUTOTST_IOCTL_GPIO_INIT:
    {
        char name[32];
        // del by kl pin_init();
        snprintf(name, 32, "at_gio_%d", git.gpio);
/*      // not support, already config on platform
        if( git.pup_enb ) {
        }
        if( git.pdwn_enb ) {
        }
*/      
        gpio_request(git.gpio, name);
        if( AUTOTST_GPIO_DIR_IN == git.dir ) {
            gpio_direction_input(git.gpio);
        } else {
            gpio_direction_output(git.gpio, git.val);
        }
    }
        break;
    case AUTOTST_IOCTL_GPIO_GET: {
		int gv = gpio_get_value(git.gpio);

        git.val = (gv > 0) ? 1 : 0;
        if( copy_to_user((void __user *)arg, &git, sizeof(struct autotst_gpio_info_t)) ) {
            printk(KERN_ERR "copy_to_user fail: arg = %lu\n", arg);
            ret = -EFAULT;
        }
	}
        break;
    case AUTOTST_IOCTL_GPIO_SET:
        gpio_set_value(git.gpio, git.val);
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int key_ioctl( unsigned int cmd, unsigned long arg )
{
	int ret = 0;
	struct autotst_key_info_t kit;

    if( copy_from_user(&kit, (const void __user *)arg, sizeof(struct autotst_key_info_t)) ) {
        printk(KERN_ERR "copy_from_user fail: arg = %lu\n", arg);
        return -EFAULT;
    }

	if( KEY_POWER == kit.val ) {
		kit.row = AUTOTST_KEY_INVALID_ROW;
		kit.col = AUTOTST_KEY_INVALID_COL;
		kit.gio = EIC_KEY_POWER;
	} else {
		unsigned char rc = (kit.val & 0xF7);

		kit.row = ((rc >> 4) & 0x0F);
		kit.col = ((rc >> 0) & 0x0F);
		kit.gio = 0;
	}
	
	if( 0 == ret ) {
		if( copy_to_user((void __user *)arg, &kit, sizeof(struct autotst_key_info_t)) ) {
            printk(KERN_ERR "copy_to_user fail: arg = %lu\n", arg);
            ret = -EFAULT;
        }
	}

	return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static long autotst_ioctl( struct file *filp, unsigned int cmd, unsigned long arg )
{
    long ret = -1;
   
	FUN_ENTER;

	DBG_INFO("cmd = 0x%X\n", cmd);
    switch( cmd ) {
    case AUTOTST_IOCTL_I2C_READ:
    case AUTOTST_IOCTL_I2C_WRITE:
        ret = i2c_ioctl(cmd, arg);
        break;
    case AUTOTST_IOCTL_GPIO_INIT:
    case AUTOTST_IOCTL_GPIO_GET:
    case AUTOTST_IOCTL_GPIO_SET:
        ret = gpio_ioctl(cmd, arg);
        break;
    case AUTOTST_IOCTL_LCD_DATA:
    {
        int i;
        DBG_INFO("lcd data = 0x%X\n", (uint32_t)arg);
        
        lcm_init();
#if 1
        for( i = 0; i < 32; ++i ) {
            lcm_send_data((uint32_t)arg);
            //dispc_mcu_send_data((uint32_t)arg);
        }
#endif
        ret = 0;
    }
        break;
#if 0  // del by kl 20140904
        case AUTOTST_IOCTL_LCD_MIPI_ON:
        {
				int p_freq = 100*1000;
                uint32_t i = 0x555555;
                DBG_INFO("lcd mipi on\n");
				sprdfb_test_init(i,p_freq);
                ret = 0;
        }
        break;
        case AUTOTST_IOCTL_LCD_MIPI_OFF:
        {
                DBG_INFO("lcd mipi off\n");
                autotst_dispc_uninit(2);
                   ret = 0;
        }
        break;
#endif
	case AUTOTST_IOCTL_GET_KEYINFO:
		ret = key_ioctl(cmd, arg);
		break;
    default:
        ret = -EINVAL;
        break;
    }
   
	FUN_LEAVE;
    return ret;
}

static struct file_operations autotst_fops =
{
    .owner          = THIS_MODULE,
    .unlocked_ioctl = autotst_ioctl,
    .open           = autotst_open,
    .release        = autotst_release,
};

//==============================================================================

static int __init autotst_init(void)
{
    int ret;
	
    FUN_ENTER;

	// auto select a major
    ret = alloc_chrdev_region(&s_devt, 0, 1, CDEV_NAME);
	if( ret < 0 ) {
        printk(KERN_ERR "ERROR: alloc_chrdev_region %d\n", ret);
        return ret;
    }
    
	cdev_init(&s_cdev, &autotst_fops);
	ret = cdev_add(&s_cdev, s_devt, 1);
    if( ret < 0 )
	{
		unregister_chrdev_region(s_devt, 1);
        printk(KERN_ERR "ERROR: cdev_add %d\n", ret);
        return ret;
	}

    s_class = class_create(THIS_MODULE, CDEV_NAME);
    device_create(s_class, NULL, MKDEV(MAJOR(s_devt), MINOR(s_devt)), NULL, CDEV_NAME);
	
    //for( i = 0; i < SPRD_MAX_PIN; ++i ) {
    //    sGpioPins[i] = SPRD_GPIO_PIN_INVALID_VALUE;
    //}
    //sGpioPins[59] = MFP_CFG_X(SIMCLK3,	AF3,	DS1,	F_PULL_NONE,	S_PULL_NONE,	IO_OE);

    FUN_LEAVE;
    return 0;
}

static void __exit autotst_exit(void)
{
	FUN_ENTER;
	
    device_destroy(s_class, MKDEV(MAJOR(s_devt), MINOR(s_devt)));
    class_destroy(s_class);

    cdev_del(&s_cdev);
    unregister_chrdev_region(s_devt, 1);
    
    FUN_LEAVE;
}

module_init(autotst_init);
module_exit(autotst_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("anli.wei");
MODULE_VERSION("0.0.1");
