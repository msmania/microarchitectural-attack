!include ..\common.inc

TARGET=branch.exe

OBJS=\
	$(OBJDIR)\main.obj\
	$(OBJDIR)\01.obj\

LIBS=\

LFLAGS=\
	$(LFLAGS)\
	/SUBSYSTEM:CONSOLE\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{$(SRCDIR)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(ASM) $(AFLAGS) -o $@ $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
