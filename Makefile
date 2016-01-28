
#
#   ISPY / MTEST / FTEST / LOAD / CKMON / BOOTPATH Makefile
#

E               =
O               = .o
MODEL           =
COPTS           = $(MODEL) -Wall
OPTIM           = -O
CC              = gcc -c $(OPTIM) $(COPTS)
CCLM            = $(CC)
COPTSLM         = $(COPTS)
LINKMODULE      = b004link
DEFAULTLINK	= \"/dev/link0\"
LINKOBJS        = $(LINKMODULE)$(O)
LINK            = gcc $(OPTIM)
LIBRARIES       =
RM              = rm
OCCAM		= oc -y -a -n -k -v -e -w -h -T
FIND            = grep

all:     ispy$(E) mtest$(E)

TESTSRCS=test2.occ test414.occ test425.occ test80x.occ test800.occ test801.occ
TESTHSRCS=test2.h test414.h test425.h test32.h test80x.h test800.h test801.h
CSRCS= *.c checklib.h cklib.h
OCCSRCS=check*.occ mtest*.occ

archive:
	zip check.zip $(OCCSRCS) $(CSRCS)

# this makefile in 'included' by another makefile

clean:
		rm -f *.o ispy mtest

#
#  ispy
#

ispy$(E):      check$(O) cklib$(O) $(LINKOBJS)
		$(LINK) -o ispy$(E) check$(O) cklib$(O) $(LINKOBJS) $(LIBRARIES)

check$(O):      check.c checklib.h inmos.h cklib.h \
		type32.h type16.h check32.h check16.h iserver.h
		$(CC) -DDEFAULTLINK=$(DEFAULTLINK) -o check$(O) check.c


#
#  mtest
#

mtest$(E):      mtest$(O) cklib$(O) $(LINKOBJS)
		$(LINK) -o mtest$(E) mtest$(O) cklib$(O) $(LINKOBJS)

mtest$(O):      mtest.c cklib.h inmos.h  \
		mtest32.h mtest16.h checklib.h
		$(CC) -o mtest$(O) mtest.c


#
#  ckmon
#

ckmon$(E):      ckmon$(O) screen$(O) cklib$(O) $(LINKOBJS)
		$(LINK) -o ckmon$(E) ckmon$(O) screen$(O) cklib$(O) $(LINKOBJS)

ckmon$(O):      ckmon.c link.h screen.h inmos.h opcodes.h \
		cklib.h boot.h peek.h pp32.h pp16.h
		$(CC) -o ckmon$(O) ckmon.c

screen$(O):     screen.c screen.h
		$(CC) screen$(O) screen.c

#
#  support routines
#

cklib$(O):      cklib.c cklib.h checklib.h
		$(CC) -o cklib$(O) cklib.c

hostend$(O):    hostend.c inmos.h \
		iserver.h pack.h
		$(CC) -o hostend$(O) hostend.c
		
#
#  link code
#

$(LINKMODULE)$(O):      $(LINKMODULE).c inmos.h
		$(CCLM) $(COPTSLM) $(LINKMODULE).c -o $(LINKMODULE)$(O)

			

#
#  end of C makefile
#
#
#RM              = rm
#OCCAM		= oc -y -a -n -k -v -e -w -h -T
#FIND            = grep
#
#
#type32.h:       type32.occ checklib.occ 
#		$(OCCAM)A type32
#		ilist -c type32.tco | perl tco2h.pl > type32.h
#		$(FIND) "total code requirement"  type32.h
#		$(RM) type32.tco
#
#type16.h:       type16.occ checklib.occ 
#		$(OCCAM)2 type16
#		ilist -c type16.tco | perl tco2h.pl > type16.h
#		$(FIND) "total code requirement" < type16.h
#		$(RM) type16.tco
#
#check32.h:      check32.occ checklib.occ 
#		$(OCCAM)A check32
#		ilist -c check32.tco | perl tco2h.pl > check32.h
#		$(FIND) "total code requirement" < check32.h
#		$(RM) check32.tco
#
#check16.h:      check16.occ checklib.occ 
#		$(OCCAM)2 check16
#		ilist -c check16.tco | perl tco2h.pl > check16.h
#		$(FIND) "total code requirement" < check16.h
#		$(RM) check16.tco
#
# c00432.h:       c00432.occ checklib.occ 
# 		$(OCCAM)A c00432
# 		ilist -c c00432.tco | perl tco2h.pl > c00432.h
# 		$(FIND) "total code requirement" < c00432.h
# 		$(RM) c00432.tco
# 
# c00416.h:       c00416.occ checklib.occ 
# 		$(OCCAM)2 c00416
# 		ilist -c c00416.tco | perl tco2h.pl > c00416.h
# 		$(FIND) "total code requirement" < c00416.h
# 		$(RM) c00416.tco
# 
#
#  mtest
#
#
#mtest32.h:      mtest32.occ checklib.occ 
#		$(OCCAM)A mtest32
#		ilist -c mtest32.tco | perl tco2h.pl > mtest32.h
#		$(FIND) "total code requirement" < mtest32.h
#		$(RM) mtest32.tco
#
#mtest16.h:      mtest16.occ checklib.occ 
#		$(OCCAM)2 mtest16
#		ilist -c mtest16.tco | perl tco2h.pl > mtest16.h
#		$(FIND) "total code requirement" < mtest16.h
#		$(RM) mtest16.tco
#
#
#  eof
#
