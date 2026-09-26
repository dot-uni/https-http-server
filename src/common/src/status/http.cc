#include "project/common/status/http.h"


namespace uni::common::status {

http int_to_status(unsigned v)
{
    switch(static_cast<http>(v))
    {
    // 1xx
    case http::continue_:
    case http::switching_protocols:
    case http::processing:
    case http::early_hints:
        FALLTHROUGH;

    // 2xx
    case http::ok:
    case http::created:
    case http::accepted:
    case http::non_authoritative_information:
    case http::no_content:
    case http::reset_content:
    case http::partial_content:
    case http::multi_status:
    case http::already_reported:
    case http::im_used:
        FALLTHROUGH;

    // 3xx
    case http::multiple_choices:
    case http::moved_permanently:
    case http::found:
    case http::see_other:
    case http::not_modified:
    case http::use_proxy:
    case http::temporary_redirect:
    case http::permanent_redirect:
        FALLTHROUGH;

    // 4xx
    case http::bad_request:
    case http::unauthorized:
    case http::payment_required:
    case http::forbidden:
    case http::not_found:
    case http::method_not_allowed:
    case http::not_acceptable:
    case http::proxy_authentication_required:
    case http::request_timeout:
    case http::conflict:
    case http::gone:
    case http::length_required:
    case http::precondition_failed:
    case http::payload_too_large:
    case http::uri_too_long:
    case http::unsupported_media_type:
    case http::range_not_satisfiable:
    case http::expectation_failed:
    case http::i_am_a_teapot:
    case http::misdirected_request:
    case http::unprocessable_entity:
    case http::locked:
    case http::failed_dependency:
    case http::too_early:
    case http::upgrade_required:
    case http::precondition_required:
    case http::too_many_requests:
    case http::request_header_fields_too_large:
    case http::unavailable_for_legal_reasons:
        FALLTHROUGH;

    // 5xx
    case http::internal_server_error:
    case http::not_implemented:
    case http::bad_gateway:
    case http::service_unavailable:
    case http::gateway_timeout:
    case http::http_version_not_supported:
    case http::variant_also_negotiates:
    case http::insufficient_storage:
    case http::loop_detected:
    case http::not_extended:
    case http::network_authentication_required:
        return static_cast<http>(v);

    default:
        break;
    }
    return http::unknown;
}

status_class to_status_class(unsigned v)
{
    switch(v / 100)
    {
    case 1: return status_class::informational;
    case 2: return status_class::successful;
    case 3: return status_class::redirection;
    case 4: return status_class::client_error;
    case 5: return status_class::server_error;
    default:
        break;
    }
    return status_class::unknown;
}

status_class to_status_class(http v)
{
    return to_status_class(static_cast<int>(v));
}

std::string_view obsolete_reason(http v)
{
    switch(static_cast<http>(v))
    {
    // 1xx
    case http::continue_:                             return "Continue";
    case http::switching_protocols:                   return "Switching Protocols";
    case http::processing:                            return "Processing";
    case http::early_hints:                           return "Early Hints";

    // 2xx
    case http::ok:                                    return "OK";
    case http::created:                               return "Created";
    case http::accepted:                              return "Accepted";
    case http::non_authoritative_information:         return "Non-Authoritative Information";
    case http::no_content:                            return "No Content";
    case http::reset_content:                         return "Reset Content";
    case http::partial_content:                       return "Partial Content";
    case http::multi_status:                          return "Multi-Status";
    case http::already_reported:                      return "Already Reported";
    case http::im_used:                               return "IM Used";

    // 3xx
    case http::multiple_choices:                      return "Multiple Choices";
    case http::moved_permanently:                     return "Moved Permanently";
    case http::found:                                 return "Found";
    case http::see_other:                             return "See Other";
    case http::not_modified:                          return "Not Modified";
    case http::use_proxy:                             return "Use Proxy";
    case http::temporary_redirect:                    return "Temporary Redirect";
    case http::permanent_redirect:                    return "Permanent Redirect";

    // 4xx
    case http::bad_request:                           return "Bad Request";
    case http::unauthorized:                          return "Unauthorized";
    case http::payment_required:                      return "Payment Required";
    case http::forbidden:                             return "Forbidden";
    case http::not_found:                             return "Not Found";
    case http::method_not_allowed:                    return "Method Not Allowed";
    case http::not_acceptable:                        return "Not Acceptable";
    case http::proxy_authentication_required:         return "Proxy Authentication Required";
    case http::request_timeout:                       return "Request Timeout";
    case http::conflict:                              return "Conflict";
    case http::gone:                                  return "Gone";
    case http::length_required:                       return "Length Required";
    case http::precondition_failed:                   return "Precondition Failed";
    case http::payload_too_large:                     return "Payload Too Large";
    case http::uri_too_long:                          return "URI Too Long";
    case http::unsupported_media_type:                return "Unsupported Media Type";
    case http::range_not_satisfiable:                 return "Range Not Satisfiable";
    case http::expectation_failed:                    return "Expectation Failed";
    case http::i_am_a_teapot:                         return "I'm a teapot";
    case http::misdirected_request:                   return "Misdirected Request";
    case http::unprocessable_entity:                  return "Unprocessable Entity";
    case http::locked:                                return "Locked";
    case http::failed_dependency:                     return "Failed Dependency";
    case http::too_early:                             return "Too Early";
    case http::upgrade_required:                      return "Upgrade Required";
    case http::precondition_required:                 return "Precondition Required";
    case http::too_many_requests:                     return "Too Many Requests";
    case http::request_header_fields_too_large:       return "Request Header Fields Too Large";
    case http::unavailable_for_legal_reasons:         return "Unavailable For Legal Reasons";
    // 5xx
    case http::internal_server_error:                 return "Internal Server Error";
    case http::not_implemented:                       return "Not Implemented";
    case http::bad_gateway:                           return "Bad Gateway";
    case http::service_unavailable:                   return "Service Unavailable";
    case http::gateway_timeout:                       return "Gateway Timeout";
    case http::http_version_not_supported:            return "HTTP Version Not Supported";
    case http::variant_also_negotiates:               return "Variant Also Negotiates";
    case http::insufficient_storage:                  return "Insufficient Storage";
    case http::loop_detected:                         return "Loop Detected";
    case http::not_extended:                          return "Not Extended";
    case http::network_authentication_required:       return "Network Authentication Required";

    default:
        break;
    }
    return "<unknown-status>";
}

} // namespace uni::common::status