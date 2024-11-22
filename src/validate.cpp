#include "validate.h"

#include <boost/beast/http.hpp>

#include <wallet/wallet2.h>

namespace xmr402
{

namespace http = boost::beast::http;     // from <boost/beast/http.hpp>

std::pair<bool, http::response<http::dynamic_body>>
validate(const http::request<http::dynamic_body>& req, tools::wallet2& wallet)
{
	wallet.refresh(true);
	if (auto auth_value = req[http::field::authorization];
	    auth_value.empty() || !auth_value.starts_with("Bearer "))
	{
		http::response<http::dynamic_body> res{http::status::payment_required, req.version()};
		res.set(http::field::www_authenticate, R"(Bearer address="54gqcJZAtgz...")");
		return {false, res};
	}
	return {true, {}};
}

}
