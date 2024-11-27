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

bool is_valid(std::string_view auth, tools::wallet2& wallet, const std::string& str_min_amount, std::string& error_description)
{
	if (!auth.starts_with(BEARER_PART))
	{
		error_description = "not bearer token";
		return false;
	}
	if (auto&& [tx,sig] = split_pair(auth.substr(BEARER_PART.size()), ':');
	    !tx.empty() && !sig.empty())
	{
		crypto::hash txid;
		if (!epee::string_tools::hex_to_pod(std::string(tx), txid))
		{
			std::cout << tx << " malformed\n";
			error_description = "malformed txid";
			return false;
		}
		try
		{
			wallet.refresh(true);
			std::cout << "blockchain_current_height: " << wallet.get_blockchain_current_height() << '\n';
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
				error_description = "tx in pool";
				return false;
			}
			if (confirmations < 2)
			{
				std::cout << tx << " confirmations(" << confirmations << ") < 2\n";
				error_description = "tx needs 2 confirmations";
				return false;
			}
			uint64_t min_amount;
			if (!cryptonote::parse_amount(min_amount, str_min_amount))
			{
				std::cerr << "Error: parse_amount - " << str_min_amount << std::endl;
				return false;
			}
			if (received < min_amount)
			{
				std::cout << tx << " received=" << cryptonote::print_money(received) << " < min_amount=" << cryptonote::print_money(min_amount) << '\n';
				error_description = "amount received less then minimum " + cryptonote::print_money(min_amount);
				return false;
			}
			std::cout << tx << " received=" << cryptonote::print_money(received) << " confirmations=" << confirmations << std::endl;
			return r;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			error_description = e.what();
			return false;
		}
	}

	error_description = "invalid token";
	return false;
}

}

std::pair<bool, http::response<http::dynamic_body>>
validate(const http::request<http::dynamic_body>& req, tools::wallet2& wallet, const std::string& str_min_amount)
{
	std::string error_description;
	if (auto auth_value = req[http::field::authorization];
	    auth_value.empty() || !is_valid(auth_value, wallet, str_min_amount, error_description))
	{
		http::response<http::dynamic_body> res{http::status::payment_required, req.version()};
		std::string auth_res = R"(Bearer realm="xmr402 proxy",currency="XMR",address="${address}")";
		if (!auth_value.empty() && !error_description.empty())
		{
			auth_res += R"(,error="invalid_token",error_description="${error_description}")";
		}
		assert(replace_all(auth_res, {{"${address}", wallet.get_address_as_str()},{"${error_description}", error_description}}));
		res.set(http::field::www_authenticate, auth_res);
		return {false, res};
	}
	return {true, {}};
}

}
