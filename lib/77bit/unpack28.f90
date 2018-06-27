subroutine unpack28(n28_0,c13)

  parameter (NTOKENS=4874084,N24=16777216)
  integer nc(6)
  character*13 c13
  character*37 c1
  character*36 c2
  character*10 c3
  character*27 c4
  data c1/' 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'/
  data c2/'0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'/
  data c3/'0123456789'/
  data c4/' ABCDEFGHIJKLMNOPQRSTUVWXYZ'/
  data nc/37,36,19,27,27,27/

  n28=n28_0
  if(n28.lt.NTOKENS) then
! Special tokens DE, QRZ, CQ, CQ_nnn, CQ_aaaa
     if(n28.eq.0) c13='DE           '
     if(n28.eq.1) c13='QRZ          '
     if(n28.eq.2) c13='CQ           '
     if(n28.le.2) go to 900
     if(n28.le.1002) then
        write(c13,1002) n28-3
1002    format('CQ_',i3.3)
        go to 900
     endif
     if(n28.le.532443) then
        n=n28-1003
        i1=n/(27*27*27)
        n=n-27*27*27*i1
        i2=n/(27*27)
        n=n-27*27*i2
        i3=n/27
        i4=n-27*i3
        c13=c4(i1+1:i1+1)//c4(i2+1:i2+1)//c4(i3+1:i3+1)//c4(i4+1:i4+1)
        c13=adjustl(c13)
        c13='CQ_'//c13
        go to 900
     endif
  endif
  n28=n28-NTOKENS
  if(n28.lt.N24) then
     !code for 24-bit hash
  endif
  
! Standard callsign
  n=n28 - N24
  i1=n/(36*10*27*27*27)
  n=n-36*10*27*27*27*i1
  i2=n/(10*27*27*27)
  n=n-10*27*27*27*i2
  i3=n/(27*27*27)
  n=n-27*27*27*i3
  i4=n/(27*27)
  n=n-27*27*i4
  i5=n/27
  i6=n-27*i5
  c13=c1(i1+1:i1+1)//c2(i2+1:i2+1)//c3(i3+1:i3+1)//c4(i4+1:i4+1)//     &
       c4(i5+1:i5+1)//c4(i6+1:i6+1)//'       '
  c13=adjustl(c13)

900  return
end subroutine unpack28
