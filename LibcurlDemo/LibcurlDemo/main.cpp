#include <iostream>
#include "LibcurlHelper.h"
#include "httpclient.h"

void DownloadFile()
{
    HttpPara para;
    std::string strResponse;
    LibcurlHelper clientHttp;
    // download file
    //para.strUrl = "https://codeload.github.com/curl/curl/zip/master";
    para.strUrl = "http://127.0.0.1:10008/download/2019-12-13/296b1d7b-6a26-4fde-9ac600604e659700.zip";
    //para.strUrl = "http://127.0.0.1:10008/download/2019-12-12/3c4a4890-93b1-4641-834afe59ce57cd24.jpg";
    //para.strUrl = "https://192.168.2.66/svn/%E7%A5%9E%E7%9B%BE/SDBusiness/branches/WiseEyes_v3_saida/projects/SaaS/src/interface/service/heatmap.cpp";
    ///*para.strUser = "tredy6t";
    //para.strPassword = "wite_chen0";*/
    /*para.strUser = "fei.chen";
    para.strPassword = "fei.chen.00";*/
    //std::string strRstFile = "test.zip";
    clientHttp.DownloadFile(para, strResponse);
}

void DownloadBigFile()
{
    HttpPara para;
    std::string strResponse;
    LibcurlHelper clientHttp;
    // download file
    std::string strUrl = "https://127.0.0.1/svn/QQ_Project/ChatServer/ChatServer/SocketServer.cpp";
    para.strUrl = strUrl;
    para.strUser = "Wite";
    para.strPassword = "wite_chen";
    std::string strRstFile = "server.txt";
    std::string strPath = "";
    clientHttp.DownloadBigFile(para, strPath, strRstFile);
}

void LocalTest()
{
    HttpPara para;
    std::string strResponse;
    LibcurlHelper clientHttp;
    std::string strUrl;
    int nCode = 0;
    //para.strUrl = "";
    para.strUrl = "https://codeload.github.com/curl/curl/zip/master";
    //para.strUrl = "https://download.hello.com/files/hello/hello.zip";
    //para.strUrl = "https://192.168.2.66/svn/%E7%A5%9E%E7%9B%BE/SDBusiness/branches/WiseEyes_v3_saida/projects/SaaS/src/interface/service/heatmap.cpp";
    //para.strUrl = "ed2k://|file|cn_windows_10_multiple_editions_x64_dvd_6848463.iso|4303300608|94FD861E82458005A9CA8E617379856A|/";
    std::string strPath = "";
    /*para.strUser = "fei.chen";
    para.strPassword = "fei.chen.00";*/
    /*para.strUser = "1058778041@qq.com";
    para.strPassword = "wite_chen0";*/
    para.strUser = "1058778041@qq.com";
    para.strPassword = "wite_chen0";

    //nCode = clientHttp.DownloadFile(para, strFile, strResponse);

    nCode = clientHttp.DownloadBigFile(para);
}

void DownloadMsiFile()
{
    HttpPara para;
    std::string strResponse;
    LibcurlHelper clientHttp;
    std::string strUrl;
    int nCode = 0;
    //std::string strFile = "test.txt";
    std::string strFile = "test.msi";
    para.strUrl = "http://slproweb.com/download/Win32OpenSSL_Light-1_1_1d.msi";
    //para.strUrl = "ed2k://|file|cn_windows_10_multiple_editions_x64_dvd_6848463.iso|4303300608|94FD861E82458005A9CA8E617379856A|/";
    std::string strPath = "";
    /*para.strUser = "1058778041@qq.com";
    para.strPassword = "wite_chen";*/
    nCode = clientHttp.DownloadBigFile(para, strPath, strFile, 1);

}

void UploadFile()
{
    HttpPara para;
    std::string strResponse;
    LibcurlHelper clientHttp;
    para.strUrl = "http://127.0.0.1:10011/upload";
    //para.strUrl = "http://127.0.0.1:8002/upload";
    std::string strFile = "test.jpg";
    //std::string strFile = "curlDemo.zip";
    int code = clientHttp.UploadFile(para, strFile, strResponse);

    std::cout << "code:" << code << "result:" << strResponse << std::endl;
}

void UploadFile1()
{
    HttpClient clientHttp;
    std::string strResponse;
    std::string strUrl = "http://127.0.0.1:10008/upload/";
    //para.strUrl = "http://127.0.0.1:8002/upload";
    std::string strFile = "test.jpg";
    int code = clientHttp.UploadFile(strUrl, strFile, strResponse);

    std::cout << "code:" << code << "result:" << strResponse << std::endl;
}

int main()
{
    //DownloadFile();
    //DownloadBigFile();
    //LocalTest();

    //DownloadMsiFile();

    UploadFile();
    //DownloadFile();
    //UploadFile1();

    getchar();
    return 0;
}