#pragma once

#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/message.hpp> // request response

namespace tools { class wallet2; }

namespace xmr402
{
	std::pair<bool, boost::beast::http::response<boost::beast::http::string_body>>
	validate(const boost::beast::http::request<boost::beast::http::dynamic_body>& req,
	         const std::string& auth_scheme,
	         tools::wallet2& wallet,
	         const std::string& str_min_amount,
	         unsigned min_confirmations, unsigned max_confirmations);
}
