// IKMessage.cpp: implementation of the IKMessage class.
//
//////////////////////////////////////////////////////////////////////


#include "IKCommon.h"
#include "IKMessage.h"
#include "IKUtil.h"

#include "SharedMemory.h"
#include <string.h>

#ifdef PLAT_MACINTOSH
#include <stdio.h>
#endif

#ifdef PLAT_WINDOWS

static SharedMemBlock ControlPanelChannel = {"6789",0,0,6789,0,0,"ControlPanelChannel"};
static SharedMemBlock EngineChannel       = {"8765",0,0,8765,0,0,"EngineChannel"};
static SharedMemBlock SystrayChannel      = {"4321",0,0,4321,0,0,"SystrayChannel"};

static SharedMemBlock ControlPanelChannelVista = {"Global\\6789",0,0,6789,0,0,"Global\\ControlPanelChannel"};
static SharedMemBlock EngineChannelVista       = {"Global\\8765",0,0,8765,0,0,"Global\\EngineChannel"};
static SharedMemBlock SystrayChannelVista      = {"Global\\4321",0,0,4321,0,0,"Global\\SystrayChannel"};

#endif

#ifdef PLAT_MACINTOSH
static SharedMemBlock ControlPanelChannel = {"6789",0,0,6789,0};
static SharedMemBlock EngineChannel       = {"8765",0,0,8765,0};
static SharedMemBlock MenuChannel         = {"7654",0,0,7654,0};
static SharedMemBlock BalloonChannel      = {"5432",0,0,5432,0};
#endif

static int defaultTimeout       = 1000;

#define CLAIM_OFFSET 0
#define COMMAND_OFFSET 4
#define RESPONSE_OFFSET 2050

static int nclaim = 0;

static int nsend    = 0;
static int nreceive = 0;
static int nrespond = 0;

class Counter
{
public:
	Counter(int *pVal) {m_pVal = pVal; (*m_pVal)++;}
	~Counter() {(*m_pVal)--;}
	int *m_pVal;
};

static SharedMemBlock * ChooseMemoryBlock(IKString channel)
{
#ifdef PLAT_WINDOWS

	if (IKUtil::IsWinVistaOrGreater())
	{
		if (channel.CompareNoCase(TEXT("engine"))==0)
			return &EngineChannelVista;
		if (channel.CompareNoCase(TEXT("system tray"))==0)
			return &SystrayChannelVista;
		return &ControlPanelChannelVista;
	}
	else
	{
		if (channel.CompareNoCase(TEXT("engine"))==0)
			return &EngineChannel;
		if (channel.CompareNoCase(TEXT("system tray"))==0)
			return &SystrayChannel;
		return &ControlPanelChannel;
	}

#else

	if (channel.CompareNoCase(TEXT("engine"))==0)
		return &EngineChannel;
	if (channel.CompareNoCase(TEXT("balloon"))==0)
		return &BalloonChannel;
	if (channel.CompareNoCase(TEXT("menu"))==0)
		return &MenuChannel;
	return &ControlPanelChannel;

#endif

}


static void CleanupChannel(SharedMemBlock* channel)
{
	::ClearSharedMemory(channel);
}

static void ReleaseClaim(SharedMemBlock* myBlock)
{
    while(!::LockSharedMemory(myBlock)){}

    while(::ReadSharedMemoryByte(myBlock,CLAIM_OFFSET)!=0)
        ::WriteSharedMemoryByte(myBlock,CLAIM_OFFSET,0);

    ::ClearSharedMemory(myBlock);
    ::UnlockSharedMemory(myBlock);

	nclaim--;
	IKASSERT(nclaim==0);

}

static int GetDataLength (SharedMemBlock* myBlock, int offset)
{
    int i;
	int datalength=0;
    if(::LockSharedMemory(myBlock)>0)
	{
        for(i=0;i<4;i++)
           ((unsigned char *)&datalength)[i]=::ReadSharedMemoryByte(myBlock,offset+i+4);

        ::UnlockSharedMemory(myBlock);
    }

	return datalength;
}

