// IKSettings.cpp: implementation of the IKSettings class.
//
//////////////////////////////////////////////////////////////////////


#include "IKCommon.h"
#include "IKSettings.h"
#include "IKFile.h"

#include "IKUtil.h"

//#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////

static IKSettings settings;

IKSettings * IKSettings::GetSettings ()
{
	return &settings;
}

// copy ctor



IKSettings::IKSettings()
:

	m_iResponseRate				(kSettingsRateHigh),

	m_bRequiredLiftOff			(false),

	m_bUseSystemRepeatSettings	(true),

	m_iKeySoundVolume			(kSettingsKeysound2),

	m_iRepeatRate				(kSettingsRateHigh),

	m_bRepeat					(true),

	m_bShowModeWarning			(true),

	m_bRepeatLatching			(false),

	m_iShiftKeyAction			(kSettingsShiftLatching),

	m_iMouseSpeed				(kSettingsRateHigh),

	m_bSmartTyping				(false),

	m_iDataSendRate				(kSettingsRateHigh),

	m_iMakeBreakRate			(kSettingsRateHigh),

	m_iIndicatorLights			(kSettings6lights),

	m_iMode						(kSettingsModeLastSentOverlay),

	m_iUseThisSwitchSetting		(0),

	// m_sUseThisOverlay			(TEXT(""))

	m_sLastSent					(TEXT("")),
	m_sLastSentBy				(TEXT("")),

	m_bButAllowOverlays			(true)


{
	SetToDefault();
}

IKSettings::~IKSettings()
{

}

IKSettings & IKSettings :: operator = ( const IKSettings &rhs )
{
	m_iResponseRate			= rhs.m_iResponseRate;
	m_bRequiredLiftOff		= rhs.m_bRequiredLiftOff;
	m_bUseSystemRepeatSettings	= rhs.m_bUseSystemRepeatSettings;
	m_iKeySoundVolume		= rhs.m_iKeySoundVolume;
	m_iRepeatRate			= rhs.m_iRepeatRate;
	m_bRepeat				= rhs.m_bRepeat;
	m_bRepeatLatching		= rhs.m_bRepeatLatching;
	m_iShiftKeyAction		= rhs.m_iShiftKeyAction;
	m_iMouseSpeed			= rhs.m_iMouseSpeed;
	m_bSmartTyping			= rhs.m_bSmartTyping;
	m_iDataSendRate			= rhs.m_iDataSendRate;
	m_iMakeBreakRate		= rhs.m_iMakeBreakRate;
	m_iIndicatorLights		= rhs.m_iIndicatorLights;
	//m_group					= rhs.m_group;
	//m_student				= rhs.m_student;
	m_iMode					= rhs.m_iMode;
	m_iUseThisSwitchSetting	= rhs.m_iUseThisSwitchSetting;
	m_sUseThisOverlay		= rhs.m_sUseThisOverlay;

	m_keyMap				= rhs.m_keyMap;

	m_sLastSent				= rhs.m_sLastSent;
	m_sLastSentBy			= rhs.m_sLastSentBy;

	m_bShowModeWarning		= rhs.m_bShowModeWarning;

	m_bButAllowOverlays		= rhs.m_bButAllowOverlays;

	return *this;
}

bool IKSettings :: operator != ( const IKSettings &rhs )
{
	return !(*this == rhs);
}

bool IKSettings :: operator == ( const IKSettings &rhs )
{
	return
		( m_iResponseRate		== rhs.m_iResponseRate		) &&
		( m_bRequiredLiftOff	== rhs.m_bRequiredLiftOff	) &&
		( m_bUseSystemRepeatSettings	== rhs.m_bUseSystemRepeatSettings	) &&
		( m_iKeySoundVolume		== rhs.m_iKeySoundVolume	) &&
		( m_iRepeatRate			== rhs.m_iRepeatRate		) &&
		( m_bRepeat				== rhs.m_bRepeat			) &&
		( m_bRepeatLatching		== rhs.m_bRepeatLatching	) &&
		( m_iShiftKeyAction		== rhs.m_iShiftKeyAction	) &&
		( m_iMouseSpeed			== rhs.m_iMouseSpeed		) &&
		( m_bSmartTyping		== rhs.m_bSmartTyping		) &&
		( m_iDataSendRate		== rhs.m_iDataSendRate		) &&
		( m_iMakeBreakRate		== rhs.m_iMakeBreakRate		) &&
		( m_iIndicatorLights	== rhs.m_iIndicatorLights	) &&
		//( m_group				== rhs.m_group				) &&
		//( m_student				== rhs.m_student			) &&
		( m_iMode				== rhs.m_iMode				) &&
		( m_iUseThisSwitchSetting	== rhs.m_iUseThisSwitchSetting ) &&
		( m_sUseThisOverlay		== rhs.m_sUseThisOverlay	) &&
		( m_sLastSent			== rhs.m_sLastSent	) &&
		( m_sLastSentBy			== rhs.m_sLastSentBy	) &&
		( m_bShowModeWarning    == rhs.m_bShowModeWarning ) &&
		( m_bButAllowOverlays   == rhs.m_bButAllowOverlays ) &&
		true;
}

