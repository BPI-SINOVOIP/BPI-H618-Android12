 function usage ()
{
     echo "########### start usage ##########"
     echo " ./tools.sh tomk : Enable necessary Android.mk and disable Android.bp to Android.bp_s"
     echo " ./tools.sh tobp : Enable necessary Android.bp and didable Android.mk to Android.mk_s"
     echo "########### usage end ############"
     echo ""
}
usage;
if [ "$1" == "tobp" ];then
eval $(find -name Android.bp_s  | sed "s/\(.*\)_s$/mv \1_s \1; /g")
eval $(find -name Android.mk | sed "s/\(.*\)/mv \1 \1_s; /g")
mv Android.mk_s Android.mk
mv conf/Android.mk_s conf/Android.mk
mv libcore/Android.mk_s libcore/Android.mk
mv libcore/stream/Android.mk_s libcore/stream/Android.mk
mv libcore/parser/Android.mk_s libcore/parser/Android.mk
mv android_adapter/Android.mk_s android_adapter/Android.mk
echo "rename Android.bp_s to Android.bp and Android.mk to Android.mk_s. Android.bp avaiable. Done!"
fi

if [ "$1" == "tomk" ];then
eval $(find -name Android.mk_s  | sed "s/\(.*\)_s$/mv \1_s \1; /g")
eval $(find -name Android.bp | sed "s/\(.*\)/mv \1 \1_s; /g")
#find -name Android.mk_s | xargs rename 's/\.mk_s/\.mk/' *
#find -name Android.bp | xargs rename 's/\.bp/\.bp_s/' *
echo "rename Android.mk_s to Android.mk and Android.bp to Android.bp_s. Android.mk avaiable. Done!"
fi
