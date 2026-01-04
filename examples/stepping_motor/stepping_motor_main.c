/****************************************************************************
 * examples/stepping_motor/stepping_motor_main.c (Client)
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FIFO_PATH "/dev/stepper0"

struct stepper_cmd
{
  int steps;
  int delay_us;
  int direction;
};

int stepping_motor_main(int argc, char *argv[])
{
  struct stepper_cmd cmd;
  int fd;
  int wlen;

  if (argc < 4)
    {
      printf("Usage: stepping_motor <steps> <delay_us> <dir>\n");
      return -1;
    }

  cmd.steps     = atoi(argv[1]);
  cmd.delay_us  = atoi(argv[2]);
  cmd.direction = atoi(argv[3]);

  fd = open(FIFO_PATH, O_WRONLY);
  if (fd < 0)
    {
      perror("open fifo (Is stepperd running?)");
      return -1;
    }

  wlen = write(fd, &cmd, sizeof(cmd));
  if (wlen != sizeof(cmd))
    {
      perror("write failed");
    }

  close(fd);

  return 0;
}
