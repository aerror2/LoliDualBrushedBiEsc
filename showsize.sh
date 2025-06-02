	len=`tail -n2 $1  | grep -v 00000001FF | cut -b 2-3`
	off=`tail -n2 $1  | grep -v 00000001FF | cut -b 4-7`
	fsize=`echo "ibase=16;$off + $len"  | bc`
	imx=`echo $2|cut -b3-`
	maxsize=`echo "ibase=16;$imx"|bc`
    maxsizeK=`echo "scale=2;$maxsize/1024"|bc`
    fsizeK=`echo "scale=2;$fsize/1024"|bc`

	percent=`echo "scale=2;$fsize/$maxsize*100" | bc`
	echo "total hex size : $fsize of $maxsize (${fsizeK}K/${maxsizeK}K)  , usage: $percent%"