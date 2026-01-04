/****************************************************************************
 * examples/gpio_control/gpio_control_main.c
 *
 *   Copyright 2025 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPIO Pin definitions - Spresense Extension Board Pin Numbers */
#define GPIO_D0   PIN_UART2_RXD     /* D0  */
#define GPIO_D1   PIN_UART2_TXD     /* D1  */
#define GPIO_D2   PIN_HIF_IRQ_OUT   /* D2  */
#define GPIO_D3   PIN_PWM3          /* D3  */
#define GPIO_D4   PIN_SPI4_CS_X     /* D4  */
#define GPIO_D5   PIN_PWM1          /* D5  */
#define GPIO_D6   PIN_PWM0          /* D6  */
#define GPIO_D7   PIN_SPI3_CS1_X    /* D7  */
#define GPIO_D8   PIN_SPI3_CS0_X    /* D8  */
#define GPIO_D9   PIN_PWM2          /* D9  */
#define GPIO_D10  PIN_SPI4_SCK      /* D10 */
#define GPIO_D11  PIN_SPI4_MOSI     /* D11 */
#define GPIO_D12  PIN_SPI4_MISO     /* D12 */
#define GPIO_D13  PIN_SPI3_SCK      /* D13 */
#define GPIO_D14  PIN_I2C0_BDT      /* D14 */
#define GPIO_D15  PIN_I2C0_BCK      /* D15 */
#define GPIO_D16  PIN_EMMC_DATA0    /* D16 */
#define GPIO_D17  PIN_EMMC_DATA1    /* D17 */
#define GPIO_D18  PIN_I2S1_DATA_OUT /* D18 */
#define GPIO_D19  PIN_I2S1_DATA_IN  /* D19 */
#define GPIO_D20  PIN_EMMC_DATA2    /* D20 */
#define GPIO_D21  PIN_EMMC_DATA3    /* D21 */
#define GPIO_D22  PIN_SEN_IRQ_IN    /* D22 */
#define GPIO_D23  PIN_EMMC_CLK      /* D23 */
#define GPIO_D24  PIN_EMMC_CMD      /* D24 */
#define GPIO_D25  PIN_I2S1_LRCK     /* D25 */
#define GPIO_D26  PIN_I2S1_BCK      /* D26 */
#define GPIO_D27  PIN_I2S0_DATA_OUT /* D27 */
#define GPIO_D28  PIN_I2S0_DATA_IN  /* D28 */

