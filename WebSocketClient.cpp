#include "WebSocketClient.hpp"

#include <websocketpp/common/cpp11.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

BEGIN_XE_NAMESPACE

class WebSocketClient::Impl
{
public:
	Impl(std::string uri);
	~Impl();
	void send(flatbuffers::DetachedBuffer const &blob);

private:
	using Client = websocketpp::client<websocketpp::config::asio_client>;
	using Message = websocketpp::config::asio_client::message_type::ptr;
	using ConnectionHandle = websocketpp::connection_hdl;
	using Connection = Client::connection_ptr;

	void onMessage(Client *c, ConnectionHandle hdl, Message msg);

	std::thread _thread;
	Client _client;
	Connection _connection;
};

WebSocketClient::Impl::Impl(std::string uri)
{
	try {
		// Set logging to be pretty verbose (everything except message payloads)
		_client.set_access_channels(websocketpp::log::alevel::all);
		_client.clear_access_channels(websocketpp::log::alevel::frame_payload);

		// Initialize ASIO
		_client.init_asio();

		// Register our message handler
		_client.set_message_handler(bind(
			&WebSocketClient::Impl::onMessage, this, &_client, ::_1, ::_2));

		websocketpp::lib::error_code ec;
		_connection = _client.get_connection(uri, ec);
		if (ec)
		{
			std::cout << "could not create connection because: "
				<< ec.message() << std::endl;
			return;
		}

		// Note that connect here only requests a connection. No network
		// messages are exchanged until the event loop starts running in the
		// next line.
		_client.connect(_connection);

		// Start the ASIO io_service run loop
		// this will cause a single connection to be made to the server. c.run()
		// will exit when this connection is closed.
		_thread = std::thread([this]() { _client.run(); });
	} catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
}

WebSocketClient::Impl::~Impl()
{
	_thread.join();
}

void WebSocketClient::Impl::send(flatbuffers::DetachedBuffer const &blob)
{
	websocketpp::lib::error_code err;

	_client.send(
		_connection->get_handle(),
		blob.data(),
		blob.size(),
		websocketpp::frame::opcode::binary,
		err);

	if (err)
	{
		std::cout << "Error sending message: " << err.message() << std::endl;
	}
}

void WebSocketClient::Impl::onMessage(
	Client *c, ConnectionHandle hdl, Message msg)
{
	auto blob = msg->get_payload();

	std::cout << ">>" << blob << std::endl;
	// std::cout << "on_message called with hdl: " << hdl.lock().get()
	// 		  << " and message: " << msg->get_payload()
	// 		  << std::endl;
	//
	// websocketpp::lib::error_code ec;
	//
	// c->send(hdl, msg->get_payload(), msg->get_opcode(), ec);
	// if (ec)
	// {
	// 	std::cout << "Echo failed because: " << ec.message() << std::endl;
	// }
}

WebSocketClient::WebSocketClient(std::string uri) :
	_impl(std::make_unique<WebSocketClient::Impl>(uri))
{ }

void WebSocketClient::send(flatbuffers::DetachedBuffer const &blob)
{
	_impl->send(blob);
}

WebSocketClient::~WebSocketClient() = default;

END_XE_NAMESPACE
