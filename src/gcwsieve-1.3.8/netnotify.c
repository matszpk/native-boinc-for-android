/*
 * netnotify.c
 * Author: Mateusz Szpakowski
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/time.h>
#include "netnotify.h"

void netnotify_wait()
{
  struct addrinfo* addrinfo;
  struct addrinfo hints;
  struct sigaction sa_new, sa_old;
  
  int status = 0;
  fd_set rfds;
  
  int serverfd = 0;
  
  sa_new.sa_handler = SIG_IGN;
  sigemptyset(&sa_new.sa_mask);
  sa_new.sa_flags = 0;
  sigaction(SIGINT, &sa_new, &sa_old);
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  
  
  status = getaddrinfo(NULL, "17900", &hints, &addrinfo);
  if (status != 0)
  {
    fputs("Cant get addr info", stderr);
    return;
  }
  
  serverfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (serverfd == -1)
  {
    fputs("Cant open socket", stderr);
    freeaddrinfo(addrinfo);
    return;
  }
  
  status = bind(serverfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (status != -1)
    status = listen(serverfd, 10);
  
  if (status == -1)
  {
    fputs("Cant initialize", stderr);
    close(serverfd);
    freeaddrinfo(addrinfo);
    return;
  }
  
  FD_ZERO(&rfds);
  FD_SET(serverfd, &rfds);
  // waiting
  select(serverfd+1, &rfds, NULL, NULL, NULL);
  
  close(serverfd);
  freeaddrinfo(addrinfo);
}
