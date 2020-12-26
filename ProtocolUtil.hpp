#pragma once
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <strings.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>

#define BACKLOG      5
#define BUFF_NUM     1024
#define WEBROOT      "wwwroot"
#define HOMEPAGE     "index.html"
// ERROR NUMBER
#define NORMAL       0
#define WARNING      1
#define ERROR        2
// EXCEPT HTML
#define HTML_404     "wwwroot/ExceptHtml/404.html"
#define HTML_500     "wwwroot/ExceptHtml/500.html"
#define HTML_503     "wwwroot/ExceptHtml/503.html"
#define HTML_UNKNOWN "wwwroot/ExceptHtml/unknown.html"

const char* ErrLevel[] = {
  "Normal",
  "Warning",
  "Error",
};

void Log(std::string strMsg, int nLevel, std::string strFile, int nLine)
{
  std::cout << "HttpLog file:" << strFile << " " << nLine << " | " << strMsg << ". [" << ErrLevel[nLevel] << "]" << std::endl;
}

#define LOG(msg, level) Log(msg, level, __FILE__, __LINE__);

class HttpRequest;
class HttpResponse;
class Connect;

class Util
{
  public:
    static void MakeKV(const std::string& strLine, std::string& strKey, std::string& strVal)
    {
      // 将string分解成kv
      std::size_t szPos = strLine.find(": ");
      strKey = strLine.substr(0, szPos);
      strVal = strLine.substr(szPos + 2);
    }

    static std::string IntToString(const int& nNum)
    {
      std::stringstream ss;
      ss << nNum;
      return ss.str();
    }

    static std::string CodeToDesc(int nCode)
    {
      switch (nCode)
      {
        case 200:
          return "OK";
        case 400:
          return "Bad Request";
        case 404:
          return "Not Found";
        case 500:
          return "Internal Server Error";
        default:
          break;
      }
      return "Unknow";
    }

    static std::string SuffixToContent(std::string& strSuffix)
    {
      if (strSuffix == ".css")
      {
        return "text/css";
      }
      if (strSuffix == ".js")
      {
        return "application/x-javascript";
      }
      if (strSuffix == ".html" || strSuffix == ".htm")
      {
        return "text/html";
      }
      if (strSuffix == ".jpg")
      {
        return "application/x-jpg";
      }
      if (strSuffix == ".mp3")
      {
        return "audio/mp3";
      }

      return "text/html";
    }

    static std::string CodeToExceptFile(int nCode)
    {
      switch (nCode)
      {
        case 404:
          return HTML_404;
        case 500:
          return HTML_500; 
        case 503:
          return HTML_503;
        default:
          return "";
      }
    }

    static int FileSize(const std::string& strPath)
    {
      struct stat st;
      stat(strPath.c_str(), &st);
      return st.st_size;
    }
};

class SocketApi
{
  public:
    static int Socket()
    {
      int nSock = socket(AF_INET, SOCK_STREAM, 0);
      if (nSock < 0)
      {
        LOG("socket error!", ERROR);
        exit(2);
      }
      int nOpt = 1;
      setsockopt(nSock, SOL_SOCKET, SO_REUSEADDR, &nOpt, sizeof(nOpt));
      return nSock;
    }

    static void Bind(int nSock, int nPort)
    {
      struct sockaddr_in local;
      bzero(&local, sizeof(local));
      local.sin_family = AF_INET;
      local.sin_port = htons(nPort);
      local.sin_addr.s_addr = htonl(INADDR_ANY); // 动态绑定

      if (bind(nSock, (struct sockaddr*)&local, sizeof(local)))
      {
        LOG("bind error!", ERROR);
        exit(3);
      }
    }

    static void Listen(int nSock)
    {
      if(listen(nSock, BACKLOG) < 0)
      {
        LOG("listen error", ERROR);
        exit(4);
      }
    }

    static int Accept(int nListen_sock, std::string& strIp, int& nPort)
    {
      struct sockaddr_in peer;
      socklen_t len = sizeof(peer);
      int nSock = accept(nListen_sock, (struct sockaddr*)&peer, &len);
      if (nSock < 0)
      {
        LOG("accept error", WARNING);
        return -1;
      }

      strIp = inet_ntoa(peer.sin_addr);
      nPort = ntohs(peer.sin_port);
      return nSock;
    }
};

