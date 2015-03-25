#include <linux/delay.h>
#include "csi_driver.h"
#include "csi_access.h"
#include "csi_log.h"
//#include "os_api.h"


static int csi_core_initialized = 0;


static void dphy_write(u8 test_code, u8 test_data, u8* test_out)
{
	u32 temp = 0xffffff00;

	csi_core_write_part(PHY_TST_CRTL1, 0, PHY_TESTEN, 1); //phy_testen = 0
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0,1, PHY_TESTCLK, 1); //phy_testclk = 1
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, test_code, PHY_TESTDIN, PHY_TESTDIN_W); //phy_testdin
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, 1, PHY_TESTEN, 1);//phy_testen = 1
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0, 0, PHY_TESTCLK, 1);//phy_testclk = 0
	udelay(1);
	temp = csi_core_read_part(PHY_TST_CRTL1, PHY_TESTDOUT,PHY_TESTDOUT_W);
	*test_out = (u8)temp;
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, 0, PHY_TESTEN, 1); //phy_testen = 0
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, test_data, PHY_TESTDIN, PHY_TESTDIN_W);//phy_testdin
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0,1, PHY_TESTCLK, 1);//phy_testclk = 1
	udelay(1);
}

static void dphy_cfg_start(void)
{
	csi_core_write_part(PHY_TST_CRTL1, 0, PHY_TESTEN, 1); //phy_testen = 0
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0,1, PHY_TESTCLK, 1); //phy_testclk = 1
	udelay(1);
}

static void dphy_cfg_done(void)
{
	csi_core_write_part(PHY_TST_CRTL1, 0, PHY_TESTEN, 1); //phy_testen = 0
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0,1, PHY_TESTCLK, 1); //phy_testclk = 1
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, 0, PHY_TESTDIN, PHY_TESTDIN_W);//phy_testdin
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL1, 1, PHY_TESTEN, 1);//phy_testen = 1
	udelay(1);
	csi_core_write_part(PHY_TST_CRTL0, 0, PHY_TESTCLK, 1);//phy_testclk = 0
	udelay(1);
}

void dphy_init(void)
{
    u8 temp = 0;

    csi_core_write_part(PHY_SHUTDOWNZ,  0, 0, 1);
    csi_core_write_part(DPHY_RSTZ, 0, 0, 1);
    csi_core_write_part(PHY_TST_CRTL0, 1, PHY_TESTCLR, 1);
    udelay(1);
    csi_core_write_part(PHY_TST_CRTL0, 0, PHY_TESTCLR, 1);
    udelay(1);
    dphy_cfg_start();

    dphy_write(0x34, 0x14, &temp);
    dphy_write(0x44, 0x14, &temp);
    dphy_write(0x54, 0x14, &temp);
    dphy_write(0x64, 0x14, &temp);
    dphy_write(0x74, 0x14, &temp);

    dphy_cfg_done();
   
}


u8 csi_init(u32 base_address)
{
    csi_error_t e = SUCCESS;
    do
    {
        if (csi_core_initialized == 0)
        {
            access_init((u32*)base_address);
            if (csi_core_read(VERSION) == (u32)(CURRENT_VERSION))
            {
                csi_core_initialized = 1;
                break;
            }
            else
            {
                LOG_ERROR("Driver not compatible with core");
                e = ERR_NOT_COMPATIBLE;
                break;
            }
        }
        else
        {
            LOG_ERROR("driver already initialised");
            e = ERR_ALREADY_INIT;
            break;
        }

    } while(0);
    return e;
}

u8 csi_close()
{
    csi_shut_down_phy(1);
    csi_reset_controller();
    csi_core_initialized = 0;
    return SUCCESS;
}

u8 csi_get_on_lanes()
{
    return (csi_core_read_part(N_LANES, 0, 2) + 1);
}

u8 csi_set_on_lanes(u8 lanes)
{
    return csi_core_write_part(N_LANES, (lanes - 1), 0, 2);
}

u8 csi_shut_down_phy(u8 shutdown)
{
    LOG_DEBUG2("shutdown", shutdown);
    /*  active low - bit 0 */
    return csi_core_write_part(PHY_SHUTDOWNZ, shutdown? 0: 1, 0, 1);
}

u8 csi_reset_phy()
{
    /*  active low - bit 0 */
    int retVal = 0xffff;
    retVal = csi_core_write_part(DPHY_RSTZ, 0, 0, 1);
    switch (retVal)
    {
    case SUCCESS:
        return csi_core_write_part(DPHY_RSTZ, 1, 0, 1);
        break;
    case ERR_NOT_INIT:
        LOG_ERROR("Driver not initialized");
        return retVal;
        break;
    default:
        LOG_ERROR("Undefined error");
        return ERR_UNDEFINED;
        break;
    }
}

