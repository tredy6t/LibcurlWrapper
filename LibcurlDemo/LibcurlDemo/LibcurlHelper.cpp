#include "LibcurlHelper.h"
#include "RaiiHelper.h"
#include <fstream>
#include <io.h>

RaiiHelper g_curlHandle
(
    []() {curl_global_init(CURL_GLOBAL_ALL); },
    []() {curl_global_cleanup(); }
);

int LibcurlHelper::OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *)
{
    switch (itype)
    {
    case CURLINFO_TEXT:
            printf("[TEXT]%s\n", pData);
            break;
    case CURLINFO_HEADER_IN:
            printf("[HEADER_IN]%s\n", pData);
            break;
    case CURLINFO_HEADER_OUT:
            printf("[HEADER_OUT]%s\n", pData);
            break;
    case CURLINFO_DATA_IN:
            printf("[DATA_IN]%s\n", pData);
            break;
    case CURLINFO_DATA_OUT:
        printf("[DATA_OUT]%s\n", pData);
        break;
    default:
        printf("Undefined type");
        break;
    }
    return 0;
}

int64_t LibcurlHelper::OnWriteString(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
    if (nullptr == str || nullptr == buffer) return -1;

    char* pData = (char*)buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}

int64_t LibcurlHelper::OnWriteHeader(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
    if (nullptr == str || nullptr == buffer) return -1;
    std::string strHeader((char*)buffer);
    int nPos = strHeader.find("Content-Disposition");
    if (nPos != std::string::npos)
    {
        std::string strFileInfo = strHeader.substr(nPos);
        // file info:filename=filename.type
        std::string strFileTag = "filename=";
        std::string strHeaderTag = "\r\n";
        nPos = strFileInfo.find(strFileTag);
        if (nPos != std::string::npos)
        {
            strFileInfo = strFileInfo.substr(0, strFileInfo.length() - strHeaderTag.length());
            std::string strFileName = strFileInfo.substr(nPos+ strFileTag.length());
            str->append(strFileName.c_str(), strFileName.length());
        }
    }
    return nmemb;
}

int64_t LibcurlHelper::OnWriteFile(void *ptr, size_t size, size_t nMenmb, FILE *stream)
{
    int64_t written = fwrite(ptr, size, nMenmb, stream);
    return written;
    // ����д������
    //FILE* fp = NULL;  
    //fopen_s(&fp, "c:\\test.dat", "ab+");
    //size_t nWrite = fwrite(ptr, nSize, nmemb, fp);
    //fclose(fp);  
    //return nWrite; 
}

int64_t LibcurlHelper::OnMultiWriteFile(void *ptr, size_t size, size_t nMenmb, void *pUserData)
{
    FileNodeInfo *pNodeInfo = (FileNodeInfo *)pUserData;
    size_t written = 0;
    if (0 == pNodeInfo->nEndPos)
    {
        return fwrite(ptr, size, nMenmb, pNodeInfo->fp);
    }
    if (pNodeInfo->nStartPos + size * nMenmb <= pNodeInfo->nEndPos)
    {
        fseek(pNodeInfo->fp, pNodeInfo->nStartPos, SEEK_SET);
        written = fwrite(ptr, size, nMenmb, pNodeInfo->fp);
        pNodeInfo->nStartPos += size * nMenmb;
    }
    else
    {
        fseek(pNodeInfo->fp, pNodeInfo->nStartPos, SEEK_SET);
        written = fwrite(ptr, 1, pNodeInfo->nEndPos - pNodeInfo->nStartPos + 1, pNodeInfo->fp);
        pNodeInfo->nStartPos = pNodeInfo->nEndPos;
    }
    return written;
}