class HttpResponse
{
  public:
    HttpResponse()
      : mStrBlank("\r\n")
      , mCode(200)
      , mResourceSize(0)
    {

    }
    ~HttpResponse()
    {

    }

    int& Code()
    {
      return mCode;
    }

    void SetPath(const std::string& strPath)
    {
      mStrPath = strPath;
    }

    int GetResourceSize()
    {
      return mResourceSize;
    }
  
    void SetResourceSize(int nSize)
    {
      mResourceSize = nSize;
    }
    
    std::string& GetPath()
    {
      return mStrPath;
    }

    void MakeStatusLine()
    {
      mStrStatusLine  = "HTTP/1.0";
      mStrStatusLine += " ";
      mStrStatusLine += Util::IntToString(mCode);
      mStrStatusLine += " ";
      mStrStatusLine += Util::CodeToDesc(mCode); 
      mStrStatusLine += "\r\n";
    }
    
    void MakeResponseHeader()
    {
      std::string strLine;
      std::string strSuffix;
      strLine = "Content-Type: ";
      std::size_t szPos = mStrPath.rfind('.');
      if (std::string::npos != szPos)
      {
        strSuffix = mStrPath.substr(szPos); 
        transform(strSuffix.begin(), strSuffix.end(), strSuffix.begin(), ::tolower);
      }
      strLine += Util::SuffixToContent(strSuffix);
      strLine += "\r\n";
      mVecResponseHeader.push_back(strLine);

      strLine  = "Content-Length: ";
      strLine += Util::IntToString(mResourceSize);
      strLine += "\r\n";
      mVecResponseHeader.push_back(strLine);

      strLine  = "\r\n";
      mVecResponseHeader.push_back(strLine);
    }

    void MakeResponseText(HttpRequest* req)
    {
      
    }

  public:
    // 基本协议字段
    std::string mStrStatusLine;
    std::vector<std::string> mVecResponseHeader;
    std::string mStrBlank;
    std::string mStrResponseText;
  public:
    int mCode;
    std::string mStrPath; // 响应的资源
    int mResourceSize;
};

class HttpRequest
{
  public:
    HttpRequest()
      : mStrBlank("\r\n")
      , mStrPath(WEBROOT)
      , mIsCGI(false)
    {

    }
    ~HttpRequest()
    {
    
    }

    void RequesetLineParse()
    {
      // GET /x/y HTTP/1.1\n
      std::stringstream ss(mStrRequestLine);  
      ss >> mStrMethod >> mStrUri >> mStrVersion; 
    }

    void UriParse()
    {
      // 仅实现GET/POST
      if (mStrMethod == "GET")
      {
        std::size_t szPos = mStrUri.find('?');
        // 判断是否带参数
        if (szPos != std::string::npos)
        {
          mIsCGI = true;
          mStrPath += mStrUri.substr(0, szPos);
          mStrQuery = mStrUri.substr(szPos + 1);
        }
        else 
        {
          mStrPath += mStrUri;
        }
      }
      else 
      {
        mIsCGI = true;
        mStrPath += mStrUri;
      }

      if (mStrPath[mStrPath.size() - 1] == '/')
      {
        mStrPath += HOMEPAGE; // 拼接 wwwroot/index
      }
      std::cout <<"*" <<  mStrPath << std::endl;
    }

    void RequeseHeaderParse()
    {
      std::string strKey, strVal;
      for (auto it = mVecReqHeader.begin(); it != mVecReqHeader.end(); it++)
      {
        Util::MakeKV(*it, strKey, strVal);
        mMapHeader.insert({strKey, strVal});
      }
    }

    bool RequestMethodLegal()
    {
      // transform()
      if (mStrMethod != "GET" && mStrMethod != "POST")
      {
        return false;
      }
      return true;
    }

