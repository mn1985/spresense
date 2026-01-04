/****************************************************************************
 * examples/usb_cam_serial/usb_cam_serial_main.c
 *
 * USB Camera Serial Transfer with Stepper Motor Control
 * 
 * This application captures images from a USB camera connected to Spresense
 * and transfers them via serial port to a PC. It also controls a 28BYJ-48
 * stepper motor to rotate the camera for automated scanning.
 *
 * Author: T.Mano
 * Created: 2025.12.17
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <nuttx/video/video.h>
#include <malloc.h>
#include <termios.h>
#include <unistd.h>
#include <arch/board/board.h>
#include <arch/chip/pin.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VIDEO_DEV_PATH "/dev/video"
#define CAPTURE_WIDTH  VIDEO_HSIZE_QUADVGA
#define CAPTURE_HEIGHT VIDEO_VSIZE_QUADVGA
#define STILL_BUFNUM   (1)
#define JPEG_QUALITY   (70)
#define FRAMEBUFFER_SIZE (512 * 1024)  /* 512KB for JPEG (Quad VGA typical) */


/* Stepper Motor Definitions - Using UART2 pins as GPIO on main board */
#define STEPPER_IN1  PIN_UART2_TXD      /* Pin 67 on main board B2B connector */
#define STEPPER_IN2  PIN_UART2_RXD      /* Pin 68 on main board B2B connector */
#define STEPPER_IN3  PIN_UART2_CTS      /* Pin 69 on main board B2B connector */
#define STEPPER_IN4  PIN_UART2_RTS      /* Pin 70 on main board B2B connector */

#define STEPS_PER_REV     4096    /* 28BYJ-48: 64 steps * 64 gear ratio */
#define STEPS_PER_CAPTURE 256     /* Capture every 256 steps */
#define MAX_ANGLE_STEPS   2048    /* ±180 degrees = ±2048 steps */
#define STEP_DELAY_US     1000    /* 1ms delay between steps */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Stepper Motor Control Functions */

/* Half-step sequence for 28BYJ-48 (8 steps per cycle) */
static const int step_sequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

static void stepper_set_pins(int in1, int in2, int in3, int in4)
{
  board_gpio_write(STEPPER_IN1, in1);
  board_gpio_write(STEPPER_IN2, in2);
  board_gpio_write(STEPPER_IN3, in3);
  board_gpio_write(STEPPER_IN4, in4);
}

static void stepper_init(void)
{
  /* Configure GPIO pins as outputs */
  board_gpio_config(STEPPER_IN1, 0, false, true, PIN_FLOAT);
  board_gpio_config(STEPPER_IN2, 0, false, true, PIN_FLOAT);
  board_gpio_config(STEPPER_IN3, 0, false, true, PIN_FLOAT);
  board_gpio_config(STEPPER_IN4, 0, false, true, PIN_FLOAT);
  
  fprintf(stderr, "Stepper GPIO configured (pins: %d, %d, %d, %d)\n",
          STEPPER_IN1, STEPPER_IN2, STEPPER_IN3, STEPPER_IN4);
  
  /* Initialize all pins to LOW */
  stepper_set_pins(0, 0, 0, 0);
}

static void stepper_step(int step_num, bool forward)
{
  int seq_index = step_num % 8;
  if (!forward)
    {
      seq_index = 7 - seq_index;
    }
  
  stepper_set_pins(
    step_sequence[seq_index][0],
    step_sequence[seq_index][1],
    step_sequence[seq_index][2],
    step_sequence[seq_index][3]
  );
  
  usleep(STEP_DELAY_US);
}

static void stepper_move(int steps, bool forward)
{
  int i;
  fprintf(stderr, "  Motor: moving %d steps %s\n", steps, forward ? "forward" : "backward");
  for (i = 0; i < steps; i++)
    {
      stepper_step(i, forward);
    }
  
  /* Turn off all coils to save power */
  stepper_set_pins(0, 0, 0, 0);
}

/* Camera Control Functions */

static int get_cam_fd(void)
{
  int fd;
  int ret;

  ret = video_initialize(VIDEO_DEV_PATH);
  if (ret != 0)
    {
      fprintf(stderr, "ERROR: video_initialize failed: %d\n", ret);
      return ret;
    }

  fd = open(VIDEO_DEV_PATH, 0);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open %s failed: %d\n", VIDEO_DEV_PATH, errno);
      video_uninitialize(VIDEO_DEV_PATH);
      return -errno;
    }

  return fd;
}

static void close_cam_fd(int fd)
{
  close(fd);
  video_uninitialize(VIDEO_DEV_PATH);
}