static int ReadData (SharedMemBlock* myBlock,int offset, int *response, int *datalength, void *data )
{
	int theResponse;
	int theDataLength;
    int iResponse = 0;
    int iDataLength = 0;

    int i;
    if(::LockSharedMemory(myBlock)>0)
	{
        for(i=0;i<4;i++)
           ((unsigned char *)&theResponse)[i]= ::ReadSharedMemoryByte(myBlock,offset+i);

        for(i=0;i<4;i++)
           ((unsigned char *)&theDataLength)[i]=::ReadSharedMemoryByte(myBlock,offset+i+4);

#ifdef PLAT_MACINTOSH
#if TARGET_RT_BIG_ENDIAN
#else
        iResponse = theResponse;
        iDataLength = theDataLength;
        //  intel
//      theResponse   = CFSwapInt32(theResponse);
//      theDataLength = CFSwapInt32(theDataLength);
#endif
#endif
        
        if(theDataLength>0)
		{
            for(i=0;i<theDataLength;i++)
               ((unsigned char *)data)[i]= ::ReadSharedMemoryByte(myBlock,offset+i+8);
        }

        ::UnlockSharedMemory(myBlock);
		
		*response = theResponse;
		*datalength = theDataLength;
		
        if(*response == 0)
            return kResponseError;

        for(i = 0;i<8;i++)
            ::WriteSharedMemoryByte(myBlock,offset+i,0);
 
        return kResponseNoError;
    }

	return kResponseError;
}

static int WriteData (SharedMemBlock* myBlock, int command, int offset, int datalength, void *data )
{
	int theCommand    = command;
	int theDataLength = datalength;
	
#ifdef PLAT_MACINTOSH
#if TARGET_RT_BIG_ENDIAN
#else
	//  intel
//	theCommand    = CFSwapInt32(theCommand);
//	theDataLength = CFSwapInt32(theDataLength);
#endif
#endif

	int ntries = 0;
	int i;
	while (ntries<=20)
	{
        if(::LockSharedMemory(myBlock)>0)
		{
			//write the command
			for (i=0;i<sizeof(theCommand);i++)
				::WriteSharedMemoryByte(myBlock,offset+i,((unsigned char *)&theCommand)[i]);

			for (i=0;i<sizeof(theDataLength);i++)
				::WriteSharedMemoryByte(myBlock,offset+i+4,((unsigned char *)&theDataLength)[i]);
       
			if (data!=NULL && datalength>0)
			{
				for(i=0;i<datalength;i++)
					::WriteSharedMemoryByte(myBlock,offset+i+8,((unsigned char *)data)[i]);
			}
   
			while(::UnlockSharedMemory(myBlock)<1) {}

			return kResponseNoError;
		}
		IKUtil::Sleep(50);
    }
        
	return kResponseError;
}

static int ClaimChannel (SharedMemBlock* myBlock, int timeoutMS)
{

	//  get start time for timeout calc
	unsigned int start = IKUtil::GetCurrentTimeMS();
	while (1==1)
	{
		//  are we timed out?
		if (timeoutMS>0)
		{
			unsigned int now = IKUtil::GetCurrentTimeMS();
			if ((int)(now-start) > timeoutMS)
				return kResponseTimeout;
		}

		//  attempt a claim
		int myClaim = 0;
		if (::LockSharedMemory(myBlock)>0)
		{
			//  is it already claimed?
			int isClaimed = ReadSharedMemoryByte(myBlock,CLAIM_OFFSET);
			if (isClaimed==0)
			{
				//  no, let's claim it
				::WriteSharedMemoryByte(myBlock,CLAIM_OFFSET,1);
				myClaim=1;
			}
			
			::UnlockSharedMemory(myBlock);
		}

		//  were we successful?
		if(myClaim)
			break;	//  yes

		//  no.  If no timeout, return error immediately.
		if (timeoutMS<=0)
			return kResponseError;

		//  no, wait a bit and try again.
		IKUtil::Sleep(25);
	}

	//  if we get here, we've claimed.
	nclaim++;
	IKASSERT(nclaim==1);

	CleanupChannel(myBlock);

	return kResponseNoError;

}

