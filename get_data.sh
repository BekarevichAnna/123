for i in "static" "dynamic" "guided"
do
    echo ";time" > "log1_$i.csv"
    for g in {-1..16}
    do
        res=$(./check.sh $g $i | sed 's/\./,/' | awk -v var="$g" '{print var","$0}')
        echo "$res"
    done >> "log1_$i.csv"
done 