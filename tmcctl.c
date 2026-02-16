#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define PROGRAM_NAME "tmcctl"
#define VERSION "0.0.1"

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
  puts("Usage: ./tmcctl [--help/--version]... [-p FILENAME -s TEMP]");
  puts("Examples:");
  puts("  ./tmcctl -p test.txt -s 46");
}

int
main(int argc, char *argv[]) 
{
  int opt;
  char *port_name = NULL;
  int target_temp = -1;
  static struct option long_options[] = {
    {"version", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"port", required_argument, 0, 'p'},
    {"set", required_argument, 0, 's'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "vhp:s:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'v':
        print_version();
        return 0;
      case 'h':
        print_help();
        return 0;
      case 'p':
        port_name = optarg;
        break;
      case 's':
        target_temp = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Try '--help' for more information.\n");
        exit(EXIT_FAILURE);
    }
  }
  /* --- */
  if (port_name == NULL) 
  {
    fprintf(stderr, "Error: Port not specified.\n");
    return EINVAL;
  }
  if (target_temp == -1) 
  {
    fprintf(stderr, "Error: Port not specified.\n");
    return EINVAL;
  }
  /* --- */
  int fd = open(port_name, O_RDWR | O_NOCTTY);
  if (fd < 0) 
  {
    perror("Port opening error");
    return errno;
  }

  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(fd, &tty) != 0) 
  {
    perror("tcgetattr error");
    close(fd);
    return errno;
  }
  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_oflag &= ~OPOST;
  if (tcsetattr(fd, TCSANOW, &tty) != 0) 
  {
    perror("tcsetattr error");
    close(fd);
    return errno;
  }

  /* --- Отправка команды --- */
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "set %d\n", target_temp);
  if (write(fd, buf, len) < 0) 
  {
    perror("Write error");
  } 
  else 
  {
    printf("Successfully sent: %s", buf);
  }

  close(fd);
  return 0;
}