int LibcurlHelper::OnProcess(void *ptr, double nTotalToDownload, double nAlreadyDownload
    , double nTotalUploadSize, double nAlreadyUpload)
{
    if (nTotalToDownload <= 0)return 0;
    CURL *pCurl = static_cast<CURL *>(ptr);
    char szTimeInfo[kSpeedInfo] = { 0 };
    double nSpeed;
    std::string unit = "B";
    // curl_get_info������curl_easy_perform֮�����
    auto code = curl_easy_getinfo(pCurl, CURLINFO_SPEED_DOWNLOAD, &nSpeed);
    if (CURLE_OK == code)
    {
        if (nSpeed != 0)
        {
            // Time remaining
            double leftTime = (nTotalToDownload - nAlreadyDownload) / nSpeed;
            int hours = leftTime / 3600;
            int minutes = (leftTime - hours * kSecondsPerHour) / kSecondsPerMinute;
            int seconds = leftTime - hours * kSecondsPerHour - minutes * kSecondsPerMinute;

#ifdef _WIN32
            sprintf_s(szTimeInfo, kSpeedInfo, "%02d:%02d:%02d", hours, minutes, seconds);
#else
            sprintf(szTimeInfo, "%02d:%02d:%02d", hours, minutes, seconds);
#endif
        }
        if (nSpeed > kGByte)
        {
            unit = "G";
            nSpeed /= kGByte;
        }
        else if (nSpeed > kMByte)
        {
            unit = "M";
            nSpeed /= kMByte;
        }
        else if (nSpeed >kKByte)
        {
            unit = "kB";
            nSpeed /= kKByte;
        }
        printf("[speed]:%.2f%s--", nSpeed, unit.c_str());
        printf("[left-time]:%s--", szTimeInfo);
    }
    else
    {
        printf("%s\n", curl_easy_strerror(code));
    }

    int nPersent = (int)(nAlreadyDownload / nTotalToDownload * 100);
    printf("[���ؽ���]:%0d%%\n", nPersent);
    // return bigger than 0, it means pause
    return 0;
}

LibcurlHelper::LibcurlHelper()
{
    m_bDebug = true;
    m_pFout = nullptr;
    m_nPart = 0;
}

LibcurlHelper::~LibcurlHelper()
{
}

int LibcurlHelper::Post(const HttpPara& paraHttp, const std::string& strData, std::string& strResponse)
{
    CURL *pCurl = curl_easy_init();
    if (nullptr == pCurl)
        return CURLE_FAILED_INIT;
    if (m_bDebug)
    {
        curl_easy_setopt(pCurl,CURLOPT_VERBOSE, 1);	//������Ϣ
        curl_easy_setopt(pCurl,CURLOPT_DEBUGFUNCTION, &LibcurlHelper::OnDebug);
    }
    curl_easy_setopt(pCurl,CURLOPT_URL, paraHttp.strUrl.c_str());
    curl_easy_setopt(pCurl,CURLOPT_POST, true);

    /* struct curl_slist pCurl_slist_append(struct curl_slist * list, const char * string);
    @ ���Http��Ϣͷ
    @ ����string����ʽΪname+": "+contents
    */
    curl_slist *pList = set_header(paraHttp.vecHeaders);
    /* CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter);
    @ �������Լ����ò���
    */
    curl_easy_setopt(pCurl,CURLOPT_SSL_VERIFYPEER, false);	//֤����֤
    curl_easy_setopt(pCurl,CURLOPT_SSL_VERIFYHOST, false);	//ssl�㷨��֤
    curl_easy_setopt(pCurl,CURLOPT_USERNAME, "test");
    curl_easy_setopt(pCurl,CURLOPT_PASSWORD, "1234");
    curl_easy_setopt(pCurl,CURLOPT_POSTFIELDS, strData.c_str());
    curl_easy_setopt(pCurl,CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(pCurl,CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnWriteString);
    curl_easy_setopt(pCurl,CURLOPT_WRITEDATA, (void*)&strResponse);
    curl_easy_setopt(pCurl,CURLOPT_FOLLOWLOCATION, 1); //�Ƿ�ץȡ��ת���ҳ��     
    curl_easy_setopt(pCurl,CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl,CURLOPT_CONNECTTIMEOUT, m_nTimeout);
    curl_easy_setopt(pCurl,CURLOPT_TIMEOUT, m_nTimeout);

    CURLcode res = curl_easy_perform(pCurl);
    if (pList)
    {
        curl_slist_free_all(pList);
    }
    curl_easy_cleanup(pCurl);

    return res;
}

int LibcurlHelper::UploadFile(const HttpPara& paraHttp, const std::string& strFile, std::string& strResponse)
{
    curl_httppost* pFirst = nullptr;
    curl_httppost *pLast = nullptr;
    curl_slist *pHeaderList = nullptr;
    curl_formadd(&pFirst, &pLast,
        CURLFORM_COPYNAME, "file",
        CURLFORM_FILE, strFile.c_str(),
        CURLFORM_CONTENTTYPE, "multipart/form-data",
        CURLFORM_END);
    curl_formadd(&pFirst, &pLast,
        CURLFORM_COPYNAME, "submit",
        CURLFORM_COPYCONTENTS, "send",
        CURLFORM_END);
    static const char buffer[] = "Expect:";
    pHeaderList = curl_slist_append(pHeaderList, buffer);

    CURL *pCurl = curl_easy_init();
    if (m_bDebug)
    {
        curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1);	//������Ϣ
        curl_easy_setopt(pCurl, CURLOPT_DEBUGFUNCTION, &LibcurlHelper::OnDebug);
    }
    curl_easy_setopt(pCurl, CURLOPT_URL, paraHttp.strUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaderList);
    curl_easy_setopt(pCurl, CURLOPT_HTTPPOST, pFirst);
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnWriteString);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void*)&strResponse);
    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 5);
    CURLcode res = curl_easy_perform(pCurl);
    int response_code = 0;
    curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &response_code);
    if (res != CURLE_OK || response_code != 200) {
        return false;
    }
    return 0;
}

