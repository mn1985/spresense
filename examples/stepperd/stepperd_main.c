/****************************************************************************
 * examples/stepping_motor/stepperd_main.c
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <arch/board/board.h>
#include <arch/chip/pin.h>
#include <nuttx/arch.h>

/* DRV8835 pin mapping */
#define GPIO_AIN1  PIN_UART2_TXD
#define GPIO_AIN2  PIN_UART2_RXD
#define GPIO_BIN1  PIN_UART2_CTS
#define GPIO_BIN2  PIN_UART2_RTS

#define FIFO_PATH  "/dev/stepper0"

/* half-step sequence */
static const uint8_t g_seq[8][4] =
{
  {1,0,0,0}, {1,0,1,0}, {0,0,1,0}, {0,1,1,0},
  {0,1,0,0}, {0,1,0,1}, {0,0,0,1}, {1,0,0,1}
};

struct stepper_cmd
{
  int steps;
  int delay_us;
  int direction; /* 1 or -1 */
};

static int g_step_idx = 0;

/*--------------------------------------------------------------------------*/

static void write_step(int idx)
{
  board_gpio_write(GPIO_AIN1, g_seq[idx][0]);
  board_gpio_write(GPIO_AIN2, g_seq[idx][1]);
  board_gpio_write(GPIO_BIN1, g_seq[idx][2]);
  board_gpio_write(GPIO_BIN2, g_seq[idx][3]);
}

static void init_gpio(void)
{
  board_gpio_config(GPIO_AIN1, 0, false, true, PIN_FLOAT);
  board_gpio_config(GPIO_AIN2, 0, false, true, PIN_FLOAT);
  board_gpio_config(GPIO_BIN1, 0, false, true, PIN_FLOAT);
  board_gpio_config(GPIO_BIN2, 0, false, true, PIN_FLOAT);
}

/* ★重要：一意な初期位置を作る */
static void force_initial_alignment(void)
{
  /* A+ B+ (Index 1 in sequence) */
  /* g_seq[1] = {1,0,1,0} */
  board_gpio_write(GPIO_AIN1, 1);
  board_gpio_write(GPIO_AIN2, 0);
  board_gpio_write(GPIO_BIN1, 1);
  board_gpio_write(GPIO_BIN2, 0);

  up_udelay(10000); /* 10ms */
  g_step_idx = 1;
}

static void step_motor(const struct stepper_cmd *cmd)
{
  printf("stepperd: Move %d steps, delay %d, dir %d\n", cmd->steps, cmd->delay_us, cmd->direction);
  
  for (int i = 0; i < cmd->steps; i++)
    {
      if (cmd->direction > 0)
        g_step_idx = (g_step_idx + 1) & 7;
      else
        g_step_idx = (g_step_idx + 7) & 7;

      write_step(g_step_idx);
      up_udelay(cmd->delay_us);
    }
  printf("stepperd: Done\n");
}

/*--------------------------------------------------------------------------*/

int stepperd_main(int argc, char *argv[])
{
  int fd;
  struct stepper_cmd cmd;
  int ret;

  printf("stepperd starting...\n");

  init_gpio();
  force_initial_alignment();

  /* Create FIFO */
  ret = mkfifo(FIFO_PATH, 0666);
  if (ret < 0 && errno != EEXIST)
    {
      perror("mkfifo");
      return -1;
    }

  fd = open(FIFO_PATH, O_RDONLY);
  if (fd < 0)
    {
      perror("open fifo");
      return -1;
    }

  while (1)
    {
      /* Read blocks until data is available */
      ssize_t r = read(fd, &cmd, sizeof(cmd));
      if (r == sizeof(cmd))
        {
          step_motor(&cmd);
        }
      else if (r == 0)
        {
          /* EOF (End of Writer) - Reopen to wait for next client */
          close(fd);
          fd = open(FIFO_PATH, O_RDONLY);
        }
      else if (r < 0)
        {
          perror("read error");
          usleep(100000);
        }
    }

  return 0;
}