u8 csi_reset_controller()
{
    /*  active low - bit 0 */
    int retVal = 0xffff;
    retVal = csi_core_write_part(CSI2_RESETN, 0, 0, 1);
    switch (retVal)
    {
    case SUCCESS:
        return csi_core_write_part(CSI2_RESETN, 1, 0, 1);
        break;
    case ERR_NOT_INIT:
        LOG_ERROR("Driver not initialized");
        return retVal;
        break;
    default:
        LOG_ERROR("Undefined error");
        return ERR_UNDEFINED;
        break;
    }
}

csi_lane_state_t csi_lane_module_state(u8 lane)
{
    if (lane < csi_core_read_part(N_LANES, 0, 2) + 1)
    {
        if (csi_core_read_part(PHY_STATE, lane, 1))
        {
            return CSI_LANE_ULTRA_LOW_POWER;
        }
        else if (csi_core_read_part(PHY_STATE, (lane + 4), 1))
        {
            return CSI_LANE_STOP;
        }
        return CSI_LANE_ON;
    }
    else
    {
        LOG_WARNING("Lane switched off");
        return CSI_LANE_OFF;
    }
}

csi_lane_state_t csi_clk_state()
{
    if (!csi_core_read_part(PHY_STATE, 9, 1))
    {
        return CSI_LANE_ULTRA_LOW_POWER;
    }
    else if (csi_core_read_part(PHY_STATE, 10, 1))
    {
        return CSI_LANE_STOP;
    }
    else if (csi_core_read_part(PHY_STATE, 8, 1))
    {
        return CSI_LANE_HIGH_SPEED;
    }
    return CSI_LANE_ON;
}

u8 csi_payload_bypass(u8 on)
{
    return csi_core_write_part(PHY_STATE, on? 1: 0, 11, 1);
}

u8 csi_register_line_event(u8 virtual_channel_no, csi_data_type_t data_type, u8 offset)
{
    u8 id = 0;
    csi_registers_t reg_offset = 0;
    LOG_TRACE();
    if ((virtual_channel_no > 4) || (offset > 8))
    {
        return ERR_OUT_OF_BOUND;
    }
    id = (virtual_channel_no << 6) | data_type;

    reg_offset = ((offset / 4) == 1) ? DATA_IDS_2: DATA_IDS_1;

    return csi_core_write_part(reg_offset, id, (offset * 8), 8);
}
u8 csi_unregister_line_event(u8 offset)
{
    csi_registers_t reg_offset = 0;
    LOG_TRACE();
    if (offset > 8)
    {
        return ERR_OUT_OF_BOUND;
    }
    reg_offset = ((offset / 4) == 1) ? DATA_IDS_2: DATA_IDS_1;
    return csi_core_write_part(reg_offset, 0x00, (offset * 8), 8);
}

u8 csi_get_registered_line_event(u8 offset)
{
    csi_registers_t reg_offset = 0;
    LOG_TRACE();
    if (offset > 8)
    {
        return ERR_OUT_OF_BOUND;
    }
    reg_offset = ((offset / 4) == 1) ? DATA_IDS_2: DATA_IDS_1;
    return (u8)csi_core_read_part(reg_offset, (offset * 8), 8);
}

u8 csi_event_disable(u32 mask, u8 err_reg_no)
{
    switch (err_reg_no)
    {
        case 1:
            return csi_core_write(MASK1, mask | csi_core_read(MASK1));
        case 2:
            return csi_core_write(MASK2, mask | csi_core_read(MASK2));
        default:
            return ERR_OUT_OF_BOUND;
    }
}

u8 csi_event_enable(u32 mask, u8 err_reg_no)
{
    switch (err_reg_no)
    {
        case 1:
            return csi_core_write(MASK1, (~mask) & csi_core_read(MASK1));
        case 2:
            return csi_core_write(MASK2, (~mask) & csi_core_read(MASK2));
        default:
            return ERR_OUT_OF_BOUND;
    }
}
u32 csi_event_get_source(u8 err_reg_no)
{
    switch (err_reg_no)
    {
        case 1:
            return csi_core_read(ERR1);
        case 2:
            return csi_core_read(ERR2);
        default:
            return ERR_OUT_OF_BOUND;
    }
}

/*  register methods */
u32 csi_core_read(csi_registers_t address)
{
    return access_read(address>>2);
}

u8 csi_core_write(csi_registers_t address, u32 data)
{
    if (csi_core_initialized == 0)
    {
        LOG_ERROR("driver not initialised");
        return ERR_NOT_INIT;
    }
    LOG_DEBUG3("write", data, address>>2);
    access_write(data, address>>2);
    return SUCCESS;
}

u8 csi_core_write_part(csi_registers_t address, u32 data, u8 shift, u8 width)
{
    u32 mask = (1 << width) - 1;
    u32 temp = csi_core_read(address);
    temp &= ~(mask << shift);
    temp |= (data & mask) << shift;
    return csi_core_write(address, temp);
}

u32 csi_core_read_part(csi_registers_t address, u8 shift, u8 width)
{
    return (csi_core_read(address) >> shift) & ((1 << width) - 1);
}

