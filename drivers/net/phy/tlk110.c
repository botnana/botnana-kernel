/*
 * drivers/net/phy/tlk110.c
 *
 * Driver for TLK110 PHYs
 *
 * Author: Christine Fang
 *
 * Support added for TI TLK110 by christine@mapacode.tw
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <asm-generic/gpio.h>
#include <asm/gpio.h>
#include "tlk110.h"

static unsigned long phy_register = 0x999;	// not a valid reg */

struct gpio_desc *phy_gpio_reset = NULL;

static ssize_t register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long val;

	if (phy_register > PHY_REG_MASK) {
		// assign address
		phy_write(phydev, TLK110_REGCR_REG, TLK110_DEVAD_VAL | (0x0 << 14));
		phy_write(phydev, TLK110_ADDAR_REG, phy_register);
		
		// read extended register
		phy_write(phydev, TLK110_REGCR_REG, TLK110_DEVAD_VAL | (0x1 << 14));
		val = phy_read(phydev, TLK110_ADDAR_REG) & 0xffff;
	}
	else 
		val = phy_read(phydev, phy_register) & 0xffff;

	return sprintf(buf, "0x%lx\n", val);
}

static ssize_t register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct phy_device *phydev = to_phy_device(dev);
	unsigned long reg, val;
	int ret;

	if (sscanf(buf, "%lx %lx", &reg, &val) == 2) {
		goto next;
	}
	else if (sscanf(buf, "%lx", &val) == 1) {
		phy_register = reg;
		return size;
	}
	else {
		printk("%s: Failed to write register\n",
			dev_name(&phydev->dev));
		return -EINVAL;
	}
	
next:	
	if (reg > PHY_REG_MASK) {
		// assign address
		if ((ret = phy_write(phydev, TLK110_REGCR_REG, TLK110_DEVAD_VAL | (0x0 << 14)))) return ret;
		if ((ret = phy_write(phydev, TLK110_ADDAR_REG, reg))) return ret;
		
		// assign data
		if ((ret = phy_write(phydev, TLK110_REGCR_REG, TLK110_DEVAD_VAL | (0x1 << 14)))) return ret;
		if ((ret = phy_write(phydev, TLK110_ADDAR_REG, val & 0xffff))) return ret;
	}
	else {
		ret = phy_write(phydev, reg, val & 0xffff);
	}
	
	if (!ret) 
		ret = size;
	
	return ret;
}

static int tlk110_phy_config_intr(struct phy_device *phydev)
{
	return 0;
}

static int tlk110_phy_ack_interrupt(struct phy_device *phydev)
{
	int rc = phy_read (phydev, TLK110_MISR1_REG);     
	if (rc < 0) return rc;
	
	rc = phy_read (phydev, TLK110_MISR2_REG);     
	
	return rc < 0 ? rc : 0;	
}


static DEVICE_ATTR(reg_rw, 0644, register_show, register_store);


