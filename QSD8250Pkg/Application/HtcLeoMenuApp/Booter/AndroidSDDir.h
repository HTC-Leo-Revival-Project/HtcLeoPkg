#ifndef _ANDROID_SDDIR_H_
#define _ANDROID_SDDIR_H_

#define CONFIG_FILE_PATH L"\\.SelectedDir"

//GLOBAL VAR FOR ALL OF MENU APP
extern CHAR16 *SelectedDir;
extern BOOLEAN FallBack;

//function definitions
EFI_STATUS DisplayAndSelectDirectory(CHAR16 **DirList, UINTN DirCount, CHAR16 **SelectedDir);
void SetAndroidSdDir(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS ListDirectories(EFI_HANDLE *HandleBuffer, UINTN HandleCount, CHAR16 ***DirList, UINTN *DirCount);
EFI_STATUS SaveSelectedDirToFile(CHAR16 *DirPath);
EFI_STATUS LoadSelectedDirFromFile(CHAR16 **DirPath);
#endif