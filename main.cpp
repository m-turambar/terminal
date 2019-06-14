#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <functional>
#include <iomanip> //setw(2)

#define ASIO_STANDALONE
#include "asio.hpp"

#include <windows.h>

using namespace std;

template<class T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& v)
{
	os << "{";
	for(auto& e : v)
		os << e << " ";
	os << "}";
	return os;
}

template<typename T, const size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T,N> a)
{
	os << '[';
	for(auto& e : a)
		os << e << " ";
	os << ']';
	return os;
}

struct consola
{
	consola(asio::io_service& srv_, string pCOM_, int baudios_);
	void init();
	void leer_pser();

	asio::io_service& srv;
	string pCOM;
	int baudios;
	
	//asio::streambuf input_buffer;
	//asio::windows::stream_handle input_handle;
};

consola::consola(asio::io_service& srv_, string pCOM_, int baudios_):
srv(srv_), pCOM(pCOM_), baudios(baudios_)
{
	/*input handle y buffer*/

}

char toBinary(char c1, char c2)
{
	char r1, r2;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 5);
	
	
	if(c1 > '9')
		r1 = (toupper(c1) - 55)*16;
	else
		r1 = (c1-48)*16;
	
	if(c2 > '9')
		r2 = (toupper(c2) - 55);
	else
		r2 = (c2-48);
	
	printf("%c%c -> %d+%d ", c1, c2, r1, r2);	
	
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	return r1 + r2;
}

vector<char> hexAsciiToBinary(vector<char> vasc)
{
	vector<char> res;

	if(vasc.size() % 2 != 0)
	{
		cerr << "ERROR: Invalid hex string length: Odd\n";
		exit(-1);
	}
	
	//we get our byte value from the 2 character representation
	for(int i=0; i < vasc.size(); i+=2)
	{
		res.push_back( toBinary(vasc[i], vasc[i+1]) );
	}

	return res;
}

size_t crc(vector<char> vuns)
{
	size_t checksum=0;
	for(auto i : vuns)
		checksum = 0xFF&(checksum + i);
	checksum = ~checksum;

	return 0xFF&(checksum+1);
}

/* prints as green hex and return to normal after */
void print_as_hex(char* buf, size_t sz)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	for(int i=0; i<sz; ++i)
	{
		//otherwise we'd get e.g. 010 instead of 0010.
		printf("%02x", (unsigned char)buf[i]);
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void do_read(asio::serial_port& sp, char* rxbuf, size_t RXBUF_SZ)
{
	asio::async_read(sp, asio::buffer(rxbuf, RXBUF_SZ), 
			[rxbuf, &sp, RXBUF_SZ](asio::error_code ec, size_t len)
		{
			print_as_hex(rxbuf, len);
			do_read(sp, rxbuf, RXBUF_SZ);
		});
}

/*Parses the incoming command from the cmd*/
std::string parse_line(std::string line)
{
	if(line.size() <= 2 || line.size()%2 != 0)
		return line;
	else if(line[0] == '0' && line[1] == 'x')
	{
		std::string shex;
		for(int i=2; i<line.size(); i+=2)
		{
			shex += toBinary(line[i], line[i+1]);
		}
		return shex;
	}
	else
		return line;
}

int main(int argc, char** argv)
{
	//el ultimo argumento es la cadena
	vector<char> vasc(argv[argc-1], argv[argc-1] + strlen(argv[argc-1]) );
	auto vuns = hexAsciiToBinary(vasc);

	size_t mCrc = crc(vuns);
	
	vuns.push_back(mCrc);

	cout << "vasc: \n\t" << vasc << "\n";
	cout << "vuns: \n\t" << vuns << "\n";
	printf("check: %02x\n\t", mCrc);

	/*************************************/	

	string pCOM = argv[1];
	int baudios = 115200;

	try {
		asio::io_context ctx;	

		asio::serial_port sp(ctx, pCOM);
		
		if(!sp.is_open()) {
			cerr << "Incapaz de abrir puerto serie\n";
			exit(-1);
		}

		sp.set_option(asio::serial_port_base::baud_rate(baudios));
		//sp.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::even));
		sp.set_option(asio::serial_port_base::character_size(8));
		sp.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
		sp.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

		constexpr auto RXBUF_SZ = 1;
		char rxbuf[RXBUF_SZ];
		
		do_read(sp, rxbuf, RXBUF_SZ);
		
		std::thread t([&ctx] () { ctx.run(); } );
		
		asio::write(sp, asio::buffer(vuns, vuns.size()) );
		
		string line;
		while(std::getline(cin, line)) 
		{
			line = parse_line(line);
			asio::write(sp, asio::buffer(line, line.size()) ); 
		}
		
		ctx.stop();
		t.join();
		
	} catch(std::exception const& e) {
		std::cout << "Excepcion: " << e.what() << std::endl;
	}
	return 0;
}