static int tlk110_probe(struct phy_device *phydev)
{
        struct device *dev = &phydev->dev;
        struct tlk110_priv *priv;
		int status;
        static int tlk110_reset_cnt = 0;

        priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
        if (!priv)
                return -ENOMEM;

        if(tlk110_reset_cnt == 0){
            status = gpio_request(TLK110_RESET_PIN_GPIO, "phy_reset");
            if (status < 0) {
                dev_err(dev, "Could not request for RESET GPIO:%i\n", TLK110_RESET_PIN_GPIO);
                printk("... gpio: error pin %d 1\n", TLK110_RESET_PIN_GPIO);
                goto fail_1;
            }
            status = gpio_direction_output(TLK110_RESET_PIN_GPIO, 1);
            if (status) {
                dev_err(dev, "Cannot set output RESET GPIO:%i \n", TLK110_RESET_PIN_GPIO);
                printk("... gpio: error pin %d 2\n", TLK110_RESET_PIN_GPIO);
                goto fail_2;
            }
            else mdelay(10);
		
            status = gpio_request(TLK110_PR1_MII_CTL_GPIO, "gpio2_22");
            if (status < 0) {
                dev_err(dev, "Could not request for GPIO:%i\n", TLK110_PR1_MII_CTL_GPIO);
                printk("... gpio: error pin %d 1\n", TLK110_PR1_MII_CTL_GPIO);
                goto fail_2;
            }
            status = gpio_direction_output(TLK110_PR1_MII_CTL_GPIO, 1);
            if (status) {
                dev_err(dev, "Cannot set output GPIO:%i \n", TLK110_PR1_MII_CTL_GPIO);
                printk("... gpio: error pin %d 2\n", TLK110_PR1_MII_CTL_GPIO);
                goto fail_3;
            }
            else mdelay(10);
		
            status = gpio_request(TLK110_FET_nOE_GPIO, "gpio1_29");
            if (status < 0) {
                dev_err(dev, "Could not request for GPIO:%i\n", TLK110_FET_nOE_GPIO);
                printk("... gpio: error pin %d 1\n", TLK110_FET_nOE_GPIO);
                goto fail_3;
            }
            status = gpio_direction_output(TLK110_FET_nOE_GPIO, 1);
            if (status) {
                dev_err(dev, "Cannot set output GPIO:%i \n", TLK110_FET_nOE_GPIO);
                printk("... gpio: error pin %d 2\n", TLK110_FET_nOE_GPIO);
                goto fail_4;
            }
            else mdelay(10);
		
            status = gpio_request(TLK110_MUX_MII_CLL1_GPIO, "gpio3_10");
            if (status < 0) {
                dev_err(dev, "Could not request for GPIO:%i\n", TLK110_MUX_MII_CLL1_GPIO);
                printk("... gpio: error pin %d 1\n", TLK110_MUX_MII_CLL1_GPIO);
                goto fail_4;
            }
            status = gpio_direction_output(TLK110_MUX_MII_CLL1_GPIO, 1);
            if (status) {
                dev_err(dev, "Cannot set output GPIO:%i \n", TLK110_MUX_MII_CLL1_GPIO);
                printk("... gpio: error pin %d 2\n", TLK110_MUX_MII_CLL1_GPIO);
                goto fail_5;
            }
            else mdelay(10);
            
            gpio_set_value(TLK110_RESET_PIN_GPIO, 0);
            mdelay(1);
            gpio_set_value(TLK110_RESET_PIN_GPIO, 1);
            mdelay(1);
            tlk110_reset_cnt++;
            //printk("... gpio: tlk110 reset %d time(%s)\n", tlk110_reset_cnt, __func__);
        }
		
        status = device_create_file(dev, &dev_attr_reg_rw);
        if (status)
            goto fail_5;
		
        phydev->priv = priv;

        return 0;
		
fail_5:
    gpio_free(TLK110_MUX_MII_CLL1_GPIO);

fail_4:
    gpio_free(TLK110_FET_nOE_GPIO);

fail_3:
    gpio_free(TLK110_PR1_MII_CTL_GPIO);

fail_2:
    gpio_free(TLK110_RESET_PIN_GPIO);
	
fail_1:
    devm_kfree(dev, (void *)priv);
	
    return status;
}