    int RequestPathLegal(HttpResponse* rsp)
    {
      int nSize = 0;
      // 判断这个文件是否存在
      struct stat st;
      if (stat(mStrPath.c_str(), &st) < 0)
      {
        // 文件不存在
        // 404 not find
        return 404;  
      }
      else
      {
        nSize = st.st_size; 
        // 再判断
        if (S_ISDIR(st.st_mode))
        {
          // 如果是目录就返回默认网页
          mStrPath += "/";
          mStrPath += HOMEPAGE;
          stat(mStrPath.c_str(), &st);
          nSize = st.st_size;
        }
        else if (st.st_mode & S_IXUSR ||
                 st.st_mode & S_IXGRP ||
                 st.st_mode & S_IXOTH)
        {
          // 请求内容是一个可执行文件 
          // cgi执行
          mIsCGI = true;
        }
        else 
        {
          // TODO
        }
      }

      rsp->SetPath(mStrPath); 
      rsp->SetResourceSize(nSize);
      return 200;
    }

    bool IsNeedRecv()
    {
      return mStrMethod == "POST" ? true : false;
    }

    int ContentLength()
    {
      int nContentLength = -1;
      std::string strLength = mMapHeader["Content-Length"];
      std::stringstream ss(strLength);
      ss >> nContentLength;
      return nContentLength;
    }

    bool IsCgi()
    {
      return mIsCGI;
    }

    std::string GetParam()
    {
      if (mStrMethod == "GET")
      {
        return mStrQuery;
      }
      else 
      {
        return mStrRequestText;
      }
    }

  public:
    // 基本协议字段
    std::string mStrRequestLine;
    std::vector<std::string> mVecReqHeader;
    std::string mStrBlank;
    std::string mStrRequestText;
  private:
    // 解析字段
    std::string mStrMethod; 
    std::string mStrUri;     // path?arg
    std::string mStrVersion;
    std::string mStrPath;
    std::string mStrQuery;
    std::unordered_map<std::string, std::string> mMapHeader;

    bool mIsCGI;
};


class Connect
{
  public:
    Connect(int nSock)
      : mSock(nSock)
    {

    }

    ~Connect()
    {

    }

    int RecvOneLine(std::string& strLine)
    {
      char strBuff[BUFF_NUM];
      int nIndex = 0;
      char cChar = 'A';
      while (cChar != '\n' && nIndex < BUFF_NUM - 1)
      {
        ssize_t szSize = recv(mSock, &cChar, 1, 0);
        if (szSize > 0)
        {
          // 处理\r\n的问题
          // 将\r\n和\r转换为\n
          if (cChar == '\r')
          {
            recv(mSock, &cChar, 1, MSG_PEEK); 
            if (cChar == '\n')
            {
              recv(mSock, &cChar, 1, 0);
            }
            else
            {
              cChar = '\n';
            }
          }
          strBuff[nIndex++] = cChar;
        }
        else 
        {
          break;
        }
      }
      strBuff[nIndex] = 0;
      strLine = strBuff;
      return nIndex;
    }

    void RecvRequestHeader(std::vector<std::string>& vecHeader)
    {
      std::string strLine = "X";
      while (strLine != "\n")
      {
        RecvOneLine(strLine);
        if (strLine != "\n")
        {
          vecHeader.push_back(strLine);
        }
      }
    }
    
    void RecvText(std::string& strText, int nLen)
    {
      char cChar;
      for (int nPos = 0; nPos < nLen; nPos++)
      {
        recv(mSock, &cChar, 1, 0);
        strText.push_back(cChar);
      }
    }

    void SendStatusLine(HttpResponse* rsp)
    { 
      std::string& strLine = rsp->mStrStatusLine;
      send(mSock, strLine.c_str(), strLine.size(), 0);
    }

    void SendHeader(HttpResponse* rsp)
    {
      std::vector<std::string>& vecHeader = rsp->mVecResponseHeader;
      for (auto it = vecHeader.begin(); it != vecHeader.end(); it++)
      {
        send(mSock, it->c_str(), it->size(), 0);
      }
    }

    void SendText(HttpResponse* rsp, bool cgi)
    {
      if (!cgi)
      {
        std::string& strPath = rsp->GetPath();
        int nFd = open(strPath.c_str(), O_RDONLY);
        if (nFd < 0)
        {
          LOG("Open file error!", WARNING);
          return;
        }
        std::cout << "sendfile " << std::endl;
        sendfile(mSock, nFd, NULL, rsp->GetResourceSize());
        close(nFd);
      }
      else 
      {
        std::string& strRspText = rsp->mStrResponseText;
        send(mSock, strRspText.c_str(), strRspText.size(), 0);
      }

    }

