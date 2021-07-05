#include "user_filter.h"

UserFilter::UserFilter(Bank &b) : bank(b) {}

void UserFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb)
{
    std::string_view auth_header = req->getHeader("Authorization");
    if (auth_header.size() > 6)
    {
        if (auth_header.substr(0, 6) == "Basic ")
        {
            std::string_view base64_input = auth_header.substr(6);
            char base64_result[(base64_input.size() * 3) / 4]; //only alloc
            size_t new_sz;
            base64_decode(base64_input.data(), base64_input.size(), base64_result, &new_sz, 0);

            std::string_view results_view(base64_result, new_sz);
            std::size_t middle = results_view.find(':');
            if (middle != std::string::npos)
            {
                base64_result[middle] = '\0';
                base64_result[new_sz] = '\0';
                const std::string &username = results_view.substr(0, middle).data();
                if (bank.VerifyPassword(username, results_view.substr(middle + 1)))
                {
                    req->setBody(username); //feels sub optimal
                    fccb();
                    return;
                }
            }
        }
    }
    const auto &resp = HttpResponse::newHttpJsonResponse("Invalid Credentials");
    resp->setStatusCode(k401Unauthorized);
    fcb(resp);
}