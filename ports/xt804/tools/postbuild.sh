#!/bin/bash

#############################
# colors
BLACK=$(tput setaf 0)
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
LIME_YELLOW=$(tput setaf 190)
POWDER_BLUE=$(tput setaf 153)
BLUE=$(tput setaf 4)
MAGENTA=$(tput setaf 5)
CYAN=$(tput setaf 6)
WHITE=$(tput setaf 7)
GREY=$(tput setaf 15)
BRIGHT=$(tput bold)
NORMAL=$(tput sgr0)
BLINK=$(tput blink)
REVERSE=$(tput smso)
UNDERLINE=$(tput smul)

#############################
FirmwareName=$1
WMTOOL=$2
SDKPath=$3
DistPath=$4

zip_type=$5
signature=$6
code_encrypt=$7

prikey_sel=0
sign_pubkey_src=0

printf "========================================\n"
printf "Target: %s\n" $DistPath/$FirmwareName
printf "SDK: %s\n" $SDKPath

sec_img_header=8002000
sec_img_pos=8002400
run_img_header=8010000
run_img_pos=8010400
upd_img_pos=8010000

img_type=1

printf "========================================\n"
printf "prikey_sel:%s" $prikey_sel
if [ $prikey_sel ]
then
  let img_type=$img_type+32*$prikey_sel
  printf " => Image Type: 0x%x\n" $img_type
else
  printf "\n"
fi

printf "code_encrypt:%s" $code_encrypt
if [ $code_encrypt == 1 ]
then
  let img_type=$img_type+16
  printf " => Image Type: 0x%x\n" $img_type
else
  printf "\n"
fi

printf "signature:%s" $signature
if [ $signature == 1 ]
then
  let img_type=$img_type+256
  printf " => Image Type: 0x%x\n" $img_type
else
  printf "\n"
fi

printf "sign_pubkey_src:%s" $sign_pubkey_src
if [ $sign_pubkey_src ]
then
  let img_type=$img_type+512
  printf " => Image Type: 0x%x\n" $img_type
else
  printf "\n"
fi
printf "========================================\n"
printf "Image Type: 0x%x\n" $img_type
printf "========================================\n"


#SUM=md5sum
SUM=crc32
OPENSSL=openssl

ImageName=$FirmwareName
if [ $code_encrypt == 1 ]
then
  ImageName=$FirmwareName"_encrypted"
fi

if [ $code_encrypt == 1 ]
then
    let prikey_sel=$prikey_sel+1
    $OPENSSL enc -aes-128-ecb -in $DistPath/"$FirmwareName".bin -out $DistPath/"$FirmwareName"_enc.bin -K 30313233343536373839616263646566 -iv 01010101010101010101010101010101
    $OPENSSL rsautl -encrypt -in "$SDKPath"/tools/W806/ca/key.txt -inkey "$SDKPath"/tools/W806/ca/capub_"$prikey_sel".pem -pubin -out key_en.dat
    cat $DistPath/"$FirmwareName"_enc.bin key_en.dat > $DistPath/"$FirmwareName"_enc_key.bin
    cat $DistPath/"$FirmwareName"_enc_key.bin "$SDKPath"/tools/W806/ca/capub_"$prikey_sel"_N.dat > $DistPath/"$FirmwareName"_enc_key_N.bin  
    $WMTOOL -b $DistPath/"$FirmwareName"_enc_key_N.bin -o $DistPath/"$ImageName" -it $img_type -fc 0 -ra $run_img_pos -ih $run_img_header -ua $upd_img_pos -nh 0 -un 0  > /dev/null
    rm -rf $DistPath/"$FirmwareName"_enc.bin $DistPath/"$FirmwareName"_enc_key.bin $DistPath/"$FirmwareName"_enc_key_N.bin
else
    $WMTOOL -b $DistPath/"$FirmwareName".bin -o $DistPath/"$ImageName" -it $img_type -fc 0 -ra $run_img_pos -ih $run_img_header -ua $upd_img_pos -nh 0 -un 0 > /dev/null
    #$WMTOOL -b $DistPath/"$FirmwareName".bin -o $DistPath/"$ImageName" -it $img_type -fc 0 -ra $run_img_pos -ih $run_img_header -nh 0 -un 0 > /dev/null
fi

