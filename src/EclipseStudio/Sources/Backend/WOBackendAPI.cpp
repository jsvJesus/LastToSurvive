#include "r3dPCH.h"
#include "r3d.h"
#include "CkGzip.h"
#include "CkBinData.h"

#include "GameCode/UserProfile.h"

#include "WOBackendAPI.h"

const char*	gDomainBaseUrl= "/APS/";
int		gDomainPort   = 8080; // PAX_BUILD - change to 80 and no SSL
bool		gDomainUseSSL = false;
	
CWOBackendReq::CWOBackendReq(const char* url)
{
	Init(url);
}

CWOBackendReq::CWOBackendReq(const CUserProfile* prof, const char* url)
{
	Init(url);

	AddSessionInfo(prof->CustomerID, prof->SessionID);
}

void CWOBackendReq::Init(const char* url)
{
	resp_ = NULL;

	savedUrl_ = url;
	resultCode_ = 0;
	bodyStr_ = "";
	bodyLen_ = 0;

	http.put_ConnectTimeout(30);
	http.put_ReadTimeout(60);
	http.put_FollowRedirects(true);

	char fullUrl[512] = {};

	if (
		url &&
		url[0] != '/'
	)
	{
		sprintf_s(
			fullUrl,
			sizeof(fullUrl),
			"%s%s",
			gDomainBaseUrl,
			url
		);
	}
	else
	{
		sprintf_s(
			fullUrl,
			sizeof(fullUrl),
			"%s",
			url
				? url
				: ""
		);
	}

	req.put_HttpVerb("POST");

	req.put_ContentType(
		"application/x-www-form-urlencoded"
	);

	req.put_Path(fullUrl);
	req.put_Utf8(true);
}

CWOBackendReq::~CWOBackendReq()
{
	SAFE_DELETE(resp_);
}

void CWOBackendReq::AddSessionInfo(DWORD id, DWORD sid)
{
	r3d_assert(id);
	
	AddParam("s_id",  (int)id);
	AddParam("s_key", (int)sid);
}

void CWOBackendReq::AddParam(const char* name, const char* val)
{
	req.AddParam(name, val);
}

void CWOBackendReq::AddParam(const char* name, int val)
{
	char	buf[1024];
	sprintf(buf, "%d", val);
	AddParam(name, buf);
}

int CWOBackendReq::ParseResult(CkHttpResponse* resp)
{
	if (resp == NULL)
	{
		r3dOutToLog(
			"[Backend] ParseResult received NULL response\n"
		);

		return 8;
	}

	if (resp->get_StatusCode() != 200)
	{
		CkBinData ErrorBodyData;
		CkByteData ErrorBodyBytes;

		resp->GetBodyBd(
			ErrorBodyData
		);

		ErrorBodyData.GetBinary(
			ErrorBodyBytes
		);

		ErrorBodyBytes.appendChar(0);

		r3dOutToLog(
			"[Backend] HTTP %d returned for %s\n",
			resp->get_StatusCode(),
			savedUrl_
				? savedUrl_
				: "<unknown>"
		);

		r3dOutToLog(
			"[Backend] HTTP response body: %s\n",
			ErrorBodyBytes.getSize() > 1
				? reinterpret_cast<const char*>(
					ErrorBodyBytes.getData()
				)
				: "<empty>"
		);

		return 8;
	}
	
	// NOTE: we can't use getBodyStr() because it skip zeroes inside answer body
	CkBinData bodyData;
	resp->GetBodyBd(bodyData);
	bodyData.GetBinary(data_);
	data_.appendChar(0);
	
	// if context is gzipped, uncompress it
	if(data_.getSize() >= 2 && data_.getByte(0) == '$' && data_.getByte(1) == '1')
	{
		CkGzip gzip;
		// remove header and decompress
		CkByteData udata;
		data_.removeChunk(0, 2);
		if(!gzip.UncompressMemory(data_, udata)) {
			r3dOutToLog("WO_API: decompress failed\n");
			return 9;
		}
		//r3dOutToLog("WO_API: decompressed %d->%d\n", data_.getSize(), udata.getSize());

		// swap data with uncompressed one
		data_ = udata;
		data_.appendChar(0);
	}
	bodyStr_ = (const char*)data_.getData();
	bodyLen_ = data_.getSize() - 1;
	
	// validate http answer
	if(bodyLen_ < 4)
	{
		r3dOutToLog("WO_API: too small answer\n");
		return 9;
	}
	
	// check if we got XML header "<?xml"
	if(bodyStr_[0] == '<' && bodyStr_[1] == '?' && bodyStr_[2] == 'x')
	{
		return 0;
	}
	
	// check for 'WO_x' header
	if(bodyStr_[0] != 'W' || bodyStr_[1] != 'O' || bodyStr_[2] != '_')
	{
		r3dOutToLog("WO_API: wrong header: %s\n", bodyStr_);
		return 9;
	}

	int resultCode = bodyStr_[3] - '0';
	
	// offset body content past header
	bodyStr_ += 4;
	bodyLen_ -= 4;

	// parse result code
	switch(resultCode)
	{
		case 0:	// operation ok
			break;
			
		case 1:	// session not valid
			r3dOutToLog("WO_API: session disconnected\n");
			break;

		default:
			r3dOutToLog("WO_API: failed with error code %d %s\n", resultCode, bodyStr_);
			break;
	}

	return resultCode;
}