/* UART2 Pins (Used for Stepper Motor) */
#define GPIO_UART2_TXD PIN_UART2_TXD
#define GPIO_UART2_RXD PIN_UART2_RXD
#define GPIO_UART2_CTS PIN_UART2_CTS
#define GPIO_UART2_RTS PIN_UART2_RTS

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gpio_pin_map_s
{
  const char *name;
  int pin;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct gpio_pin_map_s g_pin_map[] =
{
  {"D0",  GPIO_D0},
  {"D1",  GPIO_D1},
  {"D2",  GPIO_D2},
  {"D3",  GPIO_D3},
  {"D4",  GPIO_D4},
  {"D5",  GPIO_D5},
  {"D6",  GPIO_D6},
  {"D7",  GPIO_D7},
  {"D8",  GPIO_D8},
  {"D9",  GPIO_D9},
  {"D10", GPIO_D10},
  {"D11", GPIO_D11},
  {"D12", GPIO_D12},
  {"D13", GPIO_D13},
  {"D14", GPIO_D14},
  {"D15", GPIO_D15},
  {"D16", GPIO_D16},
  {"D17", GPIO_D17},
  {"D18", GPIO_D18},
  {"D19", GPIO_D19},
  {"D20", GPIO_D20},
  {"D21", GPIO_D21},
  {"D22", GPIO_D22},
  {"D23", GPIO_D23},
  {"D24", GPIO_D24},
  {"D25", GPIO_D25},
  {"D26", GPIO_D26},
  {"D27", GPIO_D27},
  {"D28", GPIO_D28},
  /* UART2 Pins */
  {"UART2_TXD", GPIO_UART2_TXD},
  {"UART2_RXD", GPIO_UART2_RXD},
  {"UART2_CTS", GPIO_UART2_CTS},
  {"UART2_RTS", GPIO_UART2_RTS},
  /* Also add short names for convenience */
  {"TX",  GPIO_UART2_TXD},
  {"RX",  GPIO_UART2_RXD},
  {"CTS", GPIO_UART2_CTS},
  {"RTS", GPIO_UART2_RTS},
  {NULL, -1}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * name: get_pin_number
 * 
 * Get pin number from pin name (e.g., "D0", "D1", etc.)
 */
static int get_pin_number(const char *pin_name)
{
  int i;
  
  for (i = 0; g_pin_map[i].name != NULL; i++)
    {
      if (strcasecmp(pin_name, g_pin_map[i].name) == 0)
        {
          return g_pin_map[i].pin;
        }
    }
  
  return -1;
}

/**
 * name: init_gpio
 * 
 * Initialize GPIO pin as output
 */
static int init_gpio(int pin)
{
  /* Set pin as output with initial LOW state */
  board_gpio_write(pin, 0);
  board_gpio_config(pin, 0, false, true, PIN_FLOAT);
  
  return 0;
}

/**
 * name: set_gpio
 * 
 * Set GPIO pin to HIGH (1) or LOW (0)
 */
static int set_gpio(int pin, int value)
{
  board_gpio_write(pin, value ? 1 : 0);
  return 0;
}

/**
 * name: read_gpio
 * 
 * Read GPIO pin value
 */
static int read_gpio(int pin)
{
  return board_gpio_read(pin);
}

/**
 * name: print_usage
 * 
 * Print usage information
 */
static void print_usage(const char *cmd)
{
  printf("\n");
  printf("GPIO Control Tool for Spresense\n");
  printf("================================\n\n");
  printf("Usage:\n");
  printf("  %s <pin> on          - Set GPIO pin to HIGH\n", cmd);
  printf("  %s <pin> off         - Set GPIO pin to LOW\n", cmd);
  printf("  %s <pin> read        - Read GPIO pin value\n", cmd);
  printf("  %s <pin> toggle      - Toggle GPIO pin\n", cmd);
  printf("  %s list             - List available GPIO pins\n", cmd);
  printf("  %s help             - Show this help message\n\n", cmd);
  printf("Examples:\n");
  printf("  %s D18 on           - Turn on GPIO D18\n", cmd);
  printf("  %s D19 off          - Turn off GPIO D19\n", cmd);
  printf("  %s D20 read         - Read GPIO D20 value\n", cmd);
  printf("  %s D21 toggle       - Toggle GPIO D21\n\n", cmd);
  printf("Available pins: D0-D28\n");
  printf("Note: Some pins may be used by other peripherals.\n");
  printf("      Common GPIO pins: D18, D19, D25, D26\n\n");
}

/**
 * name: list_pins
 * 
 * List all available GPIO pins
 */
static void list_pins(void)
{
  int i;
  
  printf("\n");
  printf("Available GPIO Pins:\n");
  printf("====================\n\n");
  
  for (i = 0; g_pin_map[i].name != NULL; i++)
    {
      printf("  %s\n", g_pin_map[i].name);
    }
  
  printf("\n");
  printf("Commonly used GPIO pins:\n");
  printf("  D18 (I2S1_DATA_OUT)\n");
  printf("  D19 (I2S1_DATA_IN)\n");
  printf("  D25 (I2S1_LRCK)\n");
  printf("  D26 (I2S1_BCK)\n");
  printf("\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * name: main
 * 
 * Main entry point for gpio_control application
 */
int gpio_control_main(int argc, FAR char *argv[])
{
  int pin;
  int value;
  
  /* Check arguments */
  if (argc < 2)
    {
      print_usage(argv[0]);
      return -1;
    }
  
  /* Handle special commands */
  if (strcasecmp(argv[1], "help") == 0)
    {
      print_usage(argv[0]);
      return 0;
    }
  
  if (strcasecmp(argv[1], "list") == 0)
    {
      list_pins();
      return 0;
    }
  
  /* Need at least 3 arguments for pin operations */
  if (argc < 3)
    {
      printf("Error: Missing command. Use '%s help' for usage.\n", argv[0]);
      return -1;
    }
  
  /* Get pin number */
  pin = get_pin_number(argv[1]);
  if (pin < 0)
    {
      printf("Error: Invalid pin name '%s'\n", argv[1]);
      printf("Use '%s list' to see available pins.\n", argv[0]);
      return -1;
    }
  
  /* Initialize GPIO */
  init_gpio(pin);
  
  /* Execute command */
  if (strcasecmp(argv[2], "on") == 0 || strcasecmp(argv[2], "1") == 0)
    {
      set_gpio(pin, 1);
      printf("GPIO %s set to HIGH\n", argv[1]);
    }
  else if (strcasecmp(argv[2], "off") == 0 || strcasecmp(argv[2], "0") == 0)
    {
      set_gpio(pin, 0);
      printf("GPIO %s set to LOW\n", argv[1]);
    }
  else if (strcasecmp(argv[2], "read") == 0)
    {
      value = read_gpio(pin);
      printf("GPIO %s value: %d (%s)\n", argv[1], value, value ? "HIGH" : "LOW");
    }
  else if (strcasecmp(argv[2], "toggle") == 0)
    {
      value = read_gpio(pin);
      set_gpio(pin, !value);
      printf("GPIO %s toggled: %d -> %d\n", argv[1], value, !value);
    }
  else
    {
      printf("Error: Unknown command '%s'\n", argv[2]);
      printf("Use '%s help' for usage.\n", argv[0]);
      return -1;
    }
  
  return 0;
}