    void ClearRequest()
    {
      // 清除缓冲区
      std::string strLine;
      while (strLine != "\n")
      {
        RecvOneLine(strLine);
      }
    }
  private:
    int mSock;
};


class Entry
{
  public:
    static int ProcessNonCgi(Connect* conn, HttpRequest* req, HttpResponse* rsp)
    {
      rsp->MakeStatusLine(); 
      rsp->MakeResponseHeader();
      // rsp->MakeResponseText(req);

      std::cout << "start conn!" << std::endl;
      conn->SendStatusLine(rsp);
      conn->SendHeader(rsp);
      conn->SendText(rsp, false);

      return 200;
    }
    
    static int ProcessCgi(Connect* conn, HttpRequest* req, HttpResponse* rsp)
    {
      int input[2];
      int output[2];
      pipe(input);
      pipe(output);

      std::string strBin = rsp->GetPath();
      std::string strParam = req->GetParam();
      std::string strParamSize = "HTTP_CONTENT_LENGTH=";
      std::size_t szSize = strParam.size();
      strParamSize += Util::IntToString(szSize);
      std::string& strResponseText = rsp->mStrResponseText;

      pid_t pid = fork();
      if (pid < 0)  
      {
        LOG("fork error!", WARNING);
        return 500;
      }
      else if (pid == 0)
      {
        close(input[1]);
        close(output[0]);

        putenv((char*)strParamSize.c_str());

        dup2(input[0], 0);
        dup2(output[1], 1);

        execl(strBin.c_str(), strBin.c_str(), NULL);
        exit(1);
      }
      else 
      {
        close(input[0]);
        close(output[1]);

        char cChar;
        for (std::size_t i = 0; i < strParam.size(); i++)
        {
          cChar = strParam[i];
          write(input[1], &cChar, 1);
        }

        waitpid(pid, NULL, 0);
        
        while (read(output[0], &cChar, 1) > 0)
        {
          strResponseText.push_back(cChar);
        }
      
        rsp->MakeStatusLine();
        rsp->SetResourceSize(strResponseText.size()); 
        rsp->MakeResponseHeader();

        conn->SendStatusLine(rsp);
        conn->SendHeader(rsp);
        conn->SendText(rsp, true);
      }
      return 200;
    }

    static int ProcessResponse(Connect* conn, HttpRequest* req, HttpResponse* rsp)
    {
      if (req->IsCgi())
      {
        LOG("MakeResponse Use Cgi", NORMAL);
        return ProcessCgi(conn, req, rsp);
      }
      else 
      {
        return ProcessNonCgi(conn, req, rsp);
      }
    }

    static void HandlerRequest(int nSock)
    {
      pthread_detach(pthread_self());

      Connect* conn = new Connect(nSock);
      HttpRequest* req = new HttpRequest();
      HttpResponse* rsp = new HttpResponse();
      int& nCode = rsp->Code(); 
      conn->RecvOneLine(req->mStrRequestLine);
      req->RequesetLineParse();
  
      if (!req->RequestMethodLegal())
      {
        nCode = 400; 
        conn->ClearRequest();
        LOG("Request method is not legal", WARNING);
        goto END;
      }

      req->UriParse();

      if ((nCode = req->RequestPathLegal(rsp)) != 200)
      {
        conn->ClearRequest(); 
        LOG("File is not exist!" , WARNING);
        goto END;
      }

      conn->RecvRequestHeader(req->mVecReqHeader);
      req->RequeseHeaderParse();
      if (req->IsNeedRecv())
      {
        conn->RecvText(req->mStrRequestText, req->ContentLength());
      }

      ProcessResponse(conn, req, rsp);
      
END:
      if (nCode != 200)
      {
        // 出错处理
        // 构建异常响应
        std::string strExceptPath = Util::CodeToExceptFile(nCode);
        int nRs = Util::FileSize(strExceptPath);
        rsp->SetPath(strExceptPath); 
        rsp->SetResourceSize(nRs);
        ProcessNonCgi(conn, req, rsp);
      }

      delete conn;
      delete req;
      delete rsp;
      close(nSock);

      return;
    }
};