bool IKSettings::Read(IKString filename)
{
	//  set to defaults first
	SetToDefault();

	//  load the file
	bool bLoaded = m_keyMap.Read(filename);

	// extract loaded values

	if (bLoaded)
	{
		m_iResponseRate			= GetIntValue(TEXT("Response_Rate"));
		m_iRepeatRate			= GetIntValue(TEXT("Repeat_Rate"));
		m_iShiftKeyAction		= GetIntValue(TEXT("Shift_Key_Action"));
		m_iMouseSpeed			= GetIntValue(TEXT("Mouse_Speed"));
		m_iDataSendRate			= GetIntValue(TEXT("Data_Send_Rate"));
		m_iMakeBreakRate		= GetIntValue(TEXT("Make_Break_Rate"));
		m_iIndicatorLights		= kSettings6lights ; // GetIntValue(TEXT("Indicator Lights"));

		m_bRequiredLiftOff		= GetBoolValue(TEXT("Required_Lift_Off"));
		m_bUseSystemRepeatSettings		= GetBoolValue(TEXT("Use_System_Repeat_Settings"));
		m_iKeySoundVolume		= GetIntValue (TEXT("Key_Sound_Volume"));
		m_bRepeat				= GetBoolValue(TEXT("Repeat"));
		m_bRepeatLatching		= GetBoolValue(TEXT("Repeat_Latching"));
		m_bSmartTyping			= GetBoolValue(TEXT("Smart_Typing"));

		m_iMode					= GetIntValue ( TEXT("Mode") );
		m_iUseThisSwitchSetting	= GetIntValue ( TEXT("Use_This_Switch_Setting"));
		m_sUseThisOverlay		= GetStringValue (TEXT("Use_This_Overlay"));
		m_sLastSent				= GetStringValue (TEXT("Last_Sent"));
		m_sLastSentBy			= GetStringValue (TEXT("Last_Sent_By"));

		m_bShowModeWarning		= GetBoolValue ( TEXT("Show_Mode_Warning") );
		m_bButAllowOverlays		= GetBoolValue ( TEXT("But_Allow_Overlays") );
	}

	return true;
}

void IKSettings::Write(IKString filename)
{
	StoreValues();

	//  save the file
	m_keyMap.Write(filename);
}

void IKSettings::Write()
{
	StoreValues();

	//  save the file
	m_keyMap.Write();
}

int  IKSettings::GetIntValue  (const TCHAR * pKey)
{
	IKString str = GetStringValue(pKey);

	return IKUtil::StringToInt(str);
}

void IKSettings::SetIntValue  (const TCHAR * pKey, int value)
{
	m_keyMap.SetValue(pKey,value);
}

bool IKSettings::GetBoolValue (const TCHAR * pKey)
{
	IKString str = GetStringValue(pKey);
	if(str.Compare(TEXT("true"))==0)

		return true;

	if(str.Compare(TEXT("TRUE"))==0)

		return true;


	return false;
}

void IKSettings::SetBoolValue (const TCHAR * pKey, bool bValue)
{
	if (bValue)
		SetStringValue ( pKey, TEXT("TRUE"));
	else
		SetStringValue ( pKey, TEXT("FALSE"));
}


IKString IKSettings::GetStringValue(const TCHAR * pKey)
{
	IKString str = m_keyMap.GetValueString(pKey);
	return str;
}

void IKSettings::SetStringValue ( const TCHAR * pKey, const TCHAR * pValue)
{
    IKString value = pValue;
	m_keyMap.Add(pKey, value);
}

