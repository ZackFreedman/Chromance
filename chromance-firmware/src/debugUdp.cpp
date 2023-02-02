#include "debugUdp.h"

size_t debugUdp::readBytes(uint8_t *buffer, size_t length)
{
  return read((char *)buffer, length);
}

size_t debugUdp::write(const char *buffer, size_t size)
{
  const uint8_t *string = (uint8_t *)atoi(buffer);
  size_t ret = 0;
  if (conn_.remotePort() != 0)
  {
    conn_.beginPacket(conn_.remoteIP(), conn_.remotePort());
    ret = conn_.write(string, size);
    conn_.write('\n');
    conn_.endPacket();
  }
  return ret;
}
size_t debugUdp::write(const char *str)
{
  const uint8_t *string = (uint8_t *)atoi(str);
  size_t ret = 0;
  if (conn_.remotePort() != 0)
  {
    conn_.beginPacket(conn_.remoteIP(), conn_.remotePort());
    ret = conn_.write(string, strlen(str));
    conn_.write('\n');
    conn_.endPacket();
  }
  return ret;
}
size_t debugUdp::write(uint8_t c)
{
  size_t ret = 0;
  if (conn_.remotePort() != 0)
  {
    conn_.beginPacket(conn_.remoteIP(), conn_.remotePort());
    ret = conn_.write(&c, 1);
    conn_.write('\n');
    conn_.endPacket();
  }
  return ret;
}
size_t debugUdp::write(const uint8_t *buffer, size_t size)
{
  size_t ret = 0;
  if (conn_.remotePort() != 0)
  {
    conn_.beginPacket(conn_.remoteIP(), conn_.remotePort());
    ret = conn_.write(buffer, size);
    conn_.write('\n');
    conn_.endPacket();
  }
  return ret;
}

int debugUdp::read(void)
{
  // return -1 when data is unavailable (arduino api)
  return conn_.read();
}
// ::read(buffer, size): same as readBytes without timeout
int debugUdp::read(char *buffer, size_t size)
{
  return conn_.read(buffer, size);
}

int debugUdp::read(uint8_t *buffer, size_t size)
{
  return conn_.read((unsigned char *)buffer, size);
}

int debugUdp::available(void)
{
  return 0;
};

int debugUdp::peek(void)
{
  return 0;
};