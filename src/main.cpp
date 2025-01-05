#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <cryptonote_config.h>
#include <wallet/wallet2.h>

#include "validate.h"

namespace beast = boost::beast;			// from <boost/beast.hpp>
namespace http = beast::http;			// from <boost/beast/http.hpp>
namespace asio = boost::asio;			// from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;		// from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;		// from <boost/asio/ip/tcp.hpp>

// Handles an HTTP server connection
void
do_session(
	tcp::socket& socket,
	beast::tcp_stream& out_stream,
	const std::string& auth_scheme,
	tools::wallet2& wallet,
	const std::string& min_amount,
	unsigned min_confirmations,
	unsigned max_confirmations)
{
	beast::error_code ec;
	beast::flat_buffer buffer;

	for(;;)
	{
		try
		{
			http::request<http::dynamic_body> req;
			http::read(socket, buffer, req);

			if (auto&& [valid,res] = xmr402::validate(req, auth_scheme, wallet, min_amount, min_confirmations, max_confirmations); !valid)
			{
				http::write(socket, res);
				break;
			}

			http::write(out_stream, req);

			http::response<http::dynamic_body> res;
			http::read(out_stream, buffer, res);
			bool keep_alive = res.keep_alive();

			http::write(socket, res);

			if(!keep_alive)
			{
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				break;
			}
		}
		catch(const std::exception& e)
		{
			// TODO internal_server_error
			std::cerr << "Error: " << e.what() << std::endl;
			break;
		}
	}

	socket.shutdown(tcp::socket::shutdown_send, ec);
	if(ec)
		std::cerr << "shutdown: " << ec.message() << std::endl;

}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	try
	{
		if (argc < 11)
		{
			std::cerr <<
				"Usage: xmr402 <in-address> <in-port> <out-address> <out-port> <authentication scheme: Basic or Bearer> <wallet-file> <wallet-password> <min-amount> <min-confirmations> <max-confirmations> [<network-type>] [<node-url>]\n" <<
				"Example:\n" <<
				"	 xmr402 0.0.0.0 8080 127.0.0.1 8000 Basic ~/.local/share/xmr-stagenet-wallets/server 1234 0.01 3 150 STAGENET http://45.63.8.26:38081\n";
			return EXIT_FAILURE;
		}
		auto const in_address = asio::ip::make_address(argv[1]);
		auto const in_port = static_cast<unsigned short>(std::atoi(argv[2]));
		auto const out_host = argv[3];
		auto const out_port = argv[4];
		const std::string auth_scheme = argv[5];
		const std::string path = argv[6];
		const std::string password = argv[7];
		const std::string min_amount = argv[8];
		auto const min_confirmations = static_cast<unsigned>(std::atoi(argv[9]));
		auto const max_confirmations = static_cast<unsigned>(std::atoi(argv[10]));

		cryptonote::network_type network_type = cryptonote::MAINNET;
		std::string node_url = "http://localhost:18081";
		if (argc > 11)
		{
			const std::string str_net_type = argv[11];
			std::cout << "Warning: network type: " << str_net_type << std::endl;
			if (str_net_type == "TESTNET") network_type = cryptonote::TESTNET;
			else if (str_net_type == "STAGENET")
			{
				network_type = cryptonote::STAGENET;
				node_url = "http://45.63.8.26:38081";
			}
			else
			{
				std::cerr << "Unsupported network type: " << str_net_type << std::endl;
				return EXIT_FAILURE;
			}
		}
		if (argc == 13)
			node_url = argv[12];

		std::cout << "Info: Node URL: " << node_url << " [" << auth_scheme << ']' << std::endl;

		asio::io_context ioc{1};
		tcp::acceptor acceptor{ioc, {in_address, in_port}};
		tcp::resolver resolver(ioc);
		auto const results = resolver.resolve(out_host, out_port);

		auto wallet = std::make_unique<tools::wallet2>(network_type, 1, true);
		wallet->load(path, password);
		wallet->init(node_url);
		std::cout << "Info: address: " << wallet->get_address_as_str() << '\n';

		for(;;)
		{
			tcp::socket in_socket(ioc);

			// Block until we get a connection
			acceptor.accept(in_socket);

			beast::tcp_stream out_stream(ioc);
			try
			{
				beast::get_lowest_layer(out_stream).connect(results);
			}
			catch (const boost::system::system_error& e)
			{
				std::cerr << "Boost Error: " << e.what() << std::endl;
				continue;
			}

			std::thread{std::bind(
				&do_session,
				std::move(in_socket),
				std::move(out_stream),
				std::cref(auth_scheme),
				std::ref(*wallet),
				std::cref(min_amount),
				min_confirmations,
				max_confirmations)}.detach();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