int IKMessage::SendTO ( IKString channel, int command, void *dataOut, int dataOutLen, void *dataIn, int *dataInLen, int timeoutMS, bool bCheckAlive )
{

//    if (command != -1067 && command != 1051 && command != 1015) {
//        NSLog(@"IKMessage::SendTo %s command: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(command));
//    }
    
	Counter x(&nsend);

	SharedMemBlock * tempBlock = ChooseMemoryBlock ( channel );
        
	//  is the receiver there?
	if (bCheckAlive)
		if (!IsOwnerAlive(channel))
			return kResponseNoServer;
		
	//  claim the channel, with timeout

	int claimResponse = ClaimChannel (tempBlock, timeoutMS);
    ////NSLog(@"IKMessage::SendTo %s claimResponse: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(claimResponse));
	if (claimResponse != kResponseNoError)
		return claimResponse;

	//  send the message
	int response = WriteData (tempBlock, command, COMMAND_OFFSET, dataOutLen, dataOut );
	if (response==kResponseNoError)
	{
		// get the response with timeout
		unsigned int start = IKUtil::GetCurrentTimeMS();
		while (1==1)
		{
			unsigned int now = IKUtil::GetCurrentTimeMS();
			if ((int)(now-start) > timeoutMS)
			{
				//  let go, there may be no receiver.
				ReleaseClaim(tempBlock);
                ////NSLog(@"IKMessage::SendTo %s kResponseTimeout", (TCHAR *) channel);
				return kResponseTimeout;
			}

			int l = 0;

			int datalength = GetDataLength ( tempBlock,RESPONSE_OFFSET );
			if (datalength<256)
				datalength = 256;

			BYTE *mydata = new BYTE[datalength];

			int err = ReadData (tempBlock,RESPONSE_OFFSET, &response, &l, mydata);
			if (err==kResponseNoError)
			{
				//  give the caller the data
				if (dataInLen)
					*dataInLen = l;
				if (dataIn)
				{
					for (int i=0;i<l;i++)
						((BYTE *)dataIn)[i] = mydata[i];
					((BYTE *)dataIn)[l] = 0;
					((BYTE *)dataIn)[l+1] = 0;
				}
				delete [] mydata;
				break;
			}

			delete [] mydata;
			
			IKUtil::Sleep(20);
		}

	}

	//  release the claim
	ReleaseClaim(tempBlock);

	//  return the result
    //NSLog(@"IKMessage::SendTo %s response: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(response));
	return response;
}


int IKMessage::Send ( IKString channel, int command, void *dataOut, int dataOutLen, void *dataIn,int *dataInLen, bool bCheckAlive )
{
	return IKMessage::SendTO ( channel, command, dataOut, dataOutLen, dataIn, dataInLen, defaultTimeout, bCheckAlive  );
}


int IKMessage::Receive ( IKString channel, int *command, void *data, int maxdata, int *dataread )
{
	Counter x(&nreceive);

	SharedMemBlock *tempBlock = ChooseMemoryBlock ( channel );

    int isClaimed = 0;
    if(::LockSharedMemory(tempBlock)>0)
	{
        isClaimed = ReadSharedMemoryByte(tempBlock,CLAIM_OFFSET);
        ::UnlockSharedMemory(tempBlock);
	}

    if(isClaimed)
	{
		//  read the message
		int response = ReadData (tempBlock,COMMAND_OFFSET, command, dataread, data );

		if (response==kResponseNoError)
		{
            ////NSLog(@"IKMessage::Receive %s command: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(*command));
            
			//  there was a message.  Get rid of it before returning.
			if (LockSharedMemory(tempBlock)>0)
			{
				int i;
				for (i=0;i<8;i++)
					::WriteSharedMemoryByte(tempBlock,COMMAND_OFFSET+i,0);
				::UnlockSharedMemory(tempBlock);
			}
			//TRACE6("Receive channel=%s command=%s returns %s, snd=%d rcv=%d rsp=%d",
				//(TCHAR *)channel,(TCHAR *)GetMessageName(*command),(TCHAR *)GetMessageName(response),
				//nsend,nreceive,nrespond);

            ////NSLog(@"IKMessage::Receive %s response: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(response));

			return response;
		}
		else
		{
			//  error reading message.
			//TRACE6("Receive channel=%s command=%s returns %s, snd=%d rcv=%d rsp=%d",
				//(TCHAR *)channel,(TCHAR *)GetMessageName(*command),(TCHAR *)GetMessageName(kResponseNoCommand),
				//nsend,nreceive,nrespond);
			return kResponseNoCommand;
		}

	}

	//  no message.
	return kResponseNoCommand;
}