static int prepare_capture(int fd)
{
  int ret;
  struct v4l2_requestbuffers req;
  struct v4l2_format fmt;
  struct v4l2_ext_controls ctrls;
  struct v4l2_ext_control ctrl;

  /* Request buffers */
  req.type   = V4L2_BUF_TYPE_STILL_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;
  req.count  = STILL_BUFNUM;
  req.mode   = V4L2_BUF_MODE_FIFO;

  ret = ioctl(fd, VIDIOC_REQBUFS, (unsigned long)&req);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: VIDIOC_REQBUFS failed: %d\n", errno);
      return ret;
    }

  /* Set format */
  fmt.type                = V4L2_BUF_TYPE_STILL_CAPTURE;
  fmt.fmt.pix.width       = CAPTURE_WIDTH;
  fmt.fmt.pix.height      = CAPTURE_HEIGHT;
  fmt.fmt.pix.field       = V4L2_FIELD_ANY;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;

  ret = ioctl(fd, VIDIOC_S_FMT, (unsigned long)&fmt);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: VIDIOC_S_FMT failed: %d\n", errno);
      return ret;
    }

  /* Set JPEG quality (optional) */
  memset(&ctrls, 0, sizeof(ctrls));
  memset(&ctrl, 0, sizeof(ctrl));

  ctrls.ctrl_class = V4L2_CTRL_CLASS_JPEG;
  ctrls.count      = 1;
  ctrls.controls   = &ctrl;

  ctrl.id    = V4L2_CID_JPEG_COMPRESSION_QUALITY;
  ctrl.value = JPEG_QUALITY;

  ret = ioctl(fd, VIDIOC_S_EXT_CTRLS, (unsigned long)&ctrls);
  if (ret < 0)
    {
      fprintf(stderr, "Note: Failed to set JPEG quality (optional), errno: %d\n", errno);
    }

  return 0;
}