if [ $signature == 1 ]
then
    openssl dgst -sign "$SDKPath"/tools/W806/ca/cakey.pem -sha1 -out $DistPath/"$ImageName"_sign.dat $DistPath/"$ImageName".img
    cat $DistPath/"$ImageName".img $DistPath/"$ImageName"_sign.dat > $DistPath/"$ImageName"_sign.img

    if [ $zip_type == 1 ]
    then
        $WMTOOL -b $DistPath/"$ImageName"_sign.img -o $DistPath/"$ImageName"_sign -it $img_type -fc 1 -ra $run_img_pos -ih $run_img_header -ua $upd_img_pos -nh 0  -un 0  > /dev/null
        mv $DistPath/"$ImageName"_sign_gz.img $DistPath/"$ImageName"_sign_ota.img
        printf "%s\t" $FirmwareName
        if [ $code_encrypt == 1 ]
        then
            printf "[%s]" $GREEN"ENCRYPED"$NORMAL
        else
            printf "[%s]" $GREY"UNENCRYPT"$NORMAL
        fi
        printf "[%s]" $GREEN"SIGNED"$NORMAL
        printf "[%s]" $GREEN"ZIPPED"$NORMAL
        printf "\n *  %s(%s)\n" $GREEN$DistPath/"$FirmwareName".bin$NORMAL $YELLOW$(stat --format=%s $DistPath/"$FirmwareName".bin)$NORMAL
        printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_sign.img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_sign.img)$NORMAL
        printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_sign_ota.img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_sign_ota.img)$NORMAL
        $SUM $DistPath/"$FirmwareName".elf $DistPath/"$FirmwareName".bin $DistPath/"$ImageName"_sign.img $DistPath/"$ImageName"_sign_ota.img
    else
        $WMTOOL -b "$SDKPath"/tools/W806/W806_secboot.bin -o "$SDKPath"/tools/W806/W806_secboot -it 0 -fc 0 -ra $sec_img_pos -ih $sec_img_header -ua $upd_img_pos -nh $run_img_header -un 0  > /dev/null
        cat "$SDKPath"/tools/W806/W806_secboot.img $DistPath/"$ImageName"_sign.img > $DistPath/"$ImageName"_sign.fls

        printf "%s\t" $FirmwareName
        if [ $code_encrypt == 1 ]
        then
            printf "[%s]" $GREEN"ENCRYPED"$NORMAL
        else
            printf "[%s]" $GREY"UNENCRYPT"$NORMAL
        fi
        printf "[%s]" $GREEN"SIGNED"$NORMAL
        printf "[%s]" $GREY"UNZIPPED"$NORMAL
        printf "\n *  %s(%s)\n" $GREEN$DistPath/"$FirmwareName".bin$NORMAL $YELLOW$(stat --format=%s $DistPath/"$FirmwareName".bin)$NORMAL
        printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_sign.img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_sign.img)$NORMAL
        printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_sign.fls$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_sign.fls)$NORMAL
        $SUM $DistPath/"$FirmwareName".elf $DistPath/"$FirmwareName".bin $DistPath/"$ImageName"_sign.img $DistPath/"$ImageName"_sign.fls
    fi
elif [ $zip_type == 1 ]
then
    $WMTOOL -b $DistPath/"$ImageName".img -o $DistPath/"$ImageName" -it $img_type -fc 1 -ra $run_img_pos -ih $run_img_header -ua $upd_img_pos -nh 0  -un 0  > /dev/null
    mv $DistPath/"$ImageName"_gz.img $DistPath/"$ImageName"_unsign_ota.img
    printf "%s\t" $FirmwareName
    if [ $code_encrypt == 1 ]
    then
        printf "[%s]" $GREEN"ENCRYPED"$NORMAL
    else
        printf "[%s]" $GREY"UNENCRYPT"$NORMAL
    fi
    printf "[%s]" $GREY"UNSIGNED"$NORMAL
    printf "[%s]" $GREEN"ZIPPED"$NORMAL
    printf "\n *  %s(%s)\n" $GREEN$DistPath/"$FirmwareName".bin$NORMAL $YELLOW$(stat --format=%s $DistPath/"$FirmwareName".bin)$NORMAL
    printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName".img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName".img)$NORMAL
    printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_unsign_ota.img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_unsign_ota.img)$NORMAL
    $SUM $DistPath/"$FirmwareName".elf $DistPath/"$FirmwareName".bin $DistPath/"$ImageName".img $DistPath/"$ImageName"_unsign_ota.img
else
    $WMTOOL -b "$SDKPath"/tools/W806/W806_secboot.bin -o "$SDKPath"/tools/W806/W806_secboot -it 0 -fc 0 -ra $sec_img_pos -ih $sec_img_header -ua $upd_img_pos -nh $run_img_header -un 0  > /dev/null
    #$WMTOOL -b "$SDKPath"/tools/W806/W806_secboot.bin -o "$SDKPath"/tools/W806/W806_secboot -it 0 -fc 0 -ra $sec_img_pos -ih $sec_img_header -nh $run_img_header -un 0  > /dev/null
    cat "$SDKPath"/tools/W806/W806_secboot.img $DistPath/"$ImageName".img > $DistPath/"$ImageName"_unsign.fls

    printf "%s\t" $FirmwareName
    if [ $code_encrypt == 1 ]
    then
        printf "[%s]" $GREEN"ENCRYPED"$NORMAL
    else
        printf "[%s]" $GREY"UNENCRYPT"$NORMAL
    fi
    printf "[%s]" $GREY"UNSIGNED"$NORMAL
    printf "[%s]" $GREY"UNZIPPED"$NORMAL
    printf "\n *  %s(%s)\n" $GREEN$DistPath/"$FirmwareName".bin$NORMAL $YELLOW$(stat --format=%s $DistPath/"$FirmwareName".bin)$NORMAL
    printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName".img$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName".img)$NORMAL
    printf " -> %s(%s)\n" $GREEN$DistPath/"$ImageName"_unsign.fls$NORMAL $YELLOW$(stat --format=%s $DistPath/"$ImageName"_unsign.fls)$NORMAL
    $SUM $DistPath/"$FirmwareName".elf $DistPath/"$FirmwareName".bin $DistPath/"$ImageName".img "$SDKPath"/tools/W806/W806_secboot.img $DistPath/"$ImageName"_unsign.fls
fi

rm -rf $DistPath/"$ImageName".img $DistPath/"$ImageName"_sign.img


