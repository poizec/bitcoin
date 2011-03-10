#ifndef NOMRESOLV_H__
#define NOMRESOLV_H__

#include <string>
#include "curl/curl.h"

using std::string;

class NameResolutionService
{
public:
    NameResolutionService();
    ~NameResolutionService();

    string FetchAddress(const string& strRef, string& strAddy);
    string PushAddress(const string& strRef, const string& strPassword, const string& strNewaddy, string& strStatus);
    string ChangePassword(const string& strRef, const string& strPassword, const string& strNewPassword, string& strStatus);
private:
    class PostVariables
    {
    public:
        PostVariables();
        ~PostVariables();
        bool Add(const string& strKey, const string& strVal);
        curl_httppost* operator()() const;
    private:
        curl_httppost *pBegin, *pEnd;
    };

    static void ExplodeRef(const string& strRef, string& strNickname, string& strDomain);
    bool Perform();
    string MakeRequest(const string& strRef, const string& strPassword, const string& strReqname, const string& strArgument, string& strStatus);

    char pErrorBuffer[CURL_ERROR_SIZE];  
    string strBuffer;
    CURL *curl;
};

#endif

