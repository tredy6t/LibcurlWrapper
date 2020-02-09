#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <map>
#include <vector>
#include <curl/curl.h>

enum ContentType {
    kContentTypeNone = 0,
    kContentTypeTxt,
    kContentTypeHtml,
    kContentTypeXml,
    kContentTypeJson,
    kContentTypeUrlencoded,
    kContentTypeZip
};

struct HttpPara
{
    std::string strUrl;
    std::vector<std::string> vecHeaders;
    int nTimeout;
    std::string strCertificate;
    std::string strUser;
    std::string strPassword;
};

struct FileNodeInfo
{
    int64_t nPart;  // part ���
    int64_t nStartPos = 0;
    int64_t nEndPos = 0;
    FILE *fp;
    CURL *pCurl;
};

class LibcurlHelper
{
    static const int kSpeedInfo = 9;
    static const int kSecondsPerHour = 3600;
    static const int kSecondsPerMinute = 60;
    static const int kKByte = 1024;
    static const int kMByte = kKByte*kKByte;
    static const int64_t kGByte = kMByte*kKByte;
public:
    LibcurlHelper();
    ~LibcurlHelper();

public:
    static int OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *);
    static int64_t OnWriteString(void* buffer, size_t size, size_t nmemb, void* lpVoid);
    static int64_t OnWriteHeader(void* buffer, size_t size, size_t nmemb, void* lpVoid);
    static int64_t OnWriteFile(void *ptr, size_t size, size_t nMenmb, FILE *stream);
    static int64_t OnMultiWriteFile(void *ptr, size_t size, size_t nMenmb, void *pUserData);
    static int OnProcess(void *ptr, double nTotalToDownload, double nAlreadyDownload
        , double nTotalUploadSize,double nAlreadyUpload);

public:
    // �ϴ�����
    int Post(const HttpPara& paraHttp, const std::string& strData, std::string& strResponse);
    // �ϴ��ļ�
    int UploadFile(const HttpPara& paraHttp, const std::string& strFile, std::string& strResponse);
    // �ϴ�������
    int PostFormData(const HttpPara& paraHttp, const curl_httppost* pData, const std::string& strResponse);

    // ��������
    int Get(const HttpPara& paraHttp, std::string& strResponse);
    // �����ļ�
    int DownloadFile(const HttpPara& paraHttp, const std::string& strFile = "");
    // ���ش��ļ�
    int DownloadBigFile(const HttpPara& paraHttp, const std::string& strPath = "", const std::string& strFileName = "", int nThread = 1);

private:
    void get_header_type(int nType, std::string& strHeader);
    curl_slist* set_header(const std::vector<std::string>& vecHeaders);
    int64_t get_download_file_length(const HttpPara& paraHttp, std::string& strRemoteFileName);
    std::string generate_filename(const std::string& strUrl, const std::string& strCustonFile, const std::string& strFileInfo);
    void do_download(void *pData);
    void notify_download();
    long get_local_file_length(const std::string& strFile);

private:
    bool m_bDebug;
    int m_nTimeout;
    FILE* m_pFout;
    int64_t m_nPart;        // ��Ƭ��¼
    std::mutex m_mtLockRecord;  // �߳�������
    std::mutex m_mtWrite;       // д���ݿ���

};

