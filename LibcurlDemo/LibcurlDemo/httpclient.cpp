#include "httpclient.h"


RaiiHelper g_curlHandle1
(
    []() {curl_global_init(CURL_GLOBAL_ALL); },
    []() {curl_global_cleanup(); }
);

HttpClient::HeaderData::HeaderData()
    : headerlist(nullptr)
{
}

HttpClient::HeaderData::~HeaderData()
{
    if (headerlist != nullptr) {
        curl_slist_free_all(headerlist);
        headerlist = nullptr;
    }
}

HttpClient::FormData::FormData()
    : formpost(nullptr)
    , lastptr(nullptr)
    , headerlist(nullptr)
{
}

HttpClient::FormData::~FormData()
{
    if (formpost != nullptr) {
        curl_formfree(formpost);
        formpost = nullptr;
        lastptr = nullptr;
    }
    if (headerlist != nullptr) {
        curl_slist_free_all(headerlist);
        headerlist = nullptr;
    }
}

HttpClient::HttpClient()
    : curl_(nullptr)
{
    curl_ = curl_easy_init();
    // set a default user agent
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, curl_version());
}

HttpClient::~HttpClient()
{
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
}

bool HttpClient::Get(const std::string &url, std::string &response)
{
    // x-www-form-urlencoded.
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &HttpClient::write_to_string);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5);
    CURLcode res = curl_easy_perform(curl_);
    int response_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
    if (res != CURLE_OK || response_code != 200) {
        return false;
    }
    return true;
}

bool HttpClient::Post(const std::string& url, const std::string& request,
    ContentType type, std::string& response)
{
    HeaderData header;
    fill_header(type, header);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header.headerlist);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &HttpClient::write_to_string);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5);
    CURLcode res = curl_easy_perform(curl_);
    int response_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
    if (res != CURLE_OK || response_code != 200) {
        return false;
    }
    return true;
}

bool HttpClient::UploadData(const std::string& url, const std::string& data,
    std::string& response)
{
    // initialize custom header list
    // stating that Expect: 100-continue is not wanted.
    FormData formdata;
    fill_data_form(data, formdata);
    return upload(url, formdata, response);
}

bool HttpClient::UploadFile(const std::string& url, const std::string& file,
    std::string& response)
{
    // initialize custom header list
    // stating that Expect: 100-continue is not wanted.
    FormData formdata;
    fill_file_form(file, formdata);
    return upload(url, formdata, response);
}

bool HttpClient::upload(const std::string& url, const FormData& formdata,
    std::string& response)
{
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, formdata.headerlist);
    curl_easy_setopt(curl_, CURLOPT_HTTPPOST, formdata.formpost);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &HttpClient::write_to_string);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5);
    CURLcode res = curl_easy_perform(curl_);
    int response_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
    if (res != CURLE_OK || response_code != 200) {
        return false;
    }
    return true;
}

void HttpClient::fill_header(ContentType type, HeaderData& header)
{
    const char *buffer = nullptr;
    switch (type)
    {
    case kContentTypeTxt:
        buffer = "Content-Type: text/plain";
        break;
    case kContentTypeHtml:
        buffer = "Content-Type: text/html";
        break;
    case kContentTypeXml:
        buffer = "Content-Type: text/xml";
        break;
    case kContentTypeJson:
        buffer = "Content-Type: application/json";
        break;
    case kContentTypeUrlencoded:
        buffer = "Content-Type: x-www-form-urlencoded";
        break;
    default:
        buffer = "Content-Type: application/octet-stream";
        break;
    }
    header.headerlist = curl_slist_append(header.headerlist, buffer);
}

void HttpClient::fill_data_form(const std::string& data, FormData& formdata)
{
    curl_formadd(&formdata.formpost, &formdata.lastptr,
        CURLFORM_COPYNAME, "file",
        CURLFORM_BUFFER, "unnamed file",
        CURLFORM_BUFFERPTR, data.c_str(),
        CURLFORM_BUFFERLENGTH, data.size(),
        CURLFORM_CONTENTTYPE, "application/octet-stream",
        CURLFORM_END);
    curl_formadd(&formdata.formpost, &formdata.lastptr,
        CURLFORM_COPYNAME, "submit",
        CURLFORM_COPYCONTENTS, "send",
        CURLFORM_END);
    static const char buffer[] = "Expect:";
    formdata.headerlist = curl_slist_append(formdata.headerlist, buffer);
}

void HttpClient::fill_file_form(const std::string& file, FormData& formdata)
{
    curl_formadd(&formdata.formpost, &formdata.lastptr,
        CURLFORM_COPYNAME, "file",
        CURLFORM_FILE, file.c_str(),
        CURLFORM_CONTENTTYPE, "multipart/form-data",
        CURLFORM_END);
    curl_formadd(&formdata.formpost, &formdata.lastptr,
        CURLFORM_COPYNAME, "submit",
        CURLFORM_COPYCONTENTS, "send",
        CURLFORM_END);
    static const char buffer[] = "Expect:";
    formdata.headerlist = curl_slist_append(formdata.headerlist, buffer);
}

size_t HttpClient::write_to_string(char *ptr, size_t size, size_t nmemb, void *sp)
{
    size_t len = size * nmemb;
    if (sp != nullptr) {
        std::string& str = *(std::string*)(sp);
        str.insert(str.end(), ptr, ptr + len);
    }
    return len;
}