static int take_picture(int fd, uint8_t *buf, size_t buf_size, size_t *captured_size)
{
  int ret;
  struct v4l2_buffer vbuf;

  /* Enqueue buffer */
  memset(&vbuf, 0, sizeof(vbuf));
  vbuf.type = V4L2_BUF_TYPE_STILL_CAPTURE;
  vbuf.memory = V4L2_MEMORY_USERPTR;
  vbuf.index = 0;
  vbuf.m.userptr = (unsigned long)buf;
  vbuf.length = buf_size;

  ret = ioctl(fd, VIDIOC_QBUF, (unsigned long)&vbuf);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: VIDIOC_QBUF failed: %d\n", errno);
      return ret;
    }

  /* Start capture */
  ret = ioctl(fd, VIDIOC_TAKEPICT_START, 0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: VIDIOC_TAKEPICT_START failed: %d\n", errno);
      return ret;
    }

  /* Dequeue buffer (wait for capture done) */
  ret = ioctl(fd, VIDIOC_DQBUF, (unsigned long)&vbuf);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: VIDIOC_DQBUF failed: %d\n", errno);
      ioctl(fd, VIDIOC_TAKEPICT_STOP, 0);
      return ret;
    }

  *captured_size = vbuf.bytesused;

  /* Stop capture */
  ioctl(fd, VIDIOC_TAKEPICT_STOP, 0);

  return 0;
}
/*
static void send_image_data(uint8_t *buf, size_t size)
{
  uint8_t marker[4] = {0xBE, 0xEF, 0xCA, 0xFE};
  fwrite(marker, 1, sizeof(marker), stdout);

  // Send size (4 bytes, Little Endian)
  uint32_t sz = (uint32_t)size;
  fwrite(&sz, 1, 4, stdout);
  fflush(stdout);

  // Send data
  fwrite(buf, 1, size, stdout);
  fflush(stdout);
}
*/
static void send_image_data(uint8_t *buf, size_t size)
{
    static bool initialized = false;
    static uint8_t stdout_buf[16 * 1024];

    if (!initialized) {
        setvbuf(stdout, (char*)stdout_buf, _IOFBF, sizeof(stdout_buf));
        initialized = true;
    }

    const uint8_t marker[4] = {0xBE, 0xEF, 0xCA, 0xFE};
    uint32_t sz = (uint32_t)size;

    fwrite(marker, 1, 4, stdout);
    fwrite(&sz, 1, 4, stdout);
    fwrite(buf, 1, size, stdout);

    fflush(stdout);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  uint8_t *framebuffer;
  size_t fb_size = FRAMEBUFFER_SIZE;
  size_t captured_size;
  char cmd;
  int i;
  int num_positions;

  fprintf(stderr, "USB Camera + Stepper Motor Control\n");
  fprintf(stderr, "Commands:\n");
  fprintf(stderr, "  's' - Start rotation and capture sequence\n");
  fprintf(stderr, "  'c' - Single capture (no motor movement)\n");
  fprintf(stderr, "  'q' - Quit\n");
  fprintf(stderr, "\nMotor Configuration:\n");
  fprintf(stderr, "  Steps per revolution: %d\n", STEPS_PER_REV);
  fprintf(stderr, "  Steps per capture: %d\n", STEPS_PER_CAPTURE);
  fprintf(stderr, "  Max angle: ±180 degrees (±%d steps)\n", MAX_ANGLE_STEPS);
  fprintf(stderr, "  Total positions: %d\n\n", (MAX_ANGLE_STEPS * 2) / STEPS_PER_CAPTURE);

  /* Allocate framebuffer */
  framebuffer = (uint8_t *)memalign(32, fb_size);
  if (!framebuffer)
    {
      fprintf(stderr, "ERROR: Failed to allocate framebuffer\n");
      return -1;
    }

  /* Initialize Stepper Motor */
  fprintf(stderr, "Initializing stepper motor...\n");
  stepper_init();

  /* Initialize Camera */
  fprintf(stderr, "Initializing camera...\n");
  fd = get_cam_fd();
  if (fd < 0)
    {
      free(framebuffer);
      return -1;
    }

  ret = prepare_capture(fd);
  if (ret < 0)
    {
      close_cam_fd(fd);
      free(framebuffer);
      return -1;
    }

  fprintf(stderr, "Ready!\n");

  /* Main Loop */
  struct termios saveterm;
  struct termios term;

  /* Set raw mode */
  tcgetattr(0, &term);
  memcpy(&saveterm, &term, sizeof(struct termios));
  cfmakeraw(&term);
  tcsetattr(0, TCSANOW, &term);

  while (1)
    {
      /* Read command from stdin */
      cmd = getchar();

      if (cmd == 's')
        {
          /* Start rotation and capture sequence */
          fprintf(stderr, "\n=== Starting Capture Sequence ===\n");
          
          num_positions = (MAX_ANGLE_STEPS * 2) / STEPS_PER_CAPTURE;
          
          /* Phase 1: 0° to +180° */
          fprintf(stderr, "Phase 1: Rotating 0° to +180°\n");
          for (i = 0; i <= MAX_ANGLE_STEPS / STEPS_PER_CAPTURE; i++)
            {
              fprintf(stderr, "Position %d/%d (angle: +%.1f°)\n", 
                     i + 1, num_positions,
                     (float)(i * STEPS_PER_CAPTURE) * 360.0 / STEPS_PER_REV);
              
              /* Capture image */
              ret = take_picture(fd, framebuffer, fb_size, &captured_size);
              if (ret == 0)
                {
                  send_image_data(framebuffer, captured_size);
                  fprintf(stderr, "  Image sent (%zu bytes)\n", captured_size);
                }
              else
                {
                  fprintf(stderr, "  ERROR: Capture failed\n");
                }
              
              /* Move to next position (except last) */
              if (i < MAX_ANGLE_STEPS / STEPS_PER_CAPTURE)
                {
                  stepper_move(STEPS_PER_CAPTURE, true);
                }
              
              usleep(100000); /* 100ms delay between captures */
            }
          
          /* Phase 2: +180° to -180° */
          fprintf(stderr, "\nPhase 2: Rotating +180° to -180°\n");
          for (i = 0; i < (MAX_ANGLE_STEPS * 2) / STEPS_PER_CAPTURE; i++)
            {
              /* Move first */
              stepper_move(STEPS_PER_CAPTURE, false);
              
              int angle_steps = MAX_ANGLE_STEPS - (i + 1) * STEPS_PER_CAPTURE;
              fprintf(stderr, "Position %d/%d (angle: %+.1f°)\n",
                     (MAX_ANGLE_STEPS / STEPS_PER_CAPTURE) + i + 2,
                     num_positions,
                     (float)angle_steps * 360.0 / STEPS_PER_REV);
              
              /* Capture image */
              ret = take_picture(fd, framebuffer, fb_size, &captured_size);
              if (ret == 0)
                {
                  send_image_data(framebuffer, captured_size);
                  fprintf(stderr, "  Image sent (%zu bytes)\n", captured_size);
                }
              else
                {
                  fprintf(stderr, "  ERROR: Capture failed\n");
                }
              
              usleep(100000); /* 100ms delay between captures */
            }
          
          /* Phase 3: Return to 0° */
          fprintf(stderr, "\nPhase 3: Returning to 0°\n");
          stepper_move(MAX_ANGLE_STEPS, true);
          fprintf(stderr, "Returned to home position (0°)\n");
          
          fprintf(stderr, "\n=== Sequence Complete ===\n");
          fprintf(stderr, "Total images captured: %d\n\n", num_positions);
        }
      else if (cmd == 'c')
        {
          /* Single capture without motor movement */
          fprintf(stderr, "Capturing single image...\n");
          ret = take_picture(fd, framebuffer, fb_size, &captured_size);
          if (ret == 0)
            {
              send_image_data(framebuffer, captured_size);
              fprintf(stderr, "Image sent (%zu bytes)\n", captured_size);
            }
          else
            {
              fprintf(stderr, "ERROR: Capture failed\n");
            }
        }
      else if (cmd == 'q')
        {
          break;
        }
    }

  /* Finalize */
  fprintf(stderr, "\nShutting down...\n");
  tcsetattr(0, TCSANOW, &saveterm);
  close_cam_fd(fd);
  free(framebuffer);

  fprintf(stderr, "Exit.\n");
  return 0;
}