int LibcurlHelper::PostFormData(const HttpPara& paraHttp, const curl_httppost* pData, const std::string& strResponse)
{
    CURL *pCurl = curl_easy_init();

    curl_easy_setopt(pCurl, CURLOPT_URL, paraHttp.strUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_HTTPPOST, pData);
    curl_easy_setopt(pCurl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnWriteString);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(pCurl, CURLOPT_USERNAME, paraHttp.strUser);
    curl_easy_setopt(pCurl, CURLOPT_PASSWORD, paraHttp.strPassword);

    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, m_nTimeout);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, m_nTimeout);
    CURLcode res = curl_easy_perform(pCurl);
    curl_easy_cleanup(pCurl);

    return res;
}

int LibcurlHelper::Get(const HttpPara& paraHttp, std::string& strResponse)
{
    CURLcode res;
    CURL *pCurl = curl_easy_init();
    if (nullptr == pCurl)
        return CURLE_FAILED_INIT;
    if (m_bDebug)
    {
        curl_easy_setopt(pCurl,CURLOPT_VERBOSE, 1);
        curl_easy_setopt(pCurl,CURLOPT_DEBUGFUNCTION, &LibcurlHelper::OnDebug);
    }
    curl_easy_setopt(pCurl,CURLOPT_URL, paraHttp.strUrl.c_str());    //���ʵ�URL
    curl_easy_setopt(pCurl,CURLOPT_READFUNCTION, NULL);     //
    curl_easy_setopt(pCurl,CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnWriteString); //���ݻص�����
    curl_easy_setopt(pCurl,CURLOPT_WRITEDATA, (void*)&strResponse); //���ݻص�������һ��Ϊbuffer�����ļ�fd
    /*
    * ������̶߳�ʹ�ó�ʱ�����ʱ��ͬʱ���߳�����sleep����wait�Ȳ�����
    * ������������ѡ�libcurl���ᷢ�źŴ�����wait�Ӷ����³����˳���
    */
    curl_easy_setopt(pCurl,CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(pCurl,CURLOPT_CONNECTTIMEOUT, m_nTimeout);
    curl_easy_setopt(pCurl,CURLOPT_TIMEOUT, m_nTimeout);
    //https
    curl_easy_setopt(pCurl,CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(pCurl,CURLOPT_SSL_VERIFYHOST, false);
    //curl_easy_setopt(pCurl,CURLOPT_CAINFO, "D:\\cert\\PC-12.crt");
    curl_easy_setopt(pCurl, CURLOPT_USERNAME, paraHttp.strUser);
    curl_easy_setopt(pCurl, CURLOPT_PASSWORD, paraHttp.strPassword);
    /* CURLcode curl_easy_perform(CURL *handle);
    @ ��ʼ����
    */
    if (res = curl_easy_perform(pCurl), CURLE_OK != res)
        printf("%s\n", curl_easy_strerror(res));
    curl_easy_cleanup(pCurl);
    return res;
}

int LibcurlHelper::DownloadFile(const HttpPara& paraHttp, const std::string& strFile)
{
    std::string strRemoteFile;
    int64_t nFileLength = get_download_file_length(paraHttp, strRemoteFile);
    FILE *fp;
    CURL *pCurl = curl_easy_init();
    CURLcode res = CURLE_OK;
    if (nullptr == pCurl) 
        return CURLE_FAILED_INIT;
    std::string strResultFile=generate_filename(paraHttp.strUrl, strFile, strRemoteFile);
    fopen_s(&fp, strResultFile.c_str(), "wb");
    res = curl_easy_setopt(pCurl, CURLOPT_URL, paraHttp.strUrl.c_str());
    if (CURLE_OK != res)
    {
        fclose(fp);
        curl_easy_cleanup(pCurl);
        printf("%s\n", curl_easy_strerror(res));
        return -1;
    }
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, false);
    std::string strUserInfo = paraHttp.strUser + ":" + paraHttp.strPassword;
    curl_easy_setopt(pCurl, CURLOPT_USERPWD, strUserInfo.c_str());
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnWriteFile);
    res = curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(pCurl, CURLOPT_PROGRESSFUNCTION, &LibcurlHelper::OnProcess);
    // get speed info by easy-handle
    curl_easy_setopt(pCurl, CURLOPT_XFERINFODATA, pCurl);
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
    //��ʼִ������
    printf("Downloadind file...\n");
    res = curl_easy_perform(pCurl);
    fclose(fp);
    if (CURLE_OK != res)
    {
        curl_easy_cleanup(pCurl);
        printf("%s\n", curl_easy_strerror(res));
        return res;
    }
    curl_easy_cleanup(pCurl);	//�ͷ��ڴ�
    printf("Download file finished\n");
    return res;
}

