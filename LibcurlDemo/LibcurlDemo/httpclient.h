#ifndef UTILS_HTTPCLIENT__H__
#define UTILS_HTTPCLIENT__H__

#include <string>
#include <curl/curl.h>
#include "RaiiHelper.h"

class HttpClient
{
    struct HeaderData
    {
        HeaderData();
        ~HeaderData();
        // delete method
        HeaderData(const HeaderData&) = delete;
        HeaderData& operator =(const HeaderData&) = delete;

    public:
        curl_slist *headerlist;
    };

    struct FormData
    {
        FormData();
        ~FormData();
        // delete method
        FormData(const FormData&) = delete;
        FormData& operator =(const FormData&) = delete;

    public:
        curl_httppost *formpost;
        curl_httppost *lastptr;
        curl_slist *headerlist;
    };

public:
    enum ContentType {
        kContentTypeNone = 0,
        kContentTypeTxt,
        kContentTypeHtml,
        kContentTypeXml,
        kContentTypeJson,
        kContentTypeUrlencoded
    };

public:
    HttpClient();
    virtual ~HttpClient();
    // delete method
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator =(const HttpClient&) = delete;

    bool Get(const std::string &url, std::string &response);
    bool Post(const std::string& url, const std::string& request,
        ContentType type, std::string& response);
    bool UploadData(const std::string& url, const std::string& data,
        std::string& response);
    bool UploadFile(const std::string& url, const std::string& file,
        std::string& response);

private:
    bool upload(const std::string& url, const FormData& formdata, std::string& response);

    void fill_header(ContentType type, HeaderData& header);
    void fill_data_form(const std::string& data, FormData& formdata);
    void fill_file_form(const std::string& file, FormData& formdata);

private:
    static size_t write_to_string(char *ptr, size_t size, size_t nmemb, void *sp);

private:
    CURL *curl_;
};

#endif  // UTILS_HTTPCLIENT__H__