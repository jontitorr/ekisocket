#pragma once
#include <cstdint>
#ifdef _WIN32
#include <WinSock2.h>
#define socketerrno WSAGetLastError()
using socket_t = SOCKET;
using socklen_t = int;
#define poll WSAPoll
#else
#include <csignal>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#include <cerrno>
#define socketerrno errno
using socket_t = int;
#endif
