subroutine genft8(msg,msgsent,itone)

! Encode an FT8 message, producing array itone().
  
  use crc
  use packjt
  use hashing
  include 'ft8_params.f90'

  character*22 msg,msgsent
  character*84 cbits
  integer*4 i4Msg6BitWords(12)            !72-bit message as 6-bit words
  integer*1 msgbits(KK),codeword(3*ND)
  integer itone(NN)
  integer icos7(0:6)
  data icos7/2,5,6,0,4,1,3/                   !Costas 7x7 tone pattern

  call packmsg(msg,i4Msg6BitWords,itype)      !Pack into 12 6-bit bytes
  call unpackmsg(i4Msg6BitWords,msgsent)      !Unpack to get msgsent
  icrc=0        !### temporary ###

  write(cbits,1000) i4Msg6BitWords,icrc
1000 format(12b6.6,b12.12)
  read(cbits,1002) msgbits
1002 format(84i1)

  call encode168(msgbits,codeword)      !Encode the test message

! Message structure: S7 D28 S7 D28 S7
  itone(1:7)=icos7
  itone(35+1:35+7)=icos7
  itone(NN-6:NN)=icos7
  k=7
  do j=1,ND
     i=3*j -2
     k=k+1
     if(j.eq.29) k=k+7
     itone(k)=codeword(i)*4 + codeword(i+1)*2 + codeword(i+2)
  enddo

  return
end subroutine genft8
