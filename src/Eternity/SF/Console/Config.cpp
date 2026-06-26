#include "r3dPCH.h"
#include "r3d.h"
#include "Config.h"


#include "../CmdProcessor/CmdProcessor.h"

///////////////////////////////////////////////////////////////////////////////

#define REG_VAR( var, value, flag )					CmdVar * var	= NULL;
#define REG_VAR_C( var, value, minv, maxv, flag )	CmdVar * var	= NULL;
	#include "../../Eternity/SF/Console/Vars.h"
#undef REG_VAR
#undef REG_VAR_C

const char * Va( const char * str, ... );

void ExecVarIni( const char* path )
{
	char command[ 1024 ];

	_snprintf( command, sizeof command - 1, "exec %s", path );

	g_pCmdProc->Execute( command, CommandProcessor::eExecPrior_Immediate );

	g_pCmdProc->FlushBuffer();
}

static void ApplyServerHostConfig()
{
	const char* configFile = "ServerHost.cfg";
	const char* group = "ServerHost";

	if(_access(configFile, 4) != 0)
		return;

	char publicIp[128] = "";
	char serverIp[128] = "";
	char apiIp[128] = "";

	r3dscpy(publicIp, r3dReadCFG_S(configFile, group, "publicIp", ""));
	r3dscpy(serverIp, r3dReadCFG_S(configFile, group, "serverIp", publicIp));
	r3dscpy(apiIp, r3dReadCFG_S(configFile, group, "apiIp", publicIp));

	if(g_serverip && serverIp[0])
		g_serverip->SetString(serverIp);
	if(g_api_ip && apiIp[0])
		g_api_ip->SetString(apiIp);
}

//--------------------------------------------------------------------------------------------------------
void RegisterAllVars()
{

	g_pCmdProc = new CommandProcessor;
	g_pCmdProc->Init();

#define REG_VAR( var, value, flag )					var = g_pCmdProc->Register( #var, value, flag );
#define REG_VAR_C( var, value, minv, maxv, flag )	var = g_pCmdProc->Register( #var, value, minv, maxv, flag );
	#include "../../Eternity/SF/Console/Vars.h"
#undef REG_VAR
#undef REG_VAR_C

	ExecVarIni( "game.ini" );
	ExecVarIni( "local.ini" );
	ApplyServerHostConfig();
}

//--------------------------------------------------------------------------------------------------------
void UnregisterAllVars()
{
	g_pCmdProc->Release();
	SAFE_DELETE( g_pCmdProc );
}
