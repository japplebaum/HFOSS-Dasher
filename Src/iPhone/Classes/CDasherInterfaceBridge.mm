/*
 *  CDasherInterfaceBridge.cpp
 *  Dasher
 *
 *  Created by Alan Lawrence on 18/3/2009.
 *
 */

#import "CDasherInterfaceBridge.h"
#import "COSXSettingsStore.h"
#import "ModuleManager.h"
#import "DasherUtil.h"
#import "DasherAppDelegate.h"
#import "Event.h"
#import "CalibrationController.h"

#import "../Common/Common.h"

#import <iostream>

#import <fcntl.h>

#import <sys/stat.h>

using namespace std;


CDasherInterfaceBridge::CDasherInterfaceBridge(DasherAppDelegate *aDasherApp) : dasherApp(aDasherApp) {
  Realize();
}

void CDasherInterfaceBridge::CreateModules() {
	//create the default set...a good idea?!?!
	CDasherInterfaceBase::CreateModules();

	RegisterModule(m_pMouseDevice = 
				new CIPhoneMouseInput(m_pEventHandler, m_pSettingsStore));
	RegisterModule(m_pTiltDevice = 
				new CIPhoneTiltInput(m_pEventHandler, m_pSettingsStore));
	RegisterModule(m_pMixDevice =
				   new CMixedInput(m_pEventHandler, m_pSettingsStore, m_pMouseDevice, m_pTiltDevice, MIXED_INPUT));
	RegisterModule(m_pReverseMix =
				   new CMixedInput(m_pEventHandler, m_pSettingsStore, m_pTiltDevice, m_pMouseDevice, REVERSE_MIX));
	RegisterModule(m_pPlainDragFilter = new CPlainDragFilter(m_pEventHandler, m_pSettingsStore, this, 9, "Hold-down filter"));
	RegisterModule(m_pOneDFilter =
				   new CIPhone1DFilter(m_pEventHandler, m_pSettingsStore, this, 16));
	RegisterModule(m_pPolarFilter = 
				   new CIPhonePolarFilter(m_pEventHandler, m_pSettingsStore, this, 17));
	SetDefaultInputMethod(GetModuleByName("Stylus Control"));
	SetDefaultInputDevice(m_pMouseDevice);
}
	
CDasherInterfaceBridge::~CDasherInterfaceBridge() {
  if (m_pMouseDevice)
	delete m_pMouseDevice;
  if (m_pTiltDevice)
	delete m_pTiltDevice;
  if (m_pMixDevice)
	delete m_pMixDevice;
  //(ACL)registered input filters should be automatically free'd by the module mgr?
}

void CDasherInterfaceBridge::NotifyTouch(screenint x, screenint y)
{
	m_pMouseDevice->NotifyTouch(x, y);
}

void CDasherInterfaceBridge::SetTiltAxes(Vec3 main, float off, Vec3 slow, float off2)
{
	m_pTiltDevice->SetAxes(main, off, slow, off2);
}

void CDasherInterfaceBridge::SetupUI() {
  NSLog(@"CDasherInterfaceBridge::SetupUI");
}

void CDasherInterfaceBridge::OnUIRealised() {CDasherInterfaceBase::OnUIRealised();}

void CDasherInterfaceBridge::ChangeScreen(CDasherScreen *pScreen) {
  CDasherInterfaceBase::ChangeScreen(pScreen);
  m_pTiltDevice->SetScreenBounds(pScreen->GetWidth(), pScreen->GetHeight());
  m_pMouseDevice->SetScreenBounds(pScreen->GetWidth(), pScreen->GetHeight());
}

string CDasherInterfaceBridge::GetParamName(int iParameter) {
  return static_cast<COSXSettingsStore *>(m_pSettingsStore)->GetParamName(iParameter);
}

void CDasherInterfaceBridge::SetupPaths() {
  NSString *systemDir = [NSString stringWithFormat:@"%@/", [[NSBundle mainBundle] bundlePath]];
  NSString *userDir = [NSString stringWithFormat:@"%@/", [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]];

  m_pSettingsStore->SetStringParameter(SP_SYSTEM_LOC, StdStringFromNSString(systemDir));
  m_pSettingsStore->SetStringParameter(SP_USER_LOC, StdStringFromNSString(userDir));
}