int IKMessage::Respond ( IKString channel, int response, void *data, int datalength )
{
    ////NSLog(@"IKMessage::Respond %s response: %s", (TCHAR *) channel, (TCHAR *)GetMessageName(response));

	Counter x(&nrespond);

	SharedMemBlock* tempBlock = ChooseMemoryBlock ( channel );

	int myresponse = WriteData ( tempBlock, response,RESPONSE_OFFSET, datalength, data );

	return myresponse;
}


IKString IKMessage::GetMessageName(int message)
{
	switch (message)
	{
		case kQueryShouldCallOldInTT:
			return TEXT("kQueryShouldCallOldInTT");
			break;

		case kQuerySetStudent:
			return TEXT("kQuerySetStudent");
			break;

		case kQueryIsUSBIntelliKeysConnected:
			return TEXT("kQueryIsUSBIntelliKeysConnected");
			break;

		case kQueryShowControlPanel:
			return TEXT("kQueryShowControlPanel");
			break;
		
		case kQueryShutdown:
			return TEXT("kQueryStopServer");
			break;
		
		case kQuerySendOverlay:
			return TEXT("kQuerySendOverlay");
			break;
	
		case kQueryIsIntellikeysConnected:
			return TEXT("kQueryIsIntellikeysConnected");
			break;

		case kQuerySendSettings:
			return TEXT("kQuerySendSettings");
			break;

		case kQueryGetSettings:
			return TEXT("kQueryGetSettings");
			break;
		
		case kQueryIsStandardOverlayInPlace:
			return TEXT("kQueryIsStandardOverlayInPlace");
			break;

		case kQuerySendOverlayName:
			return TEXT("kQuerySendOverlayName");
			break;

		case kQueryIsIntellikeysOn:
			return TEXT("kQueryIsIntellikeysOn");
			break;

		case kQuerySendOverlayWithName:
			return TEXT("kQuerySendOverlayWithName");
			break;

		case kDisableGetResourcePatchOneTime:
			return TEXT("kDisableGetResourcePatchOneTime");
			break;
	
		case kResponseCallOldInTT:
			return TEXT("kResponseCallOldInTT");
			break;
		
		case kResponseDoNotCallOldInTT:
			return TEXT("kResponseDoNotCallOldInTT");
			break;

		case kResponseHandshake:
			return TEXT("kResponseHandshake");
			break;
				
		case kResponseTimeout:
			return TEXT("kResponseTimeout");
			break;
	
		case kResponseError:
			return TEXT("kResponseError");
			break;
	
		case kResponseNoServer:
			return TEXT("kResponseNoServer");
			break;
	
		case kResponseNotEnoughtData:
			return TEXT("kResponseNotEnoughtData");
			break;
	
		case kResponseFileError:
			return TEXT("kResponseFileError");
			break;
	
		case kResponseNotConnected:
			return TEXT("kResponseNotConnected");
			break;
	
		case kResponseConnected:
			return TEXT("kResponseConnected");
			break;

		case kResponseRegistered:
			return TEXT("kResponseRegistered");
			break;
	
		case kResponseAlreadyRegistered:
			return TEXT("kResponseAlreadyRegistered");
			break;

		case kResponseRegistrationError:
			return TEXT("kResponseRegistrationError");
			break;

		case kResponseNoError:
			return TEXT("kResponseNoError");
			break;

		case kResponseStandardOverlayIsInPlace:
			return TEXT("kResponseStandardOverlayIsInPlace");
			break;

		case kResponseStandardOverlayIsNotInPlace:
			return TEXT("kResponseStandardOverlayIsNotInPlace");
			break;

		case kResponseNotOn:
			return TEXT("kResponseNotOn");
			break;

		case kResponseOn:
			return TEXT("kResponseOn");
			break;

		case kResponseNoCommand:
			return TEXT("kResponseNoCommand");
			break;

		case kQueryUSBIntelliKeysArray:
			return TEXT("kQueryUSBIntelliKeysArray");
			break;

		case kQueryLastSentOverlay:
			return TEXT("kQueryLastSentOverlay");
			break;

		case kQueryGlobalData:
			return TEXT("kQueryGlobalData");
			break;

		case kResponseUSBIntelliKeysArray:
			return TEXT("kResponseUSBIntelliKeysArray");
			break;

		case kResponseGlobalData:
			return TEXT("kResponseGlobalData");
			break;
            
		case kQuerySendOverlayProxy:
			return TEXT("kQuerySendOverlayProxy");
			break;
            
		default:
			{
				char cstr[256];
				MySprintf(cstr,"Unknown message %d",message);
				IKString s(cstr);
				return s;
			}
			break;
	}

}

