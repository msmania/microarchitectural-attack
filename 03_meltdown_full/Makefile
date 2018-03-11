!include ..\common.inc

WDKINCPATH=C:\Program Files (x86)\Windows Kits\10\Include\10.0.16299.0\km
WDKLIBPATH=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.16299.0\km\x64
CODESIGN_SHA1=fd3df8f2694b47e0c38b629448d754d8c0e3b779

ATTACKER=meltdown.exe
CONTROLLER=mdc.exe
DRIVER=meltdown.sys

OBJS_ATTACKER=\
	attacker\$(OBJDIR)\main.obj\
	attacker\$(OBJDIR)\01.obj\

OBJS_CONTROLLER=\
	controller\$(OBJDIR)\main.obj\

OBJS_DRIVER=\
	driver\$(OBJDIR)\main.obj\

LIBS_DRIVER=\
	bufferoverflowfastfailk.lib\
	ntoskrnl.lib\

LFLAGS_CONSOLE=\
	$(LFLAGS)\
	/SUBSYSTEM:CONSOLE\

CFLAGS_DRIVER=\
	$(CFLAGS)\
	/Os\
	/GF\
	/Gm-\
	/GS\
	/Gy\
	/Gz\
	/GR-\
	/kernel -cbstring -d2epilogunwind\
	/fp:precise\
	/Zp8\
	/Zc:wchar_t-\
	/Zc:forScope\
	/Zc:inline\
	/D_AMD64_\
	/I"$(WDKINCPATH)"\
	/I"$(WDKINCPATH)\crt"\

LFLAGS_DRIVER=\
	$(LFLAGS)\
	/SUBSYSTEM:NATIVE\
	/DRIVER\
	/KERNEL\
	/ENTRY:"DriverEntry"\
	/RELEASE\
	/SECTION:"INIT,d"\
	/NODEFAULTLIB\
	/MANIFEST:NO\
	/OPT:REF\
	/OPT:ICF\
	/MERGE:"_TEXT=.text;_PAGE=PAGE"\
	/LIBPATH:"$(WDKLIBPATH)"\

all: $(OUTDIR)\$(ATTACKER) $(OUTDIR)\$(CONTROLLER) $(OUTDIR)\$(DRIVER)

$(OUTDIR)\$(ATTACKER): $(OBJS_ATTACKER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_CONSOLE) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(CONTROLLER): $(OBJS_CONTROLLER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_CONSOLE) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(DRIVER): $(OBJS_DRIVER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_DRIVER) $(LIBS_DRIVER) /PDB:"$(@R)_driver.pdb" /IMPLIB:"$(@R).lib" /OUT:$@ $**
	$(SIGNTOOL) sign /ph /sha1 $(CODESIGN_SHA1) $@

{attacker}.cpp{attacker\$(OBJDIR)}.obj:
	@if not exist attacker\$(OBJDIR) mkdir attacker\$(OBJDIR)
	$(CC) $(CFLAGS) /Fo"attacker\$(OBJDIR)\\" /Fd"attacker\$(OBJDIR)\\" $<

{attacker}.asm{attacker\$(OBJDIR)}.obj:
	@if not exist attacker\$(OBJDIR) mkdir attacker\$(OBJDIR)
	$(ASM) $(AFLAGS) -o $@ $<

{controller}.cpp{controller\$(OBJDIR)}.obj:
	@if not exist controller\$(OBJDIR) mkdir controller\$(OBJDIR)
	$(CC) $(CFLAGS) /DUNICODE /Fo"controller\$(OBJDIR)\\" /Fd"controller\$(OBJDIR)\\" $<

{driver}.cpp{driver\$(OBJDIR)}.obj:
	@if not exist driver\$(OBJDIR) mkdir driver\$(OBJDIR)
	$(CC) $(CFLAGS_DRIVER) /Fo"driver\$(OBJDIR)\\" /Fd"driver\$(OBJDIR)\\" $<

clean:
	@if exist attacker\$(OBJDIR) $(RD) attacker\$(OBJDIR)
	@if exist controller\$(OBJDIR) $(RD) controller\$(OBJDIR)
	@if exist driver\$(OBJDIR) $(RD) driver\$(OBJDIR)
	@if exist $(OUTDIR)\$(ATTACKER) $(RM) $(OUTDIR)\$(ATTACKER)
	@if exist $(OUTDIR)\$(ATTACKER:exe=pdb) $(RM) $(OUTDIR)\$(ATTACKER:exe=pdb)
	@if exist $(OUTDIR)\$(CONTROLLER) $(RM) $(OUTDIR)\$(CONTROLLER)
	@if exist $(OUTDIR)\$(CONTROLLER:exe=pdb) $(RM) $(OUTDIR)\$(CONTROLLER:exe=pdb)
	@if exist $(OUTDIR)\$(DRIVER) $(RM) $(OUTDIR)\$(DRIVER)
	@if exist $(OUTDIR)\$(DRIVER:sys=pdb) $(RM) $(OUTDIR)\$(DRIVER:sys=pdb)