bool CWOBackendReq::Issue()
{
	SAFE_DELETE(resp_);

	const char* ApiHost =
		g_api_ip
			? g_api_ip->GetString()
			: NULL;

	if (
		!ApiHost ||
		!ApiHost[0]
	)
	{
		r3dOutToLog(
			"[Backend] g_api_ip is empty\n"
		);

		resultCode_ = 8;
		return false;
	}

	char RequestPath[512] = {};

	if (
		savedUrl_ &&
		savedUrl_[0] == '/'
	)
	{
		sprintf_s(
			RequestPath,
			sizeof(RequestPath),
			"%s",
			savedUrl_
		);
	}
	else
	{
		sprintf_s(
			RequestPath,
			sizeof(RequestPath),
			"%s%s",
			gDomainBaseUrl,
			savedUrl_
				? savedUrl_
				: ""
		);
	}

	r3dOutToLog(
		"[Backend] POST %s://%s:%d%s\n",
		gDomainUseSSL
			? "https"
			: "http",
		ApiHost,
		gDomainPort,
		RequestPath
	);

	r3dOutToLog(
		"[Backend] Chilkat timeouts: connect=%ld sec, read=%ld sec\n",
		http.get_ConnectTimeout(),
		http.get_ReadTimeout()
	);

	const float RequestStartTime =
		r3dGetTime();

	resp_ =
		http.SynchronousRequest(
			ApiHost,
			gDomainPort,
			gDomainUseSSL,
			req
		);

	const float RequestTime =
		r3dGetTime() -
		RequestStartTime;

	if (!resp_)
	{
		const char* ErrorText =
			http.lastErrorText();

		r3dOutToLog(
			"[Backend] Request %s returned no HTTP response "
			"after %.3f sec\n",
			savedUrl_
				? savedUrl_
				: "<unknown>",
			RequestTime
		);

		r3dOutToLog(
			"[Backend] Chilkat error:\n%s\n",
			ErrorText && ErrorText[0]
				? ErrorText
				: "<no error text>"
		);

		resultCode_ = 8;
		return false;
	}

	resultCode_ =
		ParseResult(
			resp_
		);

	r3dOutToLog(
		"[Backend] Request %s completed in %.3f sec, "
		"HTTP=%d, result=%d\n",
		savedUrl_
			? savedUrl_
			: "<unknown>",
		RequestTime,
		resp_->get_StatusCode(),
		resultCode_
	);

	return resultCode_ == 0;
}

void CWOBackendReq::ParseXML(pugi::xml_document& xmlFile)
{
	pugi::xml_parse_result parseResult = xmlFile.load_buffer_inplace((void*)bodyStr_, bodyLen_);
	if(!parseResult)
		r3dError("Failed to parse server XML, error: %s", parseResult.description());
}