function usage ()
{
    echo "########### start usage ##########"
    echo " ./mkswitch.sh tomk : switch necessary Android.bp to Android.mk"
    echo " ./mkswitch.sh tobp : switch necessary Android.mk to Android.bp"
    echo "########### usage end ############"
    echo ""
}
usage;

if [ "$1" == "tomk" ]; then
    find . -iname Android.bp |xargs -n1 -i mv {} {}_d
    find . -iname Android.mk_d |cut -d '.' -f 2|xargs -n1 -i mv ./{}.mk_d ./{}.mk
fi
if [ "$1" == "tobp" ]; then
    find . -iname Android.mk |xargs -n1 -i mv {} {}_d
    find . -iname Android.bp_d |cut -d '.' -f 2|xargs -n1 -i mv ./{}.bp_d ./{}.bp
fi
