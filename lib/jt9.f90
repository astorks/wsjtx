program jt9

! Decoder for JT9.  Can run stand-alone, reading data from *.wav files;
! or as the back end of wsjt-x, with data placed in a shared memory region.

! NB: For unknown reason, ***MUST*** be compiled by g95 with -O0 !!!

  character*80 arg,infile
  parameter (NMAX=1800*12000)        !Total sample intervals per 30 minutes
  parameter (NDMAX=1800*1500)        !Sample intervals at 1500 Hz rate
  parameter (NSMAX=22000)            !Max length of saved spectra
  integer*4 ihdr(11)
  real*4 s(NSMAX)
  real*4 red(NSMAX)
  logical*1 lstrong(0:1023)
  integer*1 i1SoftSymbols(207)
  character*22 msg
  integer*2 id2
  complex c0
  common/jt9com/id2(NMAX),ss(184,NSMAX),savg(NSMAX),c0(NDMAX),    &
       nutc,npts8,junk(20)

  nargs=iargc()
  if(nargs.lt.1) then
     print*,'Usage: jt9 TRperiod file1 [file2 ...]'
     print*,'       Reads data from *.wav files.'
     print*,''
     print*,'       jt9 -s'
     print*,'       Gets data from shared memory region.'
     go to 999
  endif
  call getarg(1,arg)
  if(arg(1:2).eq.'-s') then
     go to 999
  endif
  read(arg,*) ntrperiod

  ifile1=2

  nfa=1000
  nfb=2000
  ntol=500
  nfqso=1500
  newdat=1
  nb=0
  nbslider=100

  do ifile=ifile1,nargs
     call getarg(ifile,infile)
     open(10,file=infile,access='stream',status='old',err=998)
     read(10) ihdr
     i1=index(infile,'.wav')
     read(infile(i1-4:i1-1),*,err=1) nutc0
     go to 2
1    nutc0=0
2    nsps=0
     if(ntrperiod.eq.1)  nsps=6912
     if(ntrperiod.eq.2)  nsps=15360
     if(ntrperiod.eq.5)  nsps=40960
     if(ntrperiod.eq.10) nsps=82944
     if(ntrperiod.eq.30) nsps=252000
     if(nsps.eq.0) stop 'Error: bad TRprtiod'

     kstep=nsps/2
     tstep=kstep/12000.0
     k=0
     nhsym0=-999
     npts=(60*ntrperiod-6)*12000
     read(10) id2(1:npts)

!     do i=1,npts
!        id2(i)=100.0*sin(6.283185307*1046.875*i/12000.0)
!     enddo

     do iblk=1,npts/kstep
        k=iblk*kstep
        nhsym=(k-2048)/kstep
        if(nhsym.ge.1 .and. nhsym.ne.nhsym0) then
! Emit signal readyForFFT
           call symspec(k,ntrperiod,nsps,ndiskdat,nb,nbslider,pxdb,   &
                s,red,df3,ihsym,nzap,slimit,lstrong)
           nhsym0=nhsym
           if(ihsym.ge.184) go to 10
        endif
     enddo

10   continue

! Now do the decoding
     nutc=nutc0

! Get sync, approx freq
     call sync9(ss,tstep,f0a,df3,ntol,nfqso,sync,fpk,red)    
     fpk0=fpk
!     iz=1000.0/df3
!     do i=1,iz
!        freq=1000.0 + (i-1)*df3
!        write(72,3001) freq,red(i),db(red(i))
!3001    format(3f10.3)
!     enddo
!     flush(72)

     call spec9(c0,npts8,nsps,f0a,fpk,xdt,i1SoftSymbols)
     call decode9(i1SoftSymbols,msg)
     write(*,1010) nutc,xdt,1000.0+fpk,msg,sync,fpk0
1010 format(i4.4,f6.1,f7.1,2x,a22,2f9.1)
  enddo

  go to 999

998 print*,'Cannot open file:'
  print*,infile

999 end program jt9
