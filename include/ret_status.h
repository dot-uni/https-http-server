#ifndef RET_STATUS_INCLUDED
#define RET_STATUS_INCLUDED

#include <cstdint>
#include <string>
#include <string_view>

#include "status.h"


namespace http {

// Application-level result code returned in the `retCode` field of every response.
// Underlying type is uint32_t since all codes are small non-negative integers.
enum class retCode : unsigned {
    Success                        = 20000,     // HTTP 200/201/202
    InvalidJsonOrParams            = 40000, // HTTP 400
    AuthError                      = 40100, // HTTP 401
    Forbidden                      = 40300, // HTTP 403 (no scope/permission)
    NotFound                       = 40400, // HTTP 404
    ConflictDuplicateClientOrderId = 40900, // HTTP 409
    InvalidSimulationState         = 42200, // HTTP 422
    RateLimitExceeded              = 42900, // HTTP 429
    InternalError                  = 50000, // HTTP 500
    InsufficientBalance            = 40001, // HTTP 400
    PriceOrQtyNotAligned           = 40002, // HTTP 400 (not multiple of tickSize/qtyStep)
    PositionOrOrderLimitExceeded   = 42201, // HTTP 422
    UnknownSymbolOrDataset         = 40401, // HTTP 404
    SimulationNotRunning           = 42202, // HTTP 422
    ClientOrderIdAlreadyUsed       = 40901, // HTTP 409
    SessionOrSignatureExpired      = 40101, // HTTP 401
    InvalidSignature               = 40102, // HTTP 401
    ClockSkewExceedsRecvWindow     = 40103, // HTTP 401
    PostOnlyWouldCross             = 42203, // HTTP 422
    RequestBufferOverflow          = 41300, // HTTP 413 (incoming request exceeded receive buffer size)
    MethodNotAllowed               = 40500, // HTTP 405 (verb not supported for this endpoint)
    NotAcceptable                  = 40600, // HTTP 406 (Accept header cannot be satisfied)
    RequestTimeout                 = 40800, // HTTP 408 (client too slow sending the request)
    UnsupportedMediaType           = 41500, // HTTP 415 (Content-Type not accepted, expected application/json)
    MissingContentLength           = 41100, // HTTP 411
    UriTooLong                     = 41400, // HTTP 414
    PreconditionFailed             = 41200, // HTTP 412 (If-Match/If-Unmodified-Since precondition failed)
    ExpectationFailed              = 41700, // HTTP 417
    ResourceGone                   = 41000, // HTTP 410 (resource permanently removed, e.g. expired dataset)
    TooManyOpenOrders              = 42901, // HTTP 429 (per-symbol/account open-order cap hit)
    RequestHeaderFieldsTooLarge    = 43100, // HTTP 431
    UnavailableForLegalReasons     = 45100, // HTTP 451 (symbol/dataset restricted in jurisdiction)
    NotImplemented                 = 50100, // HTTP 501 (endpoint/feature not yet implemented)
    BadGateway                     = 50200, // HTTP 502 (upstream matching engine/market-data feed error)
    ServiceUnavailable             = 50300, // HTTP 503 (server overloaded or in maintenance)
    GatewayTimeout                 = 50400, // HTTP 504 (upstream matching engine/market-data feed timeout)
    InsufficientStorage            = 50700, // HTTP 507
};

// Returns the HTTP status code most commonly associated with a retCode.
// Note: Success (0) is context-dependent — 200 for GET/query, 201 for creation,
// 202 for an accepted async command. Pass the actual HTTP status separately
// when handling Success; the value below is just a sensible default.
constexpr status toHttpStatus(retCode code)
{
    return int_to_status(static_cast<unsigned>(code) / 100);
}

// Human-readable description 
constexpr std::string_view retMesg(retCode code) {
    switch (static_cast<retCode>(code)) {
        case retCode::Success:                         return "Success";
        case retCode::InvalidJsonOrParams:             return "Invalid JSON or parameters";
        case retCode::AuthError:                       return "Authorization error";
        case retCode::Forbidden:                       return "Forbidden (insufficient scope)";
        case retCode::NotFound:                        return "Resource not found";
        case retCode::ConflictDuplicateClientOrderId:  return "Conflict or duplicated clientOrderId";
        case retCode::InvalidSimulationState:          return "Command not valid for current simulation state";
        case retCode::RateLimitExceeded:               return "Rate limit exceeded";
        case retCode::InternalError:                   return "Internal error";
        case retCode::InsufficientBalance:             return "Insufficient balance";
        case retCode::PriceOrQtyNotAligned:            return "Price/quantity not aligned with tickSize/qtyStep";
        case retCode::PositionOrOrderLimitExceeded:    return "Position or order size limit exceeded";
        case retCode::UnknownSymbolOrDataset:          return "Unknown symbol or dataset";
        case retCode::SimulationNotRunning:            return "Simulation is not in Running state";
        case retCode::ClientOrderIdAlreadyUsed:        return "clientOrderId already used in this simulation";
        case retCode::SessionOrSignatureExpired:       return "Session/signature expired";
        case retCode::InvalidSignature:                return "Invalid signature";
        case retCode::ClockSkewExceedsRecvWindow:      return "Clock skew exceeds recvWindow";
        case retCode::PostOnlyWouldCross:              return "PostOnly order would cross the book (WouldCross)";
        case retCode::RequestBufferOverflow:           return "Request exceeded receive buffer size";
        case retCode::MethodNotAllowed:                return "HTTP method not allowed for this endpoint";
        case retCode::NotAcceptable:                   return "Requested representation not available (Accept header)";
        case retCode::RequestTimeout:                  return "Client took too long to send the request";
        case retCode::UnsupportedMediaType:            return "Unsupported Content-Type (expected application/json)";
        case retCode::MissingContentLength:            return "Content-Length header is required";
        case retCode::UriTooLong:                      return "Request URI too long";
        case retCode::PreconditionFailed:              return "Precondition failed";
        case retCode::ExpectationFailed:               return "Expectation failed";
        case retCode::ResourceGone:                    return "Resource permanently removed";
        case retCode::TooManyOpenOrders:               return "Too many open orders for this account/symbol";
        case retCode::RequestHeaderFieldsTooLarge:     return "Request header fields too large";
        case retCode::UnavailableForLegalReasons:      return "Unavailable for legal reasons";
        case retCode::NotImplemented:                  return "Not implemented";
        case retCode::BadGateway:                      return "Upstream matching engine/data feed error";
        case retCode::ServiceUnavailable:              return "Service temporarily unavailable";
        case retCode::GatewayTimeout:                  return "Upstream matching engine/data feed timeout";
        case retCode::InsufficientStorage:             return "Insufficient storage";
        default:
            break;
    }
    return "<unknown-retCode>";
}

} // namespace http

#endif