void IKSettings::SetToDefault(bool bFeatureReset /*=false*/)
{
	m_iResponseRate				= kSettingsRateHigh;
	m_bRequiredLiftOff			= false;
	m_bUseSystemRepeatSettings	= true;
	m_iKeySoundVolume			= kSettingsKeysound2;
	m_iRepeatRate				= kSettingsRateHigh;
	m_bRepeat					= true;
	m_bRepeatLatching			= false;
	m_iShiftKeyAction			= kSettingsShiftLatching;
	m_iMouseSpeed				= kSettingsRateHigh;
	m_bSmartTyping				= false;
	m_iDataSendRate				= kSettingsRateHigh;
	m_iMakeBreakRate			= kSettingsRateHigh;
	m_iIndicatorLights			= kSettings6lights;
	m_iMode						= kSettingsModeLastSentOverlay;
	m_iUseThisSwitchSetting		= 0;
	m_sUseThisOverlay			= TEXT("");
	m_bShowModeWarning			= true;
	m_bButAllowOverlays			= true;

	if (!bFeatureReset)
	{
		m_sLastSent					= TEXT("");
		m_sLastSentBy				= TEXT("");
	}
}




void IKSettings::StoreValues()
{
	//  store new values

	SetIntValue  ( TEXT("Repeat_Rate"),					m_iRepeatRate);
	SetIntValue  ( TEXT("Shift_Key_Action"),			m_iShiftKeyAction);
	SetIntValue  ( TEXT("Response_Rate"),				m_iResponseRate);
	SetIntValue  ( TEXT("Mouse_Speed"),					m_iMouseSpeed);
	SetIntValue  ( TEXT("Data_Send_Rate"),				m_iDataSendRate);
	SetIntValue  ( TEXT("Make_Break_Rate"),				m_iMakeBreakRate);
	SetIntValue  ( TEXT("Indicator_Lights"),			m_iIndicatorLights);
	SetIntValue  ( TEXT("Make_Break_Rate"),				m_iDataSendRate);

	SetBoolValue ( TEXT("Required_Lift_Off"),			m_bRequiredLiftOff);
	SetBoolValue ( TEXT("Use_System_Repeat_Settings"),	m_bUseSystemRepeatSettings);
	SetIntValue  ( TEXT("Key_Sound_Volume"),			m_iKeySoundVolume);
	SetBoolValue ( TEXT("Repeat"),						m_bRepeat);
	SetBoolValue ( TEXT("Repeat_Latching"),				m_bRepeatLatching);
	SetBoolValue ( TEXT("Smart_Typing"),				m_bSmartTyping);

	SetIntValue    ( TEXT("Mode"), m_iMode );
	SetIntValue    ( TEXT("Use_This_Switch_Setting"),	m_iUseThisSwitchSetting );
	SetStringValue ( TEXT("Use_This_Overlay"),			m_sUseThisOverlay );
	SetStringValue ( TEXT("Last_Sent"),					m_sLastSent );
	SetStringValue ( TEXT("Last_Sent_By"),				m_sLastSentBy );

	SetBoolValue   ( TEXT("Show_Mode_Warning"),			m_bShowModeWarning );
	SetBoolValue   ( TEXT("But_Allow_Overlays"),		m_bButAllowOverlays );
	
}

//////////////////////////////////
//
//  copy ctor

IKSettings::IKSettings ( const IKSettings& src)
{
	m_iResponseRate			= src.m_iResponseRate;
	m_bRequiredLiftOff		= src.m_bRequiredLiftOff;
	m_bUseSystemRepeatSettings	= src.m_bUseSystemRepeatSettings;
	m_iKeySoundVolume		= src.m_iKeySoundVolume;
	m_iRepeatRate			= src.m_iRepeatRate;
	m_bRepeat				= src.m_bRepeat;
	m_bRepeatLatching		= src.m_bRepeatLatching;
	m_iShiftKeyAction		= src.m_iShiftKeyAction;
	m_iMouseSpeed			= src.m_iMouseSpeed;
	m_bSmartTyping			= src.m_bSmartTyping;
	m_iDataSendRate			= src.m_iDataSendRate;
	m_iMakeBreakRate		= src.m_iMakeBreakRate;
	m_iIndicatorLights		= src.m_iIndicatorLights;
	//m_group					= rhs.m_group;
	//m_student				= rhs.m_student;
	m_iMode					= src.m_iMode;
	m_iUseThisSwitchSetting	= src.m_iUseThisSwitchSetting;
	m_sUseThisOverlay		= src.m_sUseThisOverlay;

	m_keyMap				= src.m_keyMap;

	m_sLastSent				= src.m_sLastSent;
	m_sLastSentBy			= src.m_sLastSentBy;

	m_bShowModeWarning		= src.m_bShowModeWarning;
	m_bButAllowOverlays		= src.m_bButAllowOverlays;
}

