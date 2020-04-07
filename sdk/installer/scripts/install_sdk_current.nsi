#####################################################################
# iobjects_sdk.nsi
#
# Script for creating the SDK installer using NSIS PiMP.
# Written by Eric R. Turner <erict@iobjects.com>
# Last updated August 9, 2001
#
# Note: When updating this script for a newer version of the SDK,
# searching for "TO UPDATE:" will indicate the portions that you
# should modify.
#####################################################################

#####################################################################
# Set up the properties of the installer
#####################################################################

# Installer name
Name "iObjects SDK"

# Name of the SDK installer file
OutFile "..\images\installer.exe"

# User should see the installation
# Other options are 'silent' and 'silentlog'
SilentInstall normal

# Make sure the installer is not corrupt
# User can override with /NCRC flag
CRCCheck off

# set install progress
InstProgressFlags smooth

# The user should see the license.
# LicenseData could be located on a share that Legal updates,
# so we always build with the latest user agreement.
#
# TO UPDATE: Make sure you have the correct license file!
LicenseText "User Licensing Agreement"
LicenseData "..\sdk_eula.txt"

# most of our stuff is already compressed
SetCompress off

# set the background color
BGGradient 0000ff 000040

# Show details about what is being done
ShowInstDetails show

ComponentText "Select components to install" \
"Please select the components to install from the list below. If you \
do not have Cygwin 1.3.2 currently installed on your computer, you will \
need to install it; otherwise it is safe to install the SDK without it."

EnabledBitmap green_icon.bmp
DisabledBitmap red_icon.bmp

#####################################################################
# Warn the user if they attempt to install over a more recent
# version of the SDK.
#
# In this example the version trying to be installed is 2, so
# versions "", 0, and 1 are valid CurrentVersion registry values.
# If the installer doesn't find one of those, then alert the user
# and give them the choice of overwriting the newer version.
#
# The logic is a bit of a kludge imposed by limitations of the
# installer.
#####################################################################
Section -QuitIfOlderVersion
   ReadRegStr $1 HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK\" CurrentVersion
   # Allow brand new installation
   BrandNew:  StrCmp $1 "" ContinueInstall ReInstall
   # Allow reinstallation of the current version, or
   # installation over older versions.
   # TO UPDATE: Modify this to detect versions properly
   #            For example, to update from ver 2 to 3 replace:
   #               Reinstall: StrCmp $1 "2" ContinueInstall OldVer1
   #            with:
   #               Reinstall: StrCmp $1 "3" ContinueInstall OldVer2
   #               OldVer2:   StrCmp $1 "2" ContinueInstall OldVer1
   ReInstall: StrCmp $1 "2" ContinueInstall OldVer1
   OldVer1:   StrCmp $1 "1" ContinueInstall OldVer0
   OldVer0:   StrCmp $1 "0" ContinueInstall AbortInstall
   AbortInstall:
      # CurrentVersion is not one that this installer knows about,
      # so make the user decide.
      MessageBox MB_YESNO|MB_ICONSTOP \
                "A newer version of the SDK has been detected! \
Do you really want to overwrite that version?" \
                IDYES ContinueInstall                 
      Abort "Installation Cancelled."
   ContinueInstall:
SectionEnd

#####################################################################
# Prompt the user to choose an installation directory,
# using the default installation directory or the
# directory where the user's last version of the SDK
# was installed.
#####################################################################
InstallDir c:\iobjects\dadio
InstallDirRegKey HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK" InstallDir
DirText "Where would you like to install the SDK?" \
"Please specify a location to install the SDK, or use the default"

#####################################################################
# Allow the user to decide whether or not to install cygwin.
#####################################################################
Section "Cygwin 1.3.2"
   AddSize 200000

   CreateDirectory $INSTDIR\cygwin_files
   SetOutPath $INSTDIR\cygwin_files
   # TO UPDATE: Make sure you indicate the proper location of the cygwin setup files
   File /r "..\cygwin_files\*"
   ExecWait $INSTDIR\cygwin_files\setup.exe
   RMDir /r $INSTDIR\cygwin_files
SectionEnd