int LibcurlHelper::DownloadBigFile(const HttpPara& paraHttp, const std::string& strPath, const std::string& strFileName, int nThread)
{
    std::string strRemoteFile;
    int64_t nFileLength = get_download_file_length(paraHttp, strRemoteFile);
    if (0 >= nFileLength && strRemoteFile.empty())
    {
        printf("-------------------------Get file length and name failed----------------------\n");
        return -1;
    }
    std::string strResultFile = strPath + generate_filename(paraHttp.strUrl, strFileName, strRemoteFile);
    fopen_s(&m_pFout, strResultFile.c_str(), "wb");
    if (!m_pFout)
    {
        printf("Open file:%s failed\n", strResultFile.c_str());
        return -1;
    }
    int64_t nPartSize = nFileLength ? nFileLength / nThread : 0;
    m_nPart = nThread;
    for (int i = 0; i < nThread; i++)
    {
        CURL *pCurl = curl_easy_init();
        if (nullptr == pCurl)continue;
        FileNodeInfo *pNode(new FileNodeInfo());
        if (nFileLength > 0)
        {
            pNode->nStartPos = i * nPartSize;
            pNode->nEndPos = (i + 1) * nPartSize - 1;
            char range[64] = { 0 };
            snprintf(range, sizeof(range), "%lld-%lld", pNode->nStartPos, pNode->nEndPos);
            curl_easy_setopt(pCurl, CURLOPT_RANGE, range);
        }
        pNode->nPart = i;
        pNode->fp = m_pFout;
        pNode->pCurl = pCurl;

        // Download data
        curl_easy_setopt(pCurl, CURLOPT_URL, paraHttp.strUrl.c_str());
        curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, false);
        // set user and password info with user:password format, 
        // it passed in more situation
        //curl_easy_setopt(pCurl, CURLOPT_USERNAME, paraHttp.strUser.c_str());
        //curl_easy_setopt(pCurl, CURLOPT_PASSWORD, paraHttp.strPassword.c_str());
        std::string strUserInfo = paraHttp.strUser + ":" + paraHttp.strPassword;
        curl_easy_setopt(pCurl, CURLOPT_USERPWD, strUserInfo);
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &LibcurlHelper::OnMultiWriteFile);
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, pNode);
        curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(pCurl, CURLOPT_PROGRESSFUNCTION, &LibcurlHelper::OnProcess);
        // get speed info by easy-handle
        curl_easy_setopt(pCurl, CURLOPT_XFERINFODATA, pCurl);
        curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(pCurl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(pCurl, CURLOPT_LOW_SPEED_TIME, 5L);
        // resume download file
        //curl_easy_setopt(pCurl, CURLOPT_RESUME_FROM_LARGE, get_local_file_length(localFile));
        std::thread(std::bind(&LibcurlHelper::do_download, this, (void*)pNode)).detach();
    }
    while (m_nPart > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
    return 0;
}

