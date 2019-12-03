#include <iostream>
#include <vector>
#include <map>
#include <array>
#include <cstring>
#include <functional>
#include <iomanip> //setw(2)

#define ASIO_STANDALONE
#include "asio.hpp"

#include <windows.h>

using namespace std;

//eventually move into terminal class. If set to true CRC calculation will be done
bool bCalcCRC 	{true};
bool bHex		{true};
int estado=0; //jaja nmms

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
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 5);
	
	
	if(c1 > '9')
		r1 = (toupper(c1) - 55)*16;
	else
		r1 = (c1-48)*16;
	
	if(c2 > '9')
		r2 = (toupper(c2) - 55);
	else
		r2 = (c2-48);
	
	//printf("%c%c -> %d+%d ", c1, c2, r1, r2);	
	//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	return r1 + r2;
}


auto hexAsciiToBinary(vector<char> cont)
{
	vector<char> res;

	if(cont.size() % 2 != 0)
	{
		cerr << "ERROR: Invalid hex string length: Odd\n";
		exit(-1);
	}
	
	//we get our byte value from the 2 character representation
	for(int i=0; i < cont.size(); i+=2)
	{
		res.push_back( toBinary(cont[i], cont[i+1]) );
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

size_t crc(std::string vuns)
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

//por ahora servira pero es terrible. Asume buffers de 1 char
/*Podemos estar en estos estados:
	0: procesando datos arbitrarios
	1: esperando un caracter especial
	2: imprimiendo de manera especial los caracteres que llegan
*/
void procesar_incoming(char* bf, size_t len)
{
	static char aa;
	static short temperatura, humedad;
	static unsigned int cnt;
	assert(len == 1);
	if(estado == 0 && (*bf != '\\')) {
		if(bHex)
				print_as_hex(bf, len);
			else
				printf("%.*s", len, bf);
		return;
	}
	
	if(*bf == '\\') //brincamos a un estado especial donde el proximo caracter nos dirá a que estado ir
	{
		if(estado != 0) {//solo deberiamos llegar a 1 despues de 0, fail
			cerr << "No podemos procesar " << '\\' << " despues de si misma\n";
			return;
		}
		estado = 1;
		return;
	}
	
	if(estado == 1) {
		aa = *bf;
		estado = 2;
		cnt=0;
		return;
	}
	
	if (estado == 2) {
		if(aa == 't')
		{
			temperatura = temperatura + (*bf << (8 - cnt*8)); //buggggzzzz
			++cnt;
			if(cnt == 2) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);
				cout << "Temperatura: " << temperatura << ' ';
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
				cnt = 0;
				estado = 0;
				temperatura = 0;
				return;
			}
		}
		else if(aa == 'h')
		{
			humedad = humedad + (*bf << (8 - cnt*8)); //aaaaaaaaaaaaaaa!!
			++cnt;
			if(cnt == 2) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
				cout << "Humedad: " << humedad << '\n';
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
				cnt=0;
				estado = 0;
				humedad = 0;
				return;
			}
		}
	}
	
}

void do_read(asio::serial_port& sp, char* rxbuf, size_t RXBUF_SZ)
{
	asio::async_read(sp, asio::buffer(rxbuf, RXBUF_SZ), 
			[rxbuf, &sp, RXBUF_SZ](asio::error_code ec, size_t len)
		{
			//procesar_incoming(rxbuf, RXBUF_SZ);
			print_as_hex(rxbuf, RXBUF_SZ);
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
		if(bCalcCRC) {
			shex += crc(shex);
		}
		return shex;
	}
	//could this be our option configurator?
	else if(line[0] == '/')
	{
		string cmd = line.substr(1);
		if(cmd == "crc") {
			bCalcCRC = !bCalcCRC;
			cout << "CRC calculation is " << bCalcCRC << endl;
		}
		if(cmd == "hex") {
			bHex = !bHex;
			cout << "Imprimiendo como Hex? " << bHex << endl;
		}
		
		return "";
	}
	else
		return line;
}

int main(int argc, char** argv)
{
	//el ultimo argumento es la cadena
	//vector<char> vasc(argv[argc-1], argv[argc-1] + strlen(argv[argc-1]) );
	//auto vuns = hexAsciiToBinary(vasc);

	//size_t mCrc = crc(vuns);
	
	//vuns.push_back(mCrc);

	//cout << "vasc: \n\t" << vasc << "\n";
	//cout << "vuns: \n\t" << vuns << "\n";
	//printf("check: %02x\n\t", mCrc);

	/*************************************/	
	
	if(argc < 3) {
		cerr << "Please enter the COM port you wish to connect to, and the baudrate.\n"
			 << "e.g. term COM42 115200\n";
		exit(-1);
	}
	string pCOM = argv[1];
	string bd = argv[2];
	
	int baudios = stoi(bd);
	
	cout << "Attempting connection to " << pCOM 
		 << " with baud rate == " << to_string(baudios) << endl;
	cout << "CRC calculation is set to " << bCalcCRC << endl;

	try {
		asio::io_context ctx;	

		asio::serial_port sp(ctx, pCOM);
		
		if(!sp.is_open()) {
			cerr << "Incapaz de abrir puerto serie\n";
			exit(-1);
		}

		sp.set_option(asio::serial_port_base::baud_rate(baudios));
		sp.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
		sp.set_option(asio::serial_port_base::character_size(8));
		sp.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
		sp.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

		cout << "connected.\n";
		
		constexpr auto RXBUF_SZ = 1;
		char rxbuf[RXBUF_SZ];
		
		do_read(sp, rxbuf, RXBUF_SZ);
		
		std::thread t([&ctx] () { ctx.run(); } );
		
		cout << ">";
		
		string line;
		while(std::getline(cin, line)) 
		{
			line = parse_line(line);
			
			//we may pass configuration strings; don't send those.
			if(line != "")
				asio::write(sp, asio::buffer(line, line.size()) ); 
			cout << ">";
		}
		
		ctx.stop();
		t.join();
		
	} catch(std::exception const& e) {
		std::cout << "Excepcion: " << e.what() << std::endl;
	}
	return 0;
}