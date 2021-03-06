// boost_serial_callback.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"
#include <boost/thread.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
//#include <fstream>
#include <boost/chrono.hpp>
//#include <mutex>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

//using namespace boost::filesystem;

#define SERIAL_PORT_READ_BUF_SIZE 256
#define BAUD_RATE 19200
using namespace boost;

class espejo {
public:
	espejo(const std::string &puerto1, const std::string &puerto2)
	{
		boost::filesystem::path p{"salida_file78.dat"};
		outfile.open(p, std::ios::binary | std::ios::out | std::ios::app);
		outfile << "\n\n\n\tNUEVA CORRIDA \n\n\n";
		//auto out_sync = std::make_shared<SynchronizedFile>(&mutexS);
		port1 = boost::shared_ptr<boost::asio::serial_port> (new boost::asio::serial_port(io1));
		port1->open(puerto1, ec);
		port1->set_option(asio::serial_port_base::baud_rate(BAUD_RATE));
		//port_->set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
		port1->set_option(boost::asio::serial_port_base::character_size(8));
		port1->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		port1->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd));
		port1->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		//port 2
		port2 = boost::shared_ptr<boost::asio::serial_port>(new boost::asio::serial_port(io2));
		port2->open(puerto2, ec);
		port2->set_option(asio::serial_port_base::baud_rate(BAUD_RATE));
		//port_->set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
		port2->set_option(boost::asio::serial_port_base::character_size(8));
		port2->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		port2->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd));
		port2->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
		boost::thread t1(boost::bind(&boost::asio::io_service::run, &io1));
		boost::thread t2(boost::bind(&boost::asio::io_service::run, &io2));
		t0 = boost::chrono::high_resolution_clock::now();
		read_char1();
		read_char2();
	}

	virtual void on_receive1(const boost::system::error_code& ec, std::size_t bytes_transferred) {
		boost::mutex::scoped_lock look(mutex1);
		if (port1.get() == NULL || !port1->is_open()) {
			std::string enviar = "pC[";
			enviar += std::to_string(boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count());
			enviar+= "]";
			write_data(enviar, NULL, 0);
			return;
		}
		if (ec) {
			std::cout << "Error at Rec1:" << boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count() << ec.message() << '\n';
			read_char1();
			return;
		}
		//auto t2 = boost::chrono::high_resolution_clock::now();
		std::string enviar = "pC[";
		enviar += std::to_string(boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count());
		enviar+="]";
		write_data(enviar,read_buf_raw1, bytes_transferred);
		boost::system::error_code ecl;
		port2->write_some(boost::asio::buffer(read_buf_raw1, bytes_transferred), ecl);
		read_char1();
	}
	virtual void on_receive2(const boost::system::error_code& ec, std::size_t bytes_transferred) {
		boost::mutex::scoped_lock look(mutex2);
		if (port2.get() == NULL || !port2->is_open()) {
			std::string enviar = "pL[";
			enviar += std::to_string(boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count());
			enviar+= "]";
			write_data(enviar, NULL, 0);

			return;
		}
		if (ec) {
			std::cout << "Error at Rec2:" << boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count() <<ec.message()<<'\n';
			read_char2();
			return;
		}
		boost::system::error_code ecl;
		//escribo en el otro
		std::string enviar = "pL[";
		enviar += std::to_string(boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - t0).count());
		enviar += "]";
		write_data(enviar,read_buf_raw2, bytes_transferred);
		port1->write_some(boost::asio::buffer(read_buf_raw2, bytes_transferred), ecl);
		read_char2();
	}

	void read_char1() {

	    	port1->async_read_some(
			boost::asio::buffer(read_buf_raw1, SERIAL_PORT_READ_BUF_SIZE),
			boost::bind(
				&espejo::on_receive1,
				this, boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));


	}
	void read_char2() {
				port2->async_read_some(
			boost::asio::buffer(read_buf_raw2, SERIAL_PORT_READ_BUF_SIZE),
			boost::bind(
				&espejo::on_receive2,
				this, boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

	}
	void agregar_comentario(const std::string &comentario)
	{
		std::string enviar = "\n COMENT:";
			enviar += comentario;
		write_data(enviar, NULL, 0);

	}
	void  write_data(const std::string& comentario, char * comando, size_t largo) {
		boost::mutex::scoped_lock look(w_mutex);
		outfile << comentario;
		if (largo != 0)outfile.write(comando, largo);
	}

	virtual ~espejo(void) {
		outfile.close();
		boost::mutex::scoped_lock look1(mutex1);
		if (port1) {
			port1->cancel();
			port1->close();
			port1.reset();
		}
		io1.stop();
		io1.reset();
		boost::mutex::scoped_lock look2(mutex2);

		if (port2) {
			port2->cancel();
			port2->close();
			port2.reset();
		}
		io2.stop();
		io2.reset();
		Sleep(5 * 1000);
	}

private:
	boost::filesystem::ofstream outfile;
	boost::shared_ptr<boost::asio::serial_port> port1,port2;
	//boost::asio::ip::tcp::socket socket(my_context);
	boost::shared_ptr<boost::asio::ip::tcp::socket> tcp_socket;
	asio::io_service io1,io2;
	boost::system::error_code ec;
	boost::mutex mutex1,mutex2,w_mutex;
	char read_buf_raw1[SERIAL_PORT_READ_BUF_SIZE], read_buf_raw2[SERIAL_PORT_READ_BUF_SIZE];
	//boost::chrono::duration<zero_default<long long>, boost::milli       >
	boost::chrono::high_resolution_clock::time_point t0;
};
int main()
{
	espejo copia("COM8", "COM9");
	std::string acotacion;
	do {
		acotacion.clear();
		std::cin >> acotacion;
		if(acotacion.length()>1)copia.agregar_comentario(acotacion);
		//std::cout << acotacion << "y que paso?";
	} while (acotacion.length()>2);
	//std::system("pause");
    return 0;
}

