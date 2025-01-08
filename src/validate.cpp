#include "validate.h"

#include <cassert>
#include <iostream>
#include <map>
#include <string_view>
#include <utility>

#include <boost/beast/http.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <string_coding.h> // base64_decode
#include <wallet/wallet2.h>

namespace xmr402
{

namespace http = boost::beast::http;     // from <boost/beast/http.hpp>

namespace
{

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

bool is_valid(
	const std::string& auth_scheme,
	const std::string& auth,
	tools::wallet2& wallet,
	const std::string& str_min_amount,
	unsigned min_confirmations,
	unsigned max_confirmations,
	std::string& error_description)
{
	if (!auth.starts_with(auth_scheme))
	{
		error_description = "incorrect authorization scheme, must be " + auth_scheme;
		return false;
	}
	std::string token = auth.substr(auth_scheme.size());
	boost::trim(token);
	if (auth_scheme == "Basic")
		token = epee::string_encoding::base64_decode(token);

	if (auto&& [tx,sig] = split_pair(token, ':');
	    !tx.empty() && !sig.empty())
	{
		crypto::hash txid;
		if (!epee::string_tools::hex_to_pod(std::string(tx), txid))
		{
			std::cerr << "Error: malformed txid: " << tx << '\n';
			error_description = "malformed txid";
			return false;
		}
		try
		{
			wallet.refresh(true);
			std::cout << "Info: blockchain_current_height: " << wallet.get_blockchain_current_height() << '\n';
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
				std::cerr << "Error: " << tx << " in pool\n";
				error_description = "tx in pool";
				return false;
			}
			if (confirmations < min_confirmations)
			{
				std::cerr << "Error: " << tx << " confirmations(" << confirmations << ") < " << min_confirmations << '\n';
				error_description = "tx needs " + std::to_string(min_confirmations) + " confirmations";
				return false;
			}
			if (confirmations > max_confirmations)
			{
				std::cerr << "Error: " << tx << " confirmations(" << confirmations << ") > " << max_confirmations << '\n';
				error_description = "token is valid only for " + std::to_string(max_confirmations) + " confirmations";
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
				std::cerr << "Error: " << tx << " received=" << cryptonote::print_money(received) << " < min_amount=" << cryptonote::print_money(min_amount) << '\n';
				error_description = "amount received less then minimum " + cryptonote::print_money(min_amount);
				return false;
			}
			std::cout << "Info: " << tx << " received=" << cryptonote::print_money(received) << " confirmations=" << confirmations << std::endl;
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

std::pair<bool, http::response<http::string_body>>
validate(
	const http::request<http::dynamic_body>& req,
	const std::string& auth_scheme,
	tools::wallet2& wallet,
	const std::string& str_min_amount,
	unsigned min_confirmations,
	unsigned max_confirmations)
{
	http::response<http::string_body> res {http::status::ok, req.version()};
	res.set(http::field::access_control_allow_origin, "*");
	res.set(http::field::access_control_allow_methods, "*");
	res.set(http::field::access_control_allow_headers, "Authorization");
	if (req.method() == http::verb::options)
		return {false, res};

	std::string error_description;
	// get address before refresh
	const std::string wallet_main_address = wallet.get_address_as_str();
	if (auto auth_value = req[http::field::authorization];
	    auth_value.empty() || !is_valid(auth_scheme, auth_value, wallet, str_min_amount, min_confirmations, max_confirmations, error_description))
	{
		std::cout << "Debug: auth_value: >>" << auth_value << "<<\n";
		if (auth_scheme == "Basic")
			res.result(http::status::unauthorized);
		else
			res.result(http::status::payment_required);

		std::string auth_res = R"(${auth_scheme} realm="xmr402 proxy",currency="XMR",address="${address}",min_amount="${min_amount}",min_confirmations="${min_confirmations}",max_confirmations="${max_confirmations}")";
		std::string body_res = R"({"realm":"xmr402 proxy","currency":"XMR","address":"${address}","min_amount":"${min_amount}","min_confirmations":"${min_confirmations}","max_confirmations":"${max_confirmations}")";
		if (!auth_value.empty() && !error_description.empty())
		{
			auth_res += R"(,error="invalid_token",error_description="${error_description}")";
			body_res += R"(,"error":"invalid_token","error_description":"${error_description}")";
		}
		body_res += "}";
		const std::map<std::string_view,std::string_view> map = {
			{"${auth_scheme}", auth_scheme},
			{"${address}", wallet_main_address},
			{"${min_amount}", str_min_amount},
			{"${min_confirmations}", std::to_string(min_confirmations)},
			{"${max_confirmations}", std::to_string(max_confirmations)},
			{"${error_description}", error_description}
		};
		assert(replace_all(auth_res, map));
		assert(replace_all(body_res, map));
		res.set(http::field::www_authenticate, auth_res);
		res.body() = body_res;
		res.prepare_payload();
		return {false, res};
	}
	return {true, {}};
}

}
