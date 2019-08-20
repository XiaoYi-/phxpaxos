/*
Tencent is pleased to support the open source community by making 
PhxPaxos available.
Copyright (C) 2016 THL A29 Limited, a Tencent company. 
All rights reserved.

Licensed under the BSD 3-Clause License (the "License"); you may 
not use this file except in compliance with the License. You may 
obtain a copy of the License at

https://opensource.org/licenses/BSD-3-Clause

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
implied. See the License for the specific language governing 
permissions and limitations under the License.

See the AUTHORS file for names of contributors. 
*/

#include "election.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include "phxpaxos/options.h"
#include <unistd.h>

using namespace phxelection;
using namespace phxpaxos;
using namespace std;

int parse_ipport(const char * pcStr, NodeInfo & oNodeInfo)
{
    char sIP[32] = {0};
    int iPort = -1;

    int count = sscanf(pcStr, "%[^':']:%d", sIP, &iPort);
    if (count != 2)
    {
        return -1;
    }

    oNodeInfo.SetIPPort(sIP, iPort);

    return 0;
}

int parse_ipport_list(const char * pcStr, NodeInfoList & vecNodeInfoList)
{
    string sTmpStr;
    int iStrLen = strlen(pcStr);

    for (int i = 0; i < iStrLen; i++)
    {
        if (pcStr[i] == ',' || i == iStrLen - 1)
        {
            if (i == iStrLen - 1 && pcStr[i] != ',')
            {
                sTmpStr += pcStr[i];
            }
            
            NodeInfo oNodeInfo;
            int ret = parse_ipport(sTmpStr.c_str(), oNodeInfo);
            if (ret != 0)
            {
                return ret;
            }

            vecNodeInfoList.push_back(oNodeInfo);

            sTmpStr = "";
        }
        else
        {
            sTmpStr += pcStr[i];
        }
    }

    return 0;
}

int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        printf("%s <myip:myport> <node0_ip:node_0port,node1_ip:node_1_port,node2_ip:node2_port,...>\n", argv[0]);
        return -1;
    }

    NodeInfo oMyNode;
    if (parse_ipport(argv[1], oMyNode) != 0)
    {
        printf("parse myip:myport fail\n");
        return -1;
    }

    NodeInfoList vecNodeInfoList;
    if (parse_ipport_list(argv[2], vecNodeInfoList) != 0)
    {
        printf("parse ip/port list fail\n");
        return -1;
    }

    PhxElection oElection(oMyNode, vecNodeInfoList);
    int ret = oElection.RunPaxos();
    if (ret != 0)
    {
        return -1;
    }


    while (true)
    {
        ::sleep(1);
        uint64_t llVersion = 0;
        NodeInfo oMasterNode = oElection.GetMasterWithVersion(llVersion);
        printf("master: nodeid %lu version %lu ip %s port %d\n", 
                oMasterNode.GetNodeID(), llVersion, oMasterNode.GetIP().c_str(), oMasterNode.GetPort());
    }


    return 0;
}



#include <map>
#include <algorithm>

#include "baseservice.h"
#include "MmUtil.h"
#include "xyssl.h"

#include "iBase64.h"
#include "utils/iMd5.h"
#include "UtilHelper.h"
#include "ClientVersion.h"

#include "accountclient.h"
#include "frequencesvrclient.h"

#include "guildhelper.h"
#include "SecurityHelper.h"
#include "FrequenceHelper.h"

#include "logiccallback.h"

#include "auth.pb.h"
using namespace accounthelper;
using namespace Comm;
using namespace mm;
using namespace ga;

class  GeneralSendVoiceVerifyCodeService : public BaseService < GeneralSendVoiceVerifyCodeReq, GeneralSendVoiceVerifyCodeResp>
{
public:
    GeneralSendVoiceVerifyCodeService(){};
    virtual ~GeneralSendVoiceVerifyCodeService(){};

    virtual int execute();

} ;

SERVICE_ENTRY(GeneralSendVoiceVerifyCodeService, "GeneralSendVoiceVerifyCodeService", CMD_GeneralSendVoiceVerifyCode);

int GeneralSendVoiceVerifyCodeService::execute() {
    Comm::LogErr("DEBUG: GeneralSendVoiceVerifyCodeService : _serviceInfo : %s", _serviceInfo->dump().c_str());
    Comm::LogErr("DEBUG: GeneralSendVoiceVerifyCodeService : req : %s", _req.ShortDebugString().c_str());
    int ret = 0;
    const unsigned int appid = protocol::GetAppIdFromTerminalType(_serviceInfo->terminalType);

    std::string reqPhone= "";
    if(_req.has_phone())
    {
        reqPhone = _req.phone();
    }

    unsigned int reqVerifycodeOpType = (ga::ENUM_SENSTIVE_OP_GUILD_CHAIRMAN_OP);
    if(_req.has_verify_op_type())
    {
        reqVerifycodeOpType = _req.verify_op_type();
    }

    std::string verifycodeOpDesc;
    security_helper::CGeneralVerfiyCodeHelper::GenVerifyCodeOpType2Desc(reqVerifycodeOpType, verifycodeOpDesc);

    if(reqPhone.empty())
    {
        Account::UserResp respUser;
        AccountClient().GetUserByUid(_serviceInfo->uid, respUser);
        if(respUser.phone().empty())
        {
            logErr("GeneralSendVoiceVerifyCodeService GetUserByUid uid %u phone empty", _serviceInfo->uid, ret);
            setFailRetCode(ERR_ACCOUNT_NO_BINDED_PHONE_TO_USER);
            return 0;
        }
        reqPhone = respUser.phone();
    }
    
    ret = security_helper::CGeneralVerfiyCodeHelper::SendVerifyCode(_serviceInfo->uid, appid,
        reqVerifycodeOpType, verifycodeOpDesc,
        _serviceInfo->sessionKey, reqPhone);
    
    if (ret != 0)
    {
        logErr("GeneralSendVoiceVerifyCodeService uid %u  CGeneralVerfiyCodeHelper::SendVerifyCode fail %d", _serviceInfo->uid, ret);
        setFailRetCode(ret);
        return 0;
    }
    
    setSuccessRetCode(0);

    logDebug("DEBUG: GeneralSendVerifyCodeService::execute, resp :%s",
            _resp.ShortDebugString().c_str());

    return 0;
}