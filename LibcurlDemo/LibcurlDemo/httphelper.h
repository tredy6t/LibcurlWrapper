#pragma once
#include <string>
#include <rapidjson/document.h>
#include <easyloggingpp/easylogging++.h>


namespace Saas
{
static std::string ParseHttpResponse(const std::string& response)
{
    rapidjson::Document doc;
    if (doc.Parse(response.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse return msg: " << response;
        return std::string();
    }
    if (doc.IsObject() && doc.HasMember("msg") && doc["msg"].IsString()) {
        return doc["msg"].GetString();
    }
    if (doc.IsArray() && doc.Size() > 0) {
        rapidjson::Value& item = doc[0];
        if (item.IsObject() && item.HasMember("msg") && item["msg"].IsString()) {
            return item["msg"].GetString();
        }
    }
    LOG(ERROR) << "Return msg is not valid:" << response;
    return std::string();
}
}