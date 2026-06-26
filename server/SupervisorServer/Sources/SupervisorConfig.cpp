#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"

#include "SupervisorConfig.h"
#include "../../MasterServer/Sources/NetPacketsServerBrowser.h"

CSupervisorConfig* gSupervisorConfig;

static const char* hostConfigFile = "ServerHost.cfg";

static bool IsAutoConfigValue(const std::string& value)
{
  return value.empty() || stricmp(value.c_str(), "auto") == 0;
}

CSupervisorConfig::CSupervisorConfig()
{
  const char* configFile = "SupervisorServer.cfg";
  const char* group      = "SupervisorServer";
  const char* hostGroup  = "ServerHost";

  if(_access(configFile, 4) != 0) {
    r3dError("can't open config file %s", configFile);
  }

  char hostPublicIp[128] = "localhost";
  char hostApiIp[128] = "localhost";
  char hostApiBaseUrl[128] = "/APS/";
  char hostApiServerKey[128] = "bvx425698dg6GsnxwedszF";
  int hostApiPort = 8080;
  int hostApiUseSSL = 0;

  if(_access(hostConfigFile, 4) == 0)
  {
    r3dscpy(hostPublicIp, r3dReadCFG_S(hostConfigFile, hostGroup, "publicIp", hostPublicIp));
    r3dscpy(hostApiIp, r3dReadCFG_S(hostConfigFile, hostGroup, "apiIp", hostPublicIp));
    r3dscpy(hostApiBaseUrl, r3dReadCFG_S(hostConfigFile, hostGroup, "apiBaseUrl", hostApiBaseUrl));
    r3dscpy(hostApiServerKey, r3dReadCFG_S(hostConfigFile, hostGroup, "apiServerKey", hostApiServerKey));
    hostApiPort = r3dReadCFG_I(hostConfigFile, hostGroup, "apiPort", hostApiPort);
    hostApiUseSSL = r3dReadCFG_I(hostConfigFile, hostGroup, "apiUseSSL", hostApiUseSSL);
  }

  masterPort_  = r3dReadCFG_I(configFile, group, "masterPort", SBNET_MASTER_PORT);
  masterIp_    = r3dReadCFG_S(configFile, group, "masterIp", hostPublicIp);
  if(IsAutoConfigValue(masterIp_))
    masterIp_ = hostPublicIp;
  
  serverGroup_ = r3dReadCFG_I(configFile, group, "serverGroup", GBNET_REGION_Unknown);
  serverName_  = r3dReadCFG_S(configFile, group, "serverName", "");
  
  // override serverName_ with our machine name
  char  sname[256] = {0};
  DWORD ssize = sizeof(sname);
  ::GetComputerName(sname, &ssize);
  serverName_ = sname;

  maxPlayers_  = r3dReadCFG_I(configFile, group, "maxPlayers", 1024);
  maxGames_    = r3dReadCFG_I(configFile, group, "maxGames", 32);
  portStart_   = r3dReadCFG_I(configFile, group, "portStart", SBNET_GAME_PORT);
  gameServerExe_ = r3dReadCFG_S(configFile, group, "gameServerExe", "GameServer.exe");
  externalIpStr_ = r3dReadCFG_S(configFile, group, "externalIp", hostPublicIp);
  if(IsAutoConfigValue(externalIpStr_))
    externalIpStr_ = hostPublicIp;
  externalIpAddr_= 0;
  
  // enable upload logs by default, it can be disabled by setting it to 0
  uploadLogs_ = r3dReadCFG_I(configFile, group, "uploadLogs", 1);

  webAPIDomainIP_ = r3dReadCFG_S(configFile, group, "webAPIDomainIP", hostApiIp);
  if(IsAutoConfigValue(webAPIDomainIP_))
    webAPIDomainIP_ = hostApiIp;

  webAPIDomainBaseURL_ = r3dReadCFG_S(configFile, group, "webAPIDomainBaseURL", hostApiBaseUrl);
  if(IsAutoConfigValue(webAPIDomainBaseURL_))
    webAPIDomainBaseURL_ = hostApiBaseUrl;

  webAPIDomainPort_ = r3dReadCFG_I(configFile, group, "webAPIDomainPort", hostApiPort);
  if(webAPIDomainPort_ == 0)
    webAPIDomainPort_ = hostApiPort;

  webAPIDomainUseSSL_ = r3dReadCFG_I(configFile, group, "webAPIDomainUseSSL", hostApiUseSSL) ? true : false;

  webAPIServerKey_ = r3dReadCFG_S(configFile, group, "webAPIServerKey", hostApiServerKey);
  if(IsAutoConfigValue(webAPIServerKey_))
    webAPIServerKey_ = hostApiServerKey;

  #define CHECK_I(xx) if(xx == 0)  r3dError("missing %s value", #xx);
  #define CHECK_S(xx) if(xx == "") r3dError("missing %s value", #xx);
  CHECK_I(masterPort_);
  CHECK_S(masterIp_);

  CHECK_I(serverGroup_);
  CHECK_S(serverName_);
  CHECK_I(maxPlayers_);
  CHECK_I(maxGames_);
  CHECK_I(portStart_);
  CHECK_S(gameServerExe_);

  CHECK_I(webAPIDomainPort_);
  CHECK_S(webAPIDomainIP_);
  CHECK_S(webAPIDomainBaseURL_);
  CHECK_S(webAPIServerKey_);
  #undef CHECK_I
  #undef CHECK_S
  
  if(_access(gameServerExe_.c_str(), 4) != 0) {
    r3dError("can't access game server '%s'\n", gameServerExe_.c_str());
  }
  
  ParseExternalIpAddr();
  
  return;
}

void CSupervisorConfig::ParseExternalIpAddr()
{
  // call WSAStartup()
  WORD versionRequested = MAKEWORD(2, 2);
  WSADATA wsaData;
  WSAStartup(versionRequested, &wsaData);

  if(externalIpStr_.length() == 0) {
    externalIpAddr_ = 0;
    return;
  }
  
  const char* ipname = externalIpStr_.c_str();
  struct hostent* hostEntry = gethostbyname(ipname);
  if(hostEntry == NULL || hostEntry->h_addrtype != AF_INET) 
  {
    unsigned long host = inet_addr(ipname);
    if(host == INADDR_NONE) {
      r3dError("can not parse external ip address %s\n", ipname);
    }

    externalIpAddr_ = host;
    return;
  }
  
  DWORD* ip2 = (DWORD*)hostEntry->h_addr_list[1];
  if(ip2 != NULL) 
    r3dError("external ip %s resolved to multiple IP addresses\n", ipname);
    
  externalIpAddr_ = *(DWORD*)hostEntry->h_addr_list[0];
  r3dOutToLog("External Ip: %s\n", inet_ntoa(*(in_addr*)&externalIpAddr_));
  return;
}
