!include ..\common.inc

GADGET=gadget.dll
ATTACKER=spectre.exe
VICTIM=victim.exe

OBJS_GADGET=\
	gadget\$(OBJDIR)\dllmain.obj\
	gadget\$(OBJDIR)\data.res\
	gadget\$(OBJDIR)\01.obj\

OBJS_ATTACKER=\
	attacker\$(OBJDIR)\main.obj\
	attacker\$(OBJDIR)\01.obj\

OBJS_VICTIM=\
	victim\$(OBJDIR)\main.obj\

# warning C4702: unreachable code
CFLAGS=\
	$(CFLAGS)\
	/wd4702\
	/EHsc\
	/DUNICODE\

LFLAGS=\
	$(LFLAGS)\
	/SUBSYSTEM:CONSOLE\

LFLAGS_WIN=\
	$(LFLAGS)\
	/SUBSYSTEM:WINDOWS\

all: $(OUTDIR)\$(GADGET) $(OUTDIR)\$(ATTACKER) $(OUTDIR)\$(VICTIM)

$(OUTDIR)\$(GADGET): $(OBJS_GADGET)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_WIN) /DLL /DEF:gadget\export.def /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(ATTACKER): $(OBJS_ATTACKER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(OUTDIR)\gadget.lib /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(VICTIM): $(OBJS_VICTIM)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_WIN) user32.lib $(OUTDIR)\gadget.lib /PDB:"$(@R).pdb" /OUT:$@ $**

{gadget}.cpp{gadget\$(OBJDIR)}.obj:
	@if not exist gadget\$(OBJDIR) mkdir gadget\$(OBJDIR)
	$(CC) $(CFLAGS) /Fo"gadget\$(OBJDIR)\\" /Fd"gadget\$(OBJDIR)\\" $<

{gadget}.asm{gadget\$(OBJDIR)}.obj:
	@if not exist gadget\$(OBJDIR) mkdir gadget\$(OBJDIR)
	$(ASM) $(AFLAGS) -o $@ $<

{gadget}.rc{gadget\$(OBJDIR)}.res:
	@if not exist gadget\$(OBJDIR) mkdir gadget\$(OBJDIR)
	rc /nologo /fo "$@" $<

{attacker}.cpp{attacker\$(OBJDIR)}.obj:
	@if not exist attacker\$(OBJDIR) mkdir attacker\$(OBJDIR)
	$(CC) $(CFLAGS) /Fo"attacker\$(OBJDIR)\\" /Fd"attacker\$(OBJDIR)\\" $<

{attacker}.asm{attacker\$(OBJDIR)}.obj:
	@if not exist attacker\$(OBJDIR) mkdir attacker\$(OBJDIR)
	$(ASM) $(AFLAGS) -o $@ $<

{victim}.cpp{victim\$(OBJDIR)}.obj:
	@if not exist victim\$(OBJDIR) mkdir victim\$(OBJDIR)
	$(CC) $(CFLAGS) /Fo"victim\$(OBJDIR)\\" /Fd"victim\$(OBJDIR)\\" $<

{victim}.asm{victim\$(OBJDIR)}.obj:
	@if not exist victim\$(OBJDIR) mkdir victim\$(OBJDIR)
	$(ASM) $(AFLAGS) -o $@ $<

clean:
	@if exist gadget\$(OBJDIR) $(RD) gadget\$(OBJDIR)
	@if exist attacker\$(OBJDIR) $(RD) attacker\$(OBJDIR)
	@if exist victim\$(OBJDIR) $(RD) victim\$(OBJDIR)

	@if exist $(OUTDIR)\$(GADGET) $(RM) $(OUTDIR)\$(GADGET)
	@if exist $(OUTDIR)\$(ATTACKER) $(RM) $(OUTDIR)\$(ATTACKER)
	@if exist $(OUTDIR)\$(VICTIM) $(RM) $(OUTDIR)\$(VICTIM)

	@if exist $(OUTDIR)\$(GADGET:dll=pdb) $(RM) $(OUTDIR)\$(GADGET:dll=pdb)
	@if exist $(OUTDIR)\$(GADGET:dll=exp) $(RM) $(OUTDIR)\$(GADGET:dll=exp)
	@if exist $(OUTDIR)\$(GADGET:dll=lib) $(RM) $(OUTDIR)\$(GADGET:dll=lib)
	@if exist $(OUTDIR)\$(ATTACKER:exe=pdb) $(RM) $(OUTDIR)\$(ATTACKER:exe=pdb)
	@if exist $(OUTDIR)\$(VICTIM:exe=pdb) $(RM) $(OUTDIR)\$(VICTIM:exe=pdb)