static int tlk110_phy_config_init(struct phy_device *phydev) 
{
	unsigned int val;
	
	/* This is done as a workaround to support TLK110 rev1.0 phy */
	val = phy_read(phydev, TLK110_COARSEGAIN_REG);
	phy_write(phydev, TLK110_COARSEGAIN_REG, (val | TLK110_COARSEGAIN_VAL));

	val = phy_read(phydev, TLK110_LPFHPF_REG);
	phy_write(phydev, TLK110_LPFHPF_REG, (val | TLK110_LPFHPF_VAL));

	val = phy_read(phydev, TLK110_SPAREANALOG_REG);
	phy_write(phydev, TLK110_SPAREANALOG_REG, (val | TLK110_SPANALOG_VAL));

	val = phy_read(phydev, TLK110_VRCR_REG);
	phy_write(phydev, TLK110_VRCR_REG, (val | TLK110_VRCR_VAL));

	val = phy_read(phydev, TLK110_SETFFE_REG);
	phy_write(phydev, TLK110_SETFFE_REG, (val | TLK110_SETFFE_VAL));

	val = phy_read(phydev, TLK110_FTSP_REG);
	phy_write(phydev, TLK110_FTSP_REG, (val | TLK110_FTSP_VAL));

	val = phy_read(phydev, TLK110_ALFATPIDL_REG);
	phy_write(phydev, TLK110_ALFATPIDL_REG, (val | TLK110_ALFATPIDL_VAL));

	val = phy_read(phydev, TLK110_PSCOEF21_REG);
	phy_write(phydev, TLK110_PSCOEF21_REG, (val | TLK110_PSCOEF21_VAL));

	val = phy_read(phydev, TLK110_PSCOEF3_REG);
	phy_write(phydev, TLK110_PSCOEF3_REG, (val | TLK110_PSCOEF3_VAL));

	val = phy_read(phydev, TLK110_ALFAFACTOR1_REG);
	phy_write(phydev, TLK110_ALFAFACTOR1_REG, (val | TLK110_ALFACTOR1_VAL));

	val = phy_read(phydev, TLK110_ALFAFACTOR2_REG);
	phy_write(phydev, TLK110_ALFAFACTOR2_REG, (val | TLK110_ALFACTOR2_VAL));

	val = phy_read(phydev, TLK110_CFGPS_REG);
	phy_write(phydev, TLK110_CFGPS_REG, (val | TLK110_CFGPS_VAL));

	val = phy_read(phydev, TLK110_FTSPTXGAIN_REG);
	phy_write(phydev, TLK110_FTSPTXGAIN_REG, (val | TLK110_FTSPTXGAIN_VAL));

	val = phy_read(phydev, TLK110_SWSCR3_REG);
	phy_write(phydev, TLK110_SWSCR3_REG, (val | TLK110_SWSCR3_VAL));

	val = phy_read(phydev, TLK110_SCFALLBACK_REG);
	phy_write(phydev, TLK110_SCFALLBACK_REG, (val | TLK110_SCFALLBACK_VAL));

	val = phy_read(phydev, TLK110_PHYRCR_REG);
	phy_write(phydev, TLK110_PHYRCR_REG, (val | TLK110_PHYRCR_VAL));

	val = phy_read(phydev, TLK110_RCSR_REG);
	phy_write(phydev, TLK110_RCSR_REG, (val | TLK110_RCSR_VAL));

	val = phy_read(phydev, TLK110_PHYCR_REG);
	phy_write(phydev, TLK110_PHYCR_REG, (val | TLK110_PHYCR_MDIX));
	
	
	return tlk110_phy_ack_interrupt (phydev);
}

static struct phy_driver tlk110_phy_driver = {
	.phy_id		= TLK110_PHY_ID, 
	.phy_id_mask	= TLK110_PHY_MASK,
	.name		= "TLK110",

	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause
				| SUPPORTED_Asym_Pause),
	.flags		= PHY_HAS_INTERRUPT | PHY_HAS_MAGICANEG,

	/* basic functions */
	.probe          = tlk110_probe,
	.config_aneg	= genphy_config_aneg,
	.read_status	= genphy_read_status,
	.config_init	= tlk110_phy_config_init,

	/* IRQ related */
	.ack_interrupt	= tlk110_phy_ack_interrupt,
	.config_intr	= tlk110_phy_config_intr,

	.suspend	= genphy_suspend,
	.resume		= genphy_resume,

	.driver		= { .owner = THIS_MODULE, }
};

static int __init tlk110_init(void)
{
	return phy_driver_register(&tlk110_phy_driver);
}

static void __exit tlk110_exit(void)
{
	return phy_driver_unregister(&tlk110_phy_driver);
}

MODULE_DESCRIPTION("TI PHY driver");
MODULE_AUTHOR("Christine Fang");
MODULE_LICENSE("GPL");

module_init(tlk110_init);
module_exit(tlk110_exit);