void CDasherInterfaceBridge::CreateSettingsStore() {
  m_pSettingsStore = new COSXSettingsStore(m_pEventHandler);
}

void CDasherInterfaceBridge::ScanAlphabetFiles(std::vector<std::string> &vFileList) {
  
  NSDirectoryEnumerator *dirEnum;
  NSString *file;

  dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:NSStringFromStdString(GetStringParameter(SP_SYSTEM_LOC))];
  while (file = [dirEnum nextObject]) {
    if ([file hasSuffix:@".xml"] && [file hasPrefix:@"alphabet"]) {
      vFileList.push_back(StdStringFromNSString(file));
    }
  }  
  
  dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:NSStringFromStdString(GetStringParameter(SP_USER_LOC))];
  while (file = [dirEnum nextObject]) {
    if ([file hasSuffix:@".xml"] && [file hasPrefix:@"alphabet"]) {
      vFileList.push_back(StdStringFromNSString(file));
    }
  }  
}

void CDasherInterfaceBridge::ScanColourFiles(std::vector<std::string> &vFileList) {
  NSDirectoryEnumerator *dirEnum;
  NSString *file;
  
  dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:NSStringFromStdString(GetStringParameter(SP_SYSTEM_LOC))];
  while (file = [dirEnum nextObject]) {
    if ([file hasSuffix:@".xml"] && [file hasPrefix:@"colour"]) {
      vFileList.push_back(StdStringFromNSString(file));
    }
  }  
  
  dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:NSStringFromStdString(GetStringParameter(SP_USER_LOC))];
  while (file = [dirEnum nextObject]) {
    if ([file hasSuffix:@".xml"] && [file hasPrefix:@"colour"]) {
      vFileList.push_back(StdStringFromNSString(file));
    }
  }  
}


void CDasherInterfaceBridge::NewFrame(unsigned long iTime, bool bForceRedraw) {
  CDasherInterfaceBase::NewFrame(iTime, bForceRedraw);
}

void CDasherInterfaceBridge::StartTimer() {
  [dasherApp startTimer];
}

void CDasherInterfaceBridge::ShutdownTimer() {
  [dasherApp shutdownTimer];
}
 
void CDasherInterfaceBridge::GameMessageOut(int message, const void* messagedata) {
  NSLog(@"GameMessageOut");
}

void CDasherInterfaceBridge::ExternalEventHandler(Dasher::CEvent *pEvent) {
  
  switch (pEvent->m_iEventType) {
    case EV_PARAM_NOTIFY:
      // don't need to do anything because the PreferencesController is observing changes to the 
      // user defaults controller which is observing the user defaults and will be notified when
      // the parameter is actually written by COSXSettingsStore.
//      CParameterNotificationEvent *parameterEvent(static_cast < CParameterNotificationEvent * >(pEvent));
//      NSLog(@"CParameterNotificationEvent, m_iParameter: %d", parameterEvent->m_iParameter);
	  {
		Dasher::CParameterNotificationEvent *pEvt(static_cast<Dasher::CParameterNotificationEvent *>(pEvent));
		if (pEvt->m_iParameter == LP_MAX_BITRATE || pEvt->m_iParameter == LP_BOOSTFACTOR)
			[dasherApp notifySpeedChange];
	  }
      break;
    case EV_EDIT:
	  {
//      NSLog(@"ExternalEventHandler, m_iEventType = EV_EDIT");
		  CEditEvent *editEvent((CEditEvent *)pEvent);
      switch (editEvent->m_iEditType) {
        case 1:
          //NSLog(@"ExternalEventHandler edit insert");
          [dasherApp outputCallback:NSStringFromStdString(editEvent->m_sText)];
          break;
        case 2:
          //NSLog(@"ExternalEventHandler edit delete");
          [dasherApp deleteCallback:NSStringFromStdString(editEvent->m_sText)];
          break;
        case 10:
          NSLog(@"ExternalEventHandler edit convert");
          break;
        case 11:
          NSLog(@"ExternalEventHandler edit protect");
          break;
        default:
          break;
      }
	  }
        break;
    case EV_EDIT_CONTEXT:
	{
      CEditContextEvent *ecvt((CEditContextEvent *)pEvent);
      SetContext(StdStringFromNSString([dasherApp textAtOffset:ecvt->m_iOffset Length:ecvt->m_iLength]));
      break;
	}
    case EV_START:
      NSLog(@"ExternalEventHandler, m_iEventType = EV_START");
      break;
    case EV_STOP:
      NSLog(@"ExternalEventHandler, m_iEventType = EV_STOP");
      break;
    case EV_CONTROL:
      NSLog(@"ExternalEventHandler, m_iEventType = EV_CONTROL");
      break;
    case EV_COMMAND:
      NSLog(@"ExternalEventHandler, m_iEventType = EV_COMMAND");
      break;
    case EV_LOCK:
    {
      CLockEvent *evt(static_cast<CLockEvent *>(pEvent));
      NSString *dispMsg = nil;
      if (evt->m_bLock) {
        dispMsg = NSStringFromStdString(evt->m_strMessage);
        if (evt->m_iPercent) dispMsg = [NSString stringWithFormat:@"%@ (%i%%)",
                                                                  dispMsg,evt->m_iPercent];
      }
      [dasherApp setLockText:dispMsg];
      break;
    }
    case EV_MESSAGE:
	  {
      CMessageEvent *messageEvent(static_cast < CMessageEvent * >(pEvent));
      [dasherApp displayMessage:NSStringFromStdString(messageEvent->m_strMessage) ID:messageEvent->m_iID Type:messageEvent->m_iType];
      break;
	}
    default:
      NSLog(@"ExternalEventHandler, UNKNOWN m_iEventType = %d", pEvent->m_iEventType);
      break;
  }
  
}

