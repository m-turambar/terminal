#ifndef REDES_H
#define REDES_H

#include <queue>
#include <mutex>

#include "asio.hpp"

using namespace asio::ip;

extern asio::io_service iosvc;

class cliente
{
public:
  cliente(asio::io_service& io_service, std::string huesped, std::string puerto) :
    iosvc_(io_service),
    socket_(io_service)
  {
    tcp::resolver resolvedor(io_service);
    tcp::resolver::query query(huesped, puerto); //puedes meter dns aqui
    tcp::resolver::iterator iter = resolvedor.resolve(query);
    endpoint_ = iter->endpoint();
    conectar();
    timer_queue(); //podemos hacerlo como un sistema operativo
  }

private:
  void conectar();
  void leer();
  void procesar_lectura();
  void escribir(std::string str);

  asio::io_service& iosvc_;
  tcp::socket socket_;
  tcp::endpoint endpoint_;
  enum { sz_buf = 128 };
  char rx_buf_[sz_buf];
  std::string tx_buf_;
};

void redes_main();


#endif // REDES_H
