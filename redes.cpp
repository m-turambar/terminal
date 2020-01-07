#include <iostream>
#include <future>
#include <fstream>

using namespace std;
using namespace asio;

io_service iosvc;

void cliente::conectar()
{
  socket_.async_connect(endpoint_,
    [this](error_code ec)
  {
    if(!ec)
    {
      cout << "conectado a " << socket_.remote_endpoint().address().to_string()
          << ":" << socket_.remote_endpoint().port() << '\n';
      //escribir("mike;ftw;");
      //escribir("version " + version);
      leer();
    }
    else
    {
	  cout << "Error conectando: " << ec.value() <<  ": " <<  ec.message() << '\n';
	  /*Puede ser un error 10061: Equipo destino denegó expresamente dicha conexión */
	  if(ec.value() == 10061) //levantar dialogo de seleccion de ip y puertos? esto es un buen comentario
	  {
		
	  }

	  /*Error 10056: Se solicitó conexión en socket ya conectado*/
	  else if(ec.value() == 10056)
	  {
		std::error_code ec_cerrar;
		socket_.close(ec_cerrar);
		if(!ec_cerrar)
		  conectar();
		else
		  std::cout << "Error cerrando y conectando: " << ec_cerrar.value() <<  ": " <<  ec_cerrar.message() << std::endl;
	  }
       
    }
  });
} //conectar

void cliente::leer()
{
  socket_.async_read_some(asio::buffer(rx_buf_, sz_buf),
    [this](std::error_code ec, std::size_t bytes_leidos)
  {
    if(!ec)
    {
      procesar_lectura();
    }
    else
    {
      std::cout << "Error leyendo " << ec.value() <<  ": " << ec.message() << std::endl;
      if(ec.value() == 10054) /*Error 10054: Interrupción forzada por host remoto*/
      {
        Sleep(1000);
        conectar();
      }
    }
  });
}

void cliente::procesar_lectura()
{
  string lectura = rx_buf_;
  cout << rx_buf_ << '\n';
  
  memset(rx_buf_, '\0', sz_buf);
  leer();
}


void cliente::escribir(std::string str)
{
  tx_buf_ = str;
  asio::async_write(socket_, asio::buffer(tx_buf_.data(), tx_buf_.size()),
    [this](std::error_code ec, std::size_t len)
  {
    if (!ec)
    {
      //cout << "Se escribio " << tx_buf_ << " con " << len << " bytes" << endl;
      /*procesar escritura exitosa*/
    }
  });
}

void redes_main()
{
  try
  {
    cliente tu_cliente(iosvc, "192.168.1.100", 3214);

    iosvc.run();
    std::cout << "Saliendo de ciclo de iosvc\n";
  }
  catch (std::exception& e)
  {
    std::cerr << "Excepcion en redes_main: " << e.what() << "\n";
  }

}