int CDasherInterfaceBridge::GetFileSize(const std::string &strFileName) {
  struct stat sStatInfo;
  
  if(!stat(strFileName.c_str(), &sStatInfo))
    return sStatInfo.st_size;
  else
    return 0;
}

/*void CDasherInterfaceBridge::Train(NSString *fileName) {
  std::string f = StdStringFromNSString(fileName);
  NSLog(@"Read train file: %s", f.c_str());
  NSLog(@"method disappeared!! doing nuffink");
//  CDasherInterfaceBase::TrainFile(f, GetFileSize(f), 0);
}*/

void CDasherInterfaceBridge::WriteTrainFile(const std::string &strNewText) {
  if(strNewText.length() == 0)
    return;
  
  std::string strFilename(GetStringParameter(SP_USER_LOC) + GetStringParameter(SP_TRAIN_FILE));
  
  NSLog(@"Write train file: %s", strFilename.c_str());
  
  int fd=open(strFilename.c_str(),O_CREAT|O_WRONLY|O_APPEND,S_IRUSR|S_IWUSR);
  write(fd,strNewText.c_str(),strNewText.length());
  close(fd);
}

/*NSDictionary *CDasherInterfaceBridge::ParameterDictionary() {
  COSXSettingsStore *ss(static_cast < COSXSettingsStore * >(m_pSettingsStore));
  return ss->ParameterDictionary();
}

id CDasherInterfaceBridge::GetParameter(NSString *aKey) {
  
  COSXSettingsStore *ss(static_cast < COSXSettingsStore * >(m_pSettingsStore));
  int pIndex = ss->GetParameterIndex(StdStringFromNSString(aKey));

  switch (ss->GetParameterType(pIndex)) {
    case ParamBool:
      return [NSNumber numberWithBool:GetBoolParameter(pIndex)];
      break;
    case ParamLong:
      return [NSNumber numberWithLong:GetLongParameter(pIndex)];
      break;
    case ParamString:
      return NSStringFromStdString(GetStringParameter(pIndex));
      break;
    default:
      break;
  }
  
  return nil;
}

void CDasherInterfaceBridge::SetParameter(NSString *aKey, id aValue) {
  
  COSXSettingsStore *ss(static_cast < COSXSettingsStore * >(m_pSettingsStore));
  int pIndex = ss->GetParameterIndex(StdStringFromNSString(aKey));
  
  switch (ss->GetParameterType(pIndex)) {
    case ParamBool:
      SetBoolParameter(pIndex, [aValue boolValue]);
      break;
    case ParamLong:
      SetLongParameter(pIndex, [aValue longValue]);
      break;
    case ParamString:
      SetStringParameter(pIndex, StdStringFromNSString(aValue));
      break;
    default:
      break;
  }
}*/