void LibcurlHelper::get_header_type(int nType, std::string& strHeader)
{
    switch (nType)
    {
    case kContentTypeTxt:
        strHeader = "Content-Type: text/plain";
        break;
    case kContentTypeHtml:
        strHeader = "Content-Type: text/html";
        break;
    case kContentTypeXml:
        strHeader = "Content-Type: text/xml";
        break;
    case kContentTypeJson:
        strHeader = "Content-Type: application/json";
        break;
    case kContentTypeUrlencoded:
        strHeader = "Content-Type: x-www-form-urlencoded";
        break;
    case kContentTypeZip:
        strHeader = "Content-Type: application/zip";
    default:
        strHeader = "Content-Type: application/octet-stream";
        break;
    }
}

curl_slist* LibcurlHelper::set_header(const std::vector<std::string>& vecHeaders)
{
    curl_slist* pHeader = nullptr;
    for (const auto& item : vecHeaders)
    {
        pHeader = curl_slist_append(pHeader, item.c_str());
    }
    return pHeader;
}

int64_t LibcurlHelper::get_download_file_length(const HttpPara& paraHttp, std::string& strRemoteFileName)
{
    double downloadFileLenth = 0;
    CURL *pCurl = curl_easy_init();
    curl_easy_setopt(pCurl, CURLOPT_URL, paraHttp.strUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_HEADER, 1);    //ֻ��Ҫheaderͷ
    curl_easy_setopt(pCurl, CURLOPT_NOBODY, 1);    //����Ҫbody
    curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, &LibcurlHelper::OnWriteHeader);
    // store header of response header, just parse filename
    curl_easy_setopt(pCurl, CURLOPT_HEADERDATA, (void*)&strRemoteFileName);
    curl_easy_setopt(pCurl, CURLOPT_PROXY_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, false);
    /*std::string strUserInfo = paraHttp.strUser + ":" + paraHttp.strPassword;
    curl_easy_setopt(pCurl, CURLOPT_USERPWD, strUserInfo.c_str());*/
    //curl_easy_setopt(pCurl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36");
    CURLcode code = curl_easy_perform(pCurl);
    if (CURLE_OK == code)
    {
        // get file length
        code = curl_easy_getinfo(pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
        // get content type
        /*char *strContentType;
        code = curl_easy_getinfo(pCurl, CURLINFO_CONTENT_TYPE, &strContentType);
        printf("Content-type:%s\n", strContentType);*/
    }
    else
    {
        std::string strError = curl_easy_strerror(code);
        printf("Get remote file length failed:%s\n", strError.c_str());
    }
    curl_easy_cleanup(pCurl);
    return downloadFileLenth;
}

std::string LibcurlHelper::generate_filename(const std::string& strUrl, const std::string& strCustonFile, const std::string& strFileInfo)
{
    if (!strCustonFile.empty())
        return strCustonFile;
    if (!strFileInfo.empty())
        return strFileInfo;
    // exact filename from url
    int nPos = strUrl.rfind("/");
    int nLimitPos = strUrl.find("\\");
    int nSartPos = nPos > nLimitPos ? nPos : nLimitPos;
    return strUrl.substr(nSartPos + 1);
}

void LibcurlHelper::do_download(void *pData)
{
    FileNodeInfo *pNode = (FileNodeInfo *)pData;
    printf("Downloadind file...\n");
    CURLcode res = curl_easy_perform(pNode->pCurl);
    if (res != CURLE_OK)
    {
        printf("Download data failed,part:%lld,error[%d]:%s\n", pNode->nPart,res, curl_easy_strerror((CURLcode)res));
    }
    curl_easy_cleanup(pNode->pCurl);

    notify_download();
    delete pNode;
}

void LibcurlHelper::notify_download()
{
    std::lock_guard<std::mutex> lock(m_mtLockRecord);
    if (0 == --m_nPart) {
        printf("Download finished\n");
    }
    fclose(m_pFout);
}

long LibcurlHelper::get_local_file_length(const std::string& strFile)
{
    long nLength = 0;
    if (0 != _access(strFile.c_str(), 0))
        return nLength;
    std::fstream fin(strFile.c_str(), std::ios::binary | std::ios::in);
    if (fin.is_open())
    {
        fin.seekg(0, std::ios::end);
        std::streampos size = fin.tellp();
        fin.close();
        nLength = size;
    }
    return nLength;
}