bool IKMessage::IsOwnerAlive(IKString channel)
{

    if (channel.CompareNoCase(TEXT("balloon"))==0)
	{
		IKString name;
		name = DATAS(TEXT("balloon_name_1"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("balloon_name_2"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("balloon_name_3"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		return false;
	}
	
    if (channel.CompareNoCase(TEXT("engine"))==0)
	{
		IKString name;
		name = DATAS(TEXT("engine_name_1"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("engine_name_2"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("engine_name_3"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		return false;
	}

    if (channel.CompareNoCase(TEXT("control panel"))==0)
	{
		IKString name;
		name = DATAS(TEXT("control_panel_name_1"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("control_panel_name_2"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("control_panel_name_3"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		return false;
	}


#ifdef PLAT_WINDOWS

    if (channel.CompareNoCase(TEXT("system tray"))==0)
	{
		IKString name;
		name = DATAS(TEXT("system_tray_name_1"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("system_tray_name_2"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		name = DATAS(TEXT("system_tray_name_3"),TEXT(""));
		if (!name.IsEmpty())
			if (IKUtil::IsAppRunning(name))
				return true;
		return false;
	}

#endif

	return true;
}


void IKMessage::Initialize()
{
#ifdef PLAT_WINDOWS

	if (IKUtil::IsWinVistaOrGreater())
	{
		::CreateSharedMemory(&EngineChannelVista);
		::CreateSharedMemory(&SystrayChannelVista);
		::CreateSharedMemory(&ControlPanelChannelVista);
	}
	else
	{
		::CreateSharedMemory(&EngineChannel);
		::CreateSharedMemory(&SystrayChannel);
		::CreateSharedMemory(&ControlPanelChannel);
	}

#else

	::CreateSharedMemory(&EngineChannel);
	::CreateSharedMemory(&BalloonChannel);
	::CreateSharedMemory(&MenuChannel);
	::CreateSharedMemory(&ControlPanelChannel);

#endif

}

void IKMessage::CheckChannels()
{
#ifdef PLAT_MACINTOSH_X

	if (ControlPanelChannel.address==0)
		::CreateSharedMemory(&ControlPanelChannel);

	if (EngineChannel.address==0)
		::CreateSharedMemory(&EngineChannel);
		
	if (BalloonChannel.address==0)
		::CreateSharedMemory(&BalloonChannel);
	
	if (MenuChannel.address==0)
		::CreateSharedMemory(&MenuChannel);

#endif
}
