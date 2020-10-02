#include "rc522_config.h"
#include "bsp_spi.h"

//初始化
void RC522_init(void)
{
    gpio_pin_config_t cs_config;

    IOMUXC_SetPinMux(IOMUXC_UART2_RX_DATA_ECSPI3_SCLK, 0);
    IOMUXC_SetPinMux(IOMUXC_UART2_CTS_B_ECSPI3_MOSI, 0);
    IOMUXC_SetPinMux(IOMUXC_UART2_RTS_B_ECSPI3_MISO, 0);

    IOMUXC_SetPinConfig(IOMUXC_UART2_RX_DATA_ECSPI3_SCLK, 0x10b0);
    IOMUXC_SetPinConfig(IOMUXC_UART2_CTS_B_ECSPI3_MOSI, 0x10b0);
    IOMUXC_SetPinConfig(IOMUXC_UART2_RTS_B_ECSPI3_MISO, 0x10b0);

    //片选
    IOMUXC_SetPinMux(IOMUXC_UART2_TX_DATA_GPIO1_IO20, 0);
    IOMUXC_SetPinConfig(IOMUXC_UART2_TX_DATA_GPIO1_IO20, 0x10b0);

    cs_config.direction = kGPIO_DigitalOutput;
    cs_config.outputLogic = 0;
    gpio_init(GPIO1, 20, &cs_config);

    spi_init(ECSPI3);

    RC522_RST_Config();
    //RST关闭
    gpio_pinwrite(GPIO1, 30, 1);

}

void RC522_RST_Config(void)
{
    gpio_pin_config_t rst_config;
    //rst引脚复用
    IOMUXC_SetPinMux(IOMUXC_UART5_TX_DATA_GPIO1_IO30, 0);

    IOMUXC_SetPinConfig(IOMUXC_UART5_TX_DATA_GPIO1_IO30, 0x10b0);
    //rst引脚初始化
    rst_config.direction = kGPIO_DigitalOutput;
    rst_config.outputLogic = 1;
    gpio_init(GPIO1, 30, &rst_config);
}