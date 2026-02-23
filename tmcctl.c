#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/file.h>
#include <time.h>

#define PROGRAM_NAME "tmcctl"
#define VERSION "0.1.0"

#define VERSION_FLAG 'v'
#define HELP_FLAG 'h'
#define SPEED_FLAG 's'
#define PORT_FLAG 'p'
#define SET_TEMPERATURE_FLAG 't'
#define DEBUG_FLAG 'd'
#define VMIN_FLAG 'm'
#define VTIME_FLAG 'i'
#define TTY_OPEN_WAIT_FLAG 'w'

int 
serial_read_line(int fd, char *buf, size_t buf_size, int timeout_sec) 
{
  size_t total = 0;
  time_t start = time(NULL);
  while (total < buf_size - 1) 
  {
    if (time(NULL) - start >= timeout_sec) 
    {
      break;
    }
    char c;
    int n = read(fd, &c, 1);
    if (n > 0) 
    {
      buf[total++] = c;
      if (c == '\n') 
      {
        break;
      }
      } 
      else if (n == 0) 
      {
        continue;
      } 
      else 
    {
      perror("read");
      return -1;
    }
  }
  buf[total] = '\0';
  return (int)total;
}

void
print_version(void) 
{
  printf("%s %s\n", PROGRAM_NAME, VERSION);
  puts("Copyright (C) 2026 Michael");
  puts("License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.");
  puts("This is free software: you are free to change and redistribute it.");
  puts("There is NO WARRANTY, to the extent permitted by law.\n");
  puts("Written by Michael.");
}

void
print_help(void) 
{
  puts("Usage: ./tmcctl [--help/--version]... [-p FILENAME -t TEMP -s BAUDRATE -t WAIT_TIME -m NUM_OF_RECEIVED_PACKAGES]");
  puts("Examples:");
  puts("\t./tmcctl -p /dev/ttyACM0 -s 1200 -t 150 -m 10 -i 10 -d");
  puts("\t./tmcctl -p /dev/ttyUSB0 -s 115200 -t 85");
}

int
main(int argc, char *argv[]) 
{
  /* global for main structure objects */
  char *port_name = NULL;
  char *endptr = NULL;
  int target_temp = -1;
  int using_speed = -1;
  speed_t baudrate;
  int vmin_arg = 0;
  int vtime_arg = 0;
  bool debug_state = false;
  bool tty_open_wait = false;
  int opt;
  static struct option long_options[] = 
	{
    {"version", no_argument, 0, VERSION_FLAG},
    {"help", no_argument, 0, HELP_FLAG},
    {"debug", no_argument, 0, DEBUG_FLAG},
    {"flock", no_argument, 0, TTY_OPEN_WAIT_FLAG},
    {"speed", required_argument, 0, SPEED_FLAG},
    {"port", required_argument, 0, PORT_FLAG},
    {"set", required_argument, 0, SET_TEMPERATURE_FLAG},
    {"vmin", required_argument, 0, VMIN_FLAG},
    {"vtime", required_argument, 0, VTIME_FLAG},
    {0, 0, 0, 0}
  };

  /* checking argument availability */
  while ((opt = getopt_long(argc, argv, "vhdwm:i:p:s:t:", long_options, NULL)) != -1) 
  {
    switch (opt) 
    {
      case VERSION_FLAG: 
        print_version();
        return 0;
      case HELP_FLAG: 
        print_help();
        return 0;
      case PORT_FLAG: 
        port_name = optarg;
        break;
      case SET_TEMPERATURE_FLAG:
        endptr = NULL; 
        target_temp = (int)strtoul(optarg, &endptr, 10);
        if (*endptr != '\0') 
        {
          fprintf(stderr, "Error: Incorrect target temperature value.\n");
          return EINVAL;
        }
        break;
      case SPEED_FLAG:
        endptr = NULL;
        using_speed = (int)strtoul(optarg, &endptr, 10);
        if (*endptr != '\0') 
        {
          fprintf(stderr, "Error: Incorrect target temperature value.\n");
          return EINVAL;
        }
        switch (using_speed) 
        {
          case 1200: baudrate = B1200; break;
          case 4800: baudrate = B4800; break;
          case 9600: baudrate = B9600; break;
          case 19200: baudrate = B19200; break;
          case 38400: baudrate = B38400; break;
          case 57600: baudrate = B57600; break;
          case 115200: baudrate = B115200; break;
          default:
            fprintf(stderr, "Error: Incorrect baudrate value.\n");
            return EINVAL;
        }
				break;
      case DEBUG_FLAG:
        debug_state = true;
        break;
      case VMIN_FLAG:
        endptr = NULL;
        vmin_arg = (int)strtoul(optarg, &endptr, 10);
        if (*endptr != '\0') 
        {
          fprintf(stderr, "Error: Incorrect VMIN value.\n");
          return EINVAL;
        }
        break;
      case VTIME_FLAG:
        endptr = NULL;
        vtime_arg = (int)strtoul(optarg, &endptr, 10);
        if (*endptr != '\0') 
        {
          fprintf(stderr, "Error: Incorrect VTIME value.\n");
          return EINVAL;
        }
        break;
      case TTY_OPEN_WAIT_FLAG:
        tty_open_wait = true;
        break;
      default:
        fprintf(stderr, "Try '--help' for more information.\n");
        exit(EXIT_FAILURE);
    }
  }

  /* checking argument values */
  if (port_name == NULL) 
  {
    fprintf(stderr, "Error: Port not specified.\n");
    return EINVAL;
  }
  if (target_temp == -1) 
  {
    fprintf(stderr, "Error: Target temperature not specified.\n");
    return EINVAL;
  }
  if (using_speed == -1)
  {
    fprintf(stderr, "Error: The communication speed with the device is not specified.\n");
    return EINVAL;
  }
  if (debug_state == true)
  {
    printf("%d\n", argc);
    printf("%s\n", port_name);
    printf("%d\n", target_temp);
    printf("%d\n", using_speed);
  }

  /* --- */

  int fd = open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) 
  {
    perror("Port opening error");
    return errno;
  }
  if (tty_open_wait == true) 
  {
    if (flock(fd, LOCK_EX) < 0) 
    {
      fprintf(stderr, "Error: The file is unavailable or is being used by another process. Please wait...\n");
    }
  } else 
  {
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) 
    {
      fprintf(stderr, "Error: The file is unavailable or is being used by another process.\n");
      return EBUSY;
    }
  }

  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(fd, &tty) != 0) 
  {
    perror("tcgetattr error");
    close(fd);
    return errno;
  }
  cfsetospeed(&tty, baudrate);
  cfsetispeed(&tty, baudrate);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_oflag &= ~OPOST;
  tty.c_cc[VMIN] = vmin_arg;
  tty.c_cc[VTIME] = vtime_arg;
  if (tcsetattr(fd, TCSANOW, &tty) != 0) 
  {
    perror("tcsetattr error");
    close(fd);
    return errno;
  }

  /* transmitting the command */

  char buf[32];
  int len = snprintf(buf, sizeof(buf), "set %d\n", target_temp);
  tcflush(fd, TCOFLUSH);
  if (write(fd, buf, len) < 0) 
  {
    perror("Write error");
    return ECANCELED;
  } 
  else 
  {
    tcdrain(fd);
    printf("Successfully sent: %s", buf);
  }

  /* read the response */
  char response[256];
  int n = serial_read_line(fd, response, sizeof(response), 3);
  if (n > 0) 
  {
    printf("%s", response);
  } 
  else 
  {
    printf("There was no response from the device.\n");
  }

  close(fd);

  return 0;
}