#####################################################################
# Here's where we install Interactive Objects's source and object
# files. How are we going to determine if the user's current files
# have been modified, and thus need to be backed up?
#
# The installer is too simple for such a task! We could save a tar.gz
# archive of the new files to disk, and save another program to disk
# that does the file comparisons, takes care of backing up the user's
# old files if they were modified, and installs the new files.
# The installer could execute the program.
#
# Another option is to simply ask the user if they want to save a
# backup of existing SDK files. This is what we'll do for now
# (it's simple and it works fairly well).
#####################################################################
Section "IObjects Dharma(tm) SDK 1.0"
   AddSize 300000
   ReadRegStr $1 HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK\" CurrentVersion
   # If the user is installing the SDK for the first time,
   # don't worry about making a backup.
   #
   CheckBrandNew: StrCmp $1 "" SkipBackupFiles BackupFiles
   BackupFiles:
      MessageBox MB_YESNO|MB_ICONINFORMATION \
                 "Another version of the SDK had been detected. \
Would you like save those SDK files \
(you probably want to do this if you have \
customized the SDK)?" \
                 IDNO SkipBackupFiles
      # Delete the uninstaller, since the registry keys will be
      # overwritten and the old SDK files will be moved to
      # a new location (i.e. the uninstaller will no longer work properly).
      Delete $INSTDIR\uninstall.exe
      # TO UPDATE: update the name of the backup directory to reflect
      #            the current version.
      Rename $INSTDIR $INSTDIR_backup_from_ver_1_install
   SkipBackupFiles:
   SetOutPath $INSTDIR
   # TO UPDATE: make sure you indicate the proper location of the source/object files
   File /r "..\iobjects_files\arm-elf-2.9-win32.zip"
   File /r "..\iobjects_files\build*.zip"
   
   # we need to decompress our stuff now, so figure out where cygwin is located
   ReadRegStr $7 HKEY_CURRENT_USER "Software\Cygnus Solutions\Cygwin\mounts v2\/" "native"
   StrCmp $7 "" TryLocalMachine UnzipFiles

   TryLocalMachine:
   ReadRegStr $7 HKEY_LOCAL_MACHINE "Software\Cygnus Solutions\Cygwin\mounts v2\/" "native"
   StrCmp $7 "" ErrorOut UnzipFiles

   ErrorOut: 
      MessageBox MB_OK|MB_ICONINFORMATION \
                 "Couldn't find Cygwin installed on your machine.$\n\
Installation could not be completed. You will need to \
reinstall the SDK, being careful to install Cygwin support."
      Abort "Installation failed."

   UnzipFiles:
      ExecWait '$7\bin\bash -c "cd $(/bin/cygpath -u "$INSTDIR") ; for i in `/bin/ls *.zip`; do /bin/unzip $i; /bin/rm $i; done"'

   # fix up the cygwin installation. they use a passwd file, which really makes life harder for us, since the UID
   # for everybody gets mapped to Administrator
   Rename "$7\etc\passwd" "$7\etc\passwd.old"

SectionEnd

#####################################################################
# Set the registry keys to record the location and version 
#####################################################################
Section -SetRegistryKeys
   WriteRegStr HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK\" InstallDir "$INSTDIR"
   # TO UPDATE: make sure you indicate the correct current version
   WriteRegStr HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK\" CurrentVersion "1.0"

   # repair cygwin mountpoints so that c is mounted as /c/ instead of
   # /cygdrive/c/
   WriteRegStr HKEY_CURRENT_USER "Software\Cygnus Solutions\Cygwin\mounts v2" "cygdrive prefix" "/"

   # force cygwin to mount drives as textmode instead of binmode by setting 0x20 instead of 0x22
   # (0x20 is nounmount, 0x02 is binmode)
   WriteRegDWORD HKEY_CURRENT_USER "Software\Cygnus Solutions\Cygwin\mounts v2" "cygdrive flags" "32"

   # be nice and set HOME for the user
   WriteRegStr HKEY_CURRENT_USER Environment HOME "$INSTDIR"

SectionEnd # SetRegistryKeys

#####################################################################
# Write out a path to the default .bashrc file
#####################################################################
Section -WriteBashrc
   FileOpen $9 "$INSTDIR\.bash_extras" a
   FileWrite $9 "# this section is generated by the installer$\n"
   FileWrite $9 'INSTALL_BASE=`/bin/cygpath -u "$INSTDIR"`$\n'
   FileWrite $9 "PATH=/bin:/contrib/bin:$$INSTALL_BASE/tools/H-i686-cygwin32/bin:$$PATH$\n"
   FileWrite $9 "export PATH$\n"
   FileClose $9
SectionEnd # WriteBashrc

#####################################################################
# Create uninstall.exe
#####################################################################
UninstallText "This will uninstall the SDK. Hit next to continue."
UninstallExeName "uninstall.exe"

Section "Uninstall"
  # Remove registry keys
  DeleteRegKey HKEY_LOCAL_MACHINE "Software\IObjects Dharma SDK"
  DeleteRegValue HKEY_CURRENT_USER "Environment" "HOME"

  # Remove files
  RMDir /r "$SMPROGRAMS\IObjects Dharma(tm) SDK"
  RMDir /r $INSTDIR
SectionEnd

#####################################################################
# Add shortcuts to the start menu
#####################################################################
Section -InstallShortCuts
   CreateDirectory "$SMPROGRAMS\IObjects Dharma(tm) SDK"
   CreateShortCut "$SMPROGRAMS\IObjects Dharma(tm) SDK\Dharma SDK Documentation.lnk" \
                  "$INSTDIR\docs\sdk\index.htm" \
                  "" \
                  "$INSTDIR\docs\sdk\index.htm" \
                  0
   CreateShortCut "$SMPROGRAMS\IObjects Dharma(tm) SDK\Cygwin Bash Shell.lnk" \
                  "$7\bin\bash.exe" \
                  "" \
                  "$7\bin\bash.exe" \
                  0
   CreateShortCut "$SMPROGRAMS\IObjects Dharma(tm) SDK\uninstall.lnk" \
                  "$INSTDIR\uninstall.exe" \
                  "" \
                  "$INSTDIR\uninstall.exe" \
                  0
SectionEnd

Function .onInit
  ; This string exists on NT derivatives but not on 95/98 derivatives. Unfortunately on WinNT 4.0
  ; this is probably set, and i have no idea if we actually work properly on that
  ReadRegStr $3 HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $3 "" DoAbort TestNT4

  TestNT4:
  StrCmp $3 "4.0" DoAbort SkipAbort

  DoAbort:
    MessageBox MB_OK "This software requires Windows 2000 or Windows XP to install."
    Abort ; and we exit
  SkipAbort:
FunctionEnd

# EOF