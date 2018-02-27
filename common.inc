!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=..\bin\$(ARCH)
OBJDIR=obj\$(ARCH)
SRCDIR=.

CC=cl
LINKER=link
SIGNTOOL=signtool
ASM=D:\nasm\latest\nasm.exe
RD=rd /s /q
RM=del /q

CFLAGS=\
	/nologo\
	/c\
	/Ox\
	/W4\
	/Zi\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/INCREMENTAL:NO\

AFLAGS=\
	-g\
	-O0\
!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
	-fwin64\
!ELSE
	-fwin32\
!ENDIF
