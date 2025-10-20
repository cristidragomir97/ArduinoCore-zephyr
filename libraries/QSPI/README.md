# QSPI Library

This library provides a simple Arduino-style interface for accessing QSPI (Quad SPI) flash memory on Arduino Zephyr boards.

## Features

- Initialize and configure QSPI flash
- Read data from QSPI flash memory
- Write data to QSPI flash memory
- Erase sectors/blocks
- Get flash information (size, sector size, page size)


### Device Tree Setup

Added to `arduino_giga_r1_stm32h747xx_m7.overlay` file:

```dts
&quadspi {
    pinctrl-0 = <&quadspi_clk_pf10 &quadspi_bk1_ncs_pb6
                 &quadspi_bk1_io0_pf8 &quadspi_bk1_io1_pf9
                 &quadspi_bk1_io2_pf7 &quadspi_bk1_io3_pf6>;
    pinctrl-names = "default";
    status = "okay";

    qspi_flash: qspi-nor-flash@90000000 {
        compatible = "st,stm32-qspi-nor";
        reg = <0x90000000 DT_SIZE_M(16)>;
        qspi-max-frequency = <80000000>;
        size = <DT_SIZE_M(16) * 8>;
        spi-bus-width = <4>;
        status = "okay";
    };
};
```

### Configuration Setup

Added to the `arduino_giga_r1_stm32h747xx_m7.conf`:

```kconfig
CONFIG_SPI_STM32_QSPI=y
CONFIG_SPI_NOR=y
CONFIG_SPI_NOR_SFDP_DEVICETREE=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FLASH_PAGE_LAYOUT=y
```

No changes were needed to the llext. 