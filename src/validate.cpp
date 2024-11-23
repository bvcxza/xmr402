#include "validate.h"

#include <cassert>
#include <iostream>
#include <string_view>
#include <utility>

#include <boost/beast/http.hpp>

#include <wallet/wallet2.h>

namespace xmr402
{

namespace http = boost::beast::http;     // from <boost/beast/http.hpp>

namespace
{

const std::string BEARER_PART = "Bearer ";

std::pair<std::string_view,std::string_view> split_pair(std::string_view str, char delim)
{
	auto delim_pos = str.find_first_of(delim);
	if (str.size() < 3 || delim_pos == str.npos)
		return {};

	return {str.substr(0, delim_pos), str.substr(delim_pos + 1)};
}

bool replace_all(std::string& inout, const std::map<std::string_view,std::string_view>& map)
{
	bool result = false;
	for (auto [what,with] : map)
		for (std::string::size_type pos{};
		     inout.npos != (pos = inout.find(what.data(), pos, what.length()));
		     pos += with.length(), result = true)
			inout.replace(pos, what.length(), with.data(), with.length());

	return result;
}

bool is_valid(std::string_view auth, tools::wallet2& wallet)
{
	if (auto&& [tx,sig] = split_pair(auth.substr(BEARER_PART.size()), ':');
	    !tx.empty() && !sig.empty())
	{
		std::cout << "tx: >>" << tx << "<<\n";
		std::cout << "sig: >>" << sig << "<<\n";
		crypto::hash txid;
		if (!epee::string_tools::hex_to_pod(std::string(tx), txid))
		{
			std::cout << tx << " malformed\n";
			return false;
		}
		try
		{
			wallet.refresh(true);
			uint64_t received, confirmations;
			bool in_pool;
			bool r = wallet.check_tx_proof(txid,
			                               wallet.get_address(),
			                               false, // main address
			                               "", // no message
			                               std::string(sig),
			                               received,
			                               in_pool,
			                               confirmations);

			if (in_pool)
			{
				std::cout << tx << " in pool\n";
				return false;
			}
			if (confirmations < 2)
			{
				std::cout << tx << " confirmations(" << confirmations << ") < 2\n";
				return false;
			}
			std::cout << tx << " received=" << cryptonote::print_money(received) << " confirmations=" << confirmations << std::endl;
			return r;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			return false;
		}
	}

	return false;
}

}

std::pair<bool, http::response<http::dynamic_body>>
validate(const http::request<http::dynamic_body>& req, tools::wallet2& wallet)
{
	if (auto auth_value = req[http::field::authorization];
	    auth_value.empty() || !auth_value.starts_with(BEARER_PART) || !is_valid(auth_value, wallet))
	{
		http::response<http::dynamic_body> res{http::status::payment_required, req.version()};
		std::string auth_res = R"(Bearer address="${address}")";
		assert(replace_all(auth_res, {{"${address}", wallet.get_address_as_str()}}));
		res.set(http::field::www_authenticate, auth_res);
		return {false, res};
	}
	return {true, {}};